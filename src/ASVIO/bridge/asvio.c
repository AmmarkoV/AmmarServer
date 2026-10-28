/*
ASVIO ( AmmarSerVer I/O ) bridge , see asvio.h and ../PLAN.md

URLs: http://ammar.gr
Written by Ammar Qammaz a.k.a. AmmarkoV 2026

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/random.h>
#include <sys/stat.h>
#include "asvio.h"
#include "../../AmmServerlib/AmmServerlib.h"

#define ASVIO_FINISHED_JOB_LIFETIME 600 //seconds a finished job can still be polled
#define ASVIO_MAX_JOBS 4096             //queued + running + finished-but-remembered
#define ASVIO_DEFAULT_POLL_WAIT 20      //seconds a poll waits for a change , unless it asks for less with wait=
#define ASVIO_MAX_POLL_WAIT 25
#define ASVIO_MAX_API_LENGTH 255
#define ASVIO_MAX_SESSION_LENGTH 64
#define ASVIO_STATUS_SIZE 16
#define ASVIO_SMALL_RESPONSE 4096

enum asvio_state
{
  ASVIO_QUEUED = 0, //nobody took it yet , it is in the queue
  ASVIO_TAKEN,      //Python has it ( waiting for a worker , or running )
  ASVIO_FINISHED
};

struct asvio_record
{
  struct asvio_job job; //<- first member , so the struct asvio_job * handed to Python is also the record
  enum asvio_state state;
  char status[ASVIO_STATUS_SIZE];
  unsigned int seq;
  char * payload;
  unsigned int payloadLength;
  unsigned char cancelRequested;
  time_t finishedAt;
  struct asvio_record * next;       //<- every job
  struct asvio_record * nextQueued; //<- jobs nobody took yet , oldest first
};

static struct AmmServer_Instance * server=0;
static struct AmmServer_RH_Context runContext={0};
static struct AmmServer_RH_Context pollContext={0};
static struct AmmServer_RH_Context cancelContext={0};
static struct AmmServer_RH_Context fileContext={0};
static char outputDirectory[MAX_FILE_PATH+16]={0}; //+16 for the asvio_out/ suffix

//One lock for everything below , jobs are few and every critical section is short
static pthread_mutex_t lock    = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  queued  = PTHREAD_COND_INITIALIZER; //signalled when a job enters the queue
static pthread_cond_t  changed = PTHREAD_COND_INITIALIZER; //broadcast when any job changes , pollers re-check theirs
static struct asvio_record * jobs=0;
static unsigned int numberOfJobs=0;
static struct asvio_record * queueHead=0;
static struct asvio_record * queueTail=0;
static int stopping=0;


static char * copyString(const char * from,unsigned int length)
{
  char * to = (char *) malloc(length+1);
  if (to==0) { return 0; }
  if ( (from!=0) && (length>0) ) { memcpy(to,from,length); }
  to[length]=0;
  return to;
}

static void freeJobInput(struct asvio_job * job)
{
  free(job->api);     job->api=0;
  free(job->session); job->session=0;
  free(job->data);    job->data=0; job->dataLength=0;
  for (unsigned int i=0; i<job->fileCount; i++)
  {
    free(job->files[i].field);
    free(job->files[i].filename);
    free(job->files[i].content);
  }
  memset(job->files,0,sizeof(job->files));
  job->fileCount=0;
}

static void freeRecord(struct asvio_record * record)
{
  freeJobInput(&record->job);
  free(record->payload);
  free(record);
}

//Callers hold the lock
static struct asvio_record * findJob(const char * jobID)
{
  for (struct asvio_record * record=jobs; record!=0; record=record->next)
  {
    if (strcmp(record->job.id,jobID)==0) { return record; }
  }
  return 0;
}

//Callers hold the lock
static void forgetOldJobs()
{
  time_t now = time(0);
  struct asvio_record ** link = &jobs;
  while (*link!=0)
  {
    struct asvio_record * record = *link;
    if ( (record->state==ASVIO_FINISHED) && (now - record->finishedAt > ASVIO_FINISHED_JOB_LIFETIME) )
    {
      *link = record->next;
      freeRecord(record);
      --numberOfJobs;
    } else
    { link = &record->next; }
  }
}

//Callers hold the lock
static void removeFromQueue(struct asvio_record * target)
{
  struct asvio_record * previous=0;
  for (struct asvio_record * record=queueHead; record!=0; record=record->nextQueued)
  {
    if (record==target)
    {
      if (previous==0) { queueHead=record->nextQueued; } else { previous->nextQueued=record->nextQueued; }
      if (queueTail==record) { queueTail=previous; }
      record->nextQueued=0;
      return;
    }
    previous=record;
  }
}

static int makeJobID(char * id)
{
  unsigned char random[(ASVIO_JOB_ID_SIZE-1)/2];
  if (getrandom(random,sizeof(random),0)!=sizeof(random)) { return 0; }
  for (unsigned int i=0; i<sizeof(random); i++) { snprintf(id+i*2,3,"%02x",random[i]); }
  return 1;
}

static void deadlineAfter(struct timespec * deadline,unsigned long milliseconds)
{
  clock_gettime(CLOCK_REALTIME,deadline); //PTHREAD_COND_INITIALIZER condvars time out against CLOCK_REALTIME
  deadline->tv_sec  += milliseconds/1000;
  deadline->tv_nsec += (milliseconds%1000)*1000000;
  if (deadline->tv_nsec>=1000000000) { deadline->tv_sec+=1; deadline->tv_nsec-=1000000000; }
}

//api names look like /predict , sessions are random ids made by the client
static int safeString(const char * str,unsigned int maxLength,int allowSlash)
{
  unsigned int length=0;
  for (const char * c=str; *c!=0; c++,length++)
  {
    if ( (*c>='a')&&(*c<='z') ) { continue; }
    if ( (*c>='A')&&(*c<='Z') ) { continue; }
    if ( (*c>='0')&&(*c<='9') ) { continue; }
    if ( (*c=='_')||(*c=='-') ) { continue; }
    if ( (*c=='/')&&(allowSlash) ) { continue; }
    return 0;
  }
  return (length<=maxLength);
}

//Output names are made by the Python side , e.g. 1f3a....png , anything else is refused
static int safeOutputName(const char * name)
{
  if ( (name[0]==0) || (name[0]=='.') ) { return 0; }
  if (strstr(name,"..")!=0) { return 0; }
  for (const char * c=name; *c!=0; c++)
  {
    if ( (*c>='a')&&(*c<='z') ) { continue; }
    if ( (*c>='A')&&(*c<='Z') ) { continue; }
    if ( (*c>='0')&&(*c<='9') ) { continue; }
    if ( (*c=='_')||(*c=='-')||(*c=='.') ) { continue; }
    return 0;
  }
  return 1;
}

static void * reply(struct AmmServer_DynamicRequest * rqst,unsigned int statusCode,const char * json)
{
  snprintf(rqst->content,rqst->MAXcontentSize,"%s",json);
  rqst->contentSize=strlen(rqst->content);
  rqst->headerResponse=statusCode;
  return 0;
}


/* POST /asvio/run.json : api , session , data and file parts become a queued job */
static void * runCallback(struct AmmServer_DynamicRequest * rqst)
{
  struct asvio_record * record = (struct asvio_record *) calloc(1,sizeof(struct asvio_record));
  if (record==0) { return reply(rqst,500,"{\"error\":\"out of memory\"}"); }
  struct asvio_job * job = &record->job;

  int outOfMemory=0;
  unsigned int items = rqst->POSTItemNumber;
  if (items>MAX_HTTP_POST_BOUNDARY_COUNT) { items=MAX_HTTP_POST_BOUNDARY_COUNT; }
  for (unsigned int i=0; (i<items)&&(rqst->POSTItem!=0); i++)
  {
    struct POSTRequestBoundaryContent * item = &rqst->POSTItem[i];
    if (item->name==0) { continue; }

    if ( (item->filename!=0) && (item->filenameSize>0) )
    {
      if (job->fileCount>=ASVIO_MAX_FILES) { continue; }
      struct asvio_file * file = &job->files[job->fileCount++];
      file->field    = copyString(item->name,item->nameSize);
      file->filename = copyString(item->filename,item->filenameSize);
      file->content  = copyString(item->value,item->valueSize);
      file->length   = item->valueSize;
      outOfMemory |= ( (file->field==0) || (file->filename==0) || (file->content==0) );
    } else
    if ( (strcmp(item->name,"api")==0) && (job->api==0) )
    {
      job->api = copyString(item->value,item->valueSize);
      outOfMemory |= (job->api==0);
    } else
    if ( (strcmp(item->name,"session")==0) && (job->session==0) )
    {
      job->session = copyString(item->value,item->valueSize);
      outOfMemory |= (job->session==0);
    } else
    if ( (strcmp(item->name,"data")==0) && (job->data==0) )
    {
      job->data = copyString(item->value,item->valueSize);
      job->dataLength = item->valueSize;
      outOfMemory |= (job->data==0);
    }
  }

  if (outOfMemory)                         { freeRecord(record); return reply(rqst,500,"{\"error\":\"out of memory\"}"); }
  if ( (job->api==0) || (job->data==0) )   { freeRecord(record); return reply(rqst,400,"{\"error\":\"a run needs multipart api and data fields\"}"); }
  if (job->session==0)                     { job->session=copyString("",0); if (job->session==0) { freeRecord(record); return reply(rqst,500,"{\"error\":\"out of memory\"}"); } }
  if (!safeString(job->api,ASVIO_MAX_API_LENGTH,1))         { freeRecord(record); return reply(rqst,400,"{\"error\":\"bad api name\"}"); }
  if (!safeString(job->session,ASVIO_MAX_SESSION_LENGTH,0)) { freeRecord(record); return reply(rqst,400,"{\"error\":\"bad session\"}"); }
  if (!makeJobID(job->id))                 { freeRecord(record); return reply(rqst,500,"{\"error\":\"no randomness for a job id\"}"); }

  record->state=ASVIO_QUEUED;
  snprintf(record->status,ASVIO_STATUS_SIZE,"queued");

  pthread_mutex_lock(&lock);
  forgetOldJobs();
  if ( (stopping) || (numberOfJobs>=ASVIO_MAX_JOBS) )
  {
    pthread_mutex_unlock(&lock);
    freeRecord(record);
    return reply(rqst,503,"{\"error\":\"server is busy\"}");
  }
  record->next=jobs; jobs=record; ++numberOfJobs;
  if (queueTail==0) { queueHead=record; } else { queueTail->nextQueued=record; }
  queueTail=record;
  pthread_cond_signal(&queued);
  pthread_mutex_unlock(&lock);

  snprintf(rqst->content,rqst->MAXcontentSize,"{\"job\":\"%s\"}",job->id);
  rqst->contentSize=strlen(rqst->content);
  return 0;
}


/* GET /asvio/poll.json?job=J&seq=N[&wait=S] : waits until the job is past seq N , finished , or S seconds passed */
static void * pollCallback(struct AmmServer_DynamicRequest * rqst)
{
  char jobID[ASVIO_JOB_ID_SIZE]={0};
  if (!_GETcpy(rqst,"job",jobID,ASVIO_JOB_ID_SIZE)) { return reply(rqst,400,"{\"error\":\"poll needs a job\"}"); }
  unsigned int seen = _GETuint(rqst,"seq");
  unsigned int wait = ASVIO_DEFAULT_POLL_WAIT;
  if (_GETexists(rqst,"wait")) { wait=_GETuint(rqst,"wait"); }
  if (wait>ASVIO_MAX_POLL_WAIT) { wait=ASVIO_MAX_POLL_WAIT; }

  struct timespec deadline;
  deadlineAfter(&deadline,wait*1000);

  pthread_mutex_lock(&lock);
  //Jobs can be forgotten while we wait , so look ours up again after every wake up instead of keeping the pointer
  struct asvio_record * record = findJob(jobID);
  while ( (record!=0) && (!stopping) && (record->state!=ASVIO_FINISHED) && (record->seq<=seen) )
  {
    int waited = pthread_cond_timedwait(&changed,&lock,&deadline);
    record = findJob(jobID);
    if (waited==ETIMEDOUT) { break; }
  }

  if (record==0)
  {
    pthread_mutex_unlock(&lock);
    return reply(rqst,404,"{\"error\":\"unknown job\"}");
  }

  int headerLength = snprintf(rqst->content,rqst->MAXcontentSize,"{\"job\":\"%s\",\"seq\":%u,\"status\":\"%s\",\"data\":",record->job.id,record->seq,record->status);
  unsigned long bodyLength = (record->payload!=0) ? record->payloadLength : 4;
  if ( (headerLength<0) || ((unsigned long) headerLength + bodyLength + 2 > rqst->MAXcontentSize) )
  {
    pthread_mutex_unlock(&lock);
    return reply(rqst,500,"{\"error\":\"output is larger than the poll response buffer , return big outputs as files\"}");
  }
  memcpy(rqst->content+headerLength,(record->payload!=0) ? record->payload : "null",bodyLength);
  rqst->content[headerLength+bodyLength]='}';
  rqst->content[headerLength+bodyLength+1]=0;
  rqst->contentSize = headerLength+bodyLength+1;
  pthread_mutex_unlock(&lock);
  return 0;
}


/* GET /asvio/cancel.json?job=J : a queued job is cancelled at once , a running one is flagged for Python */
static void * cancelCallback(struct AmmServer_DynamicRequest * rqst)
{
  char jobID[ASVIO_JOB_ID_SIZE]={0};
  if (!_GETcpy(rqst,"job",jobID,ASVIO_JOB_ID_SIZE)) { return reply(rqst,400,"{\"error\":\"cancel needs a job\"}"); }

  pthread_mutex_lock(&lock);
  struct asvio_record * record = findJob(jobID);
  if (record==0) { pthread_mutex_unlock(&lock); return reply(rqst,404,"{\"error\":\"unknown job\"}"); }

  if (record->state==ASVIO_QUEUED)
  {
    removeFromQueue(record);
    freeJobInput(&record->job);
    record->state=ASVIO_FINISHED;
    record->finishedAt=time(0);
    snprintf(record->status,ASVIO_STATUS_SIZE,"cancelled");
    ++record->seq;
    pthread_cond_broadcast(&changed);
  } else
  if (record->state==ASVIO_TAKEN)
  {
    record->cancelRequested=1;
  }
  pthread_mutex_unlock(&lock);
  return reply(rqst,200,"{\"ok\":1}");
}


/* GET /asvio/file.json?id=<name> : streams webroot/asvio_out/<name> , never through the file cache */
static void * fileCallback(struct AmmServer_DynamicRequest * rqst)
{
  char name[256]={0};
  if ( (!_GETcpy(rqst,"id",name,sizeof(name))) || (!safeOutputName(name)) ) { return reply(rqst,400,"{\"error\":\"bad file id\"}"); }

  char path[MAX_FILE_PATH*2];
  snprintf(path,sizeof(path),"%s%s",outputDirectory,name);
  AmmServer_DynamicRequestReturnFile(rqst,path); //AmmarServer answers 404 itself when the file is gone
  return 0;
}


int asvio_start(const char * name,const char * ip,unsigned int port,const char * webroot,unsigned int maxUploadMB,unsigned int maxResponseMB)
{
  if ( (server!=0) || (webroot==0) || (webroot[0]==0) ) { return 0; }

  char root[MAX_FILE_PATH];
  unsigned int rootLength = strlen(webroot);
  snprintf(root,MAX_FILE_PATH,"%s%s",webroot,(webroot[rootLength-1]=='/') ? "" : "/");
  snprintf(outputDirectory,sizeof(outputDirectory),"%sasvio_out/",root);

  //AmmarServer refuses ( 400 ) a resource whose directory does not resolve inside the web root , and that
  //includes dynamic ones , so /asvio/ has to exist on disk even though nothing is ever served from it
  char routeDirectory[MAX_FILE_PATH+16];
  snprintf(routeDirectory,sizeof(routeDirectory),"%sasvio",root);
  if ( ( (mkdir(routeDirectory,0700)!=0) && (errno!=EEXIST) ) || ( (mkdir(outputDirectory,0700)!=0) && (errno!=EEXIST) ) )
  {
    AmmServer_Error("ASVIO could not create %s or %s",routeDirectory,outputDirectory);
    return 0;
  }

  pthread_mutex_lock(&lock);
  stopping=0;
  pthread_mutex_unlock(&lock);

  server = AmmServer_Start(name,ip,port,0,root,root);
  if (server==0) { return 0; }

  AmmServer_SetIntSettingValue(server,AMMSET_MAX_POST_TRANSACTION_SIZE,(maxUploadMB+1)*1024*1024); //+1MB for the other fields and headers

  memset(&runContext,0,sizeof(runContext));
  memset(&pollContext,0,sizeof(pollContext));
  memset(&cancelContext,0,sizeof(cancelContext));
  memset(&fileContext,0,sizeof(fileContext));
  int ok = 1;
  ok &= AmmServer_AddResourceHandler(server,&runContext,   "/asvio/run.json",   ASVIO_SMALL_RESPONSE,     0,&runCallback,   DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);
  ok &= AmmServer_AddResourceHandler(server,&pollContext,  "/asvio/poll.json",  maxResponseMB*1024*1024,  0,&pollCallback,  DIFFERENT_PAGE_FOR_EACH_CLIENT);
  ok &= AmmServer_AddResourceHandler(server,&cancelContext,"/asvio/cancel.json",ASVIO_SMALL_RESPONSE,     0,&cancelCallback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  ok &= AmmServer_AddResourceHandler(server,&fileContext,  "/asvio/file.json",  MAX_FILE_PATH*2,          0,&fileCallback,  DIFFERENT_PAGE_FOR_EACH_CLIENT);
  if (!ok)
  {
    AmmServer_Error("ASVIO could not register its handlers");
    asvio_stop();
    return 0;
  }
  return 1;
}

int asvio_running()
{
  return ( (server!=0) && (AmmServer_Running(server)) );
}

void asvio_stop()
{
  pthread_mutex_lock(&lock);
  stopping=1;
  pthread_cond_broadcast(&queued);
  pthread_cond_broadcast(&changed);
  pthread_mutex_unlock(&lock);

  if (server!=0)
  {
    AmmServer_Stop(server);
    server=0;
  }
}

struct asvio_job * asvio_next_job(unsigned int timeoutMS)
{
  struct timespec deadline;
  deadlineAfter(&deadline,timeoutMS);

  pthread_mutex_lock(&lock);
  while ( (!stopping) && (queueHead==0) )
  {
    if (pthread_cond_timedwait(&queued,&lock,&deadline)==ETIMEDOUT) { break; }
  }

  struct asvio_record * record = 0;
  if ( (!stopping) && (queueHead!=0) )
  {
    record = queueHead;
    removeFromQueue(record);
    record->state = ASVIO_TAKEN;
  }
  pthread_mutex_unlock(&lock);

  if (record==0) { return 0; }
  return &record->job;
}

static int publish(const char * jobID,const char * status,const char * payload,unsigned int payloadLength,int final)
{
  if (jobID==0) { return 0; }
  //status goes into the poll JSON as it is , so only plain lower case words are taken
  const char * newStatus = (final) ? "done" : "running";
  if ( (status!=0) && (status[0]!=0) && (strlen(status)<ASVIO_STATUS_SIZE) && (strspn(status,"abcdefghijklmnopqrstuvwxyz")==strlen(status)) ) { newStatus=status; }

  char * copy = 0;
  if (payload!=0)
  {
    copy = copyString(payload,payloadLength);
    if (copy==0) { return 0; }
  }

  pthread_mutex_lock(&lock);
  struct asvio_record * record = findJob(jobID);
  if ( (record==0) || (record->state!=ASVIO_TAKEN) )
  {
    pthread_mutex_unlock(&lock);
    free(copy);
    return 0;
  }
  if (copy!=0)
  {
    free(record->payload);
    record->payload=copy;
    record->payloadLength=payloadLength;
  }
  snprintf(record->status,ASVIO_STATUS_SIZE,"%s",newStatus);
  ++record->seq;
  if (final)
  {
    record->state=ASVIO_FINISHED;
    record->finishedAt=time(0);
    freeJobInput(&record->job);
  }
  pthread_cond_broadcast(&changed);
  pthread_mutex_unlock(&lock);
  return 1;
}

int asvio_job_update(const char * jobID,const char * status,const char * payload,unsigned int payloadLength)
{
  return publish(jobID,status,payload,payloadLength,0);
}

int asvio_job_finish(const char * jobID,const char * status,const char * payload,unsigned int payloadLength)
{
  return publish(jobID,status,payload,payloadLength,1);
}

int asvio_job_cancelled(const char * jobID)
{
  if (jobID==0) { return 0; }
  pthread_mutex_lock(&lock);
  struct asvio_record * record = findJob(jobID);
  int cancelled = ( (record!=0) && (record->cancelRequested) );
  pthread_mutex_unlock(&lock);
  return cancelled;
}
