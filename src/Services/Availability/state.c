#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "state.h"
#include "../../InputParser/InputParser_C.h"

struct poll polls[MAX_POLLS]={{{0}}};
unsigned int numberOfPolls=0;
unsigned int nextPollUID=1;
struct hashMap * pollHashMap=0;

struct AmmServer_Instance * default_server=0;

static unsigned int tokenSeeded=0;

void generateRandomToken(char * out , unsigned int outSize)
{
  if (!tokenSeeded) { srand((unsigned int)time(0) ^ (unsigned int)getpid()); tokenSeeded=1; }

  static const char alphabet[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  unsigned int len = outSize-1;
  if (len>39) { len=39; }

  unsigned int i=0;
  for (i=0; i<len; i++) { out[i]=alphabet[rand()%62]; }
  out[len]=0;
}


void formatOptionHTML(struct pollOption * opt , char * buf , unsigned int bufSize)
{
  char timePart[64]={0};
  if (opt->hasStartTime)
  {
    if (opt->hasEndTime)
    {
      snprintf(timePart,sizeof(timePart)," %02u:%02u - %02u:%02u",opt->startHour,opt->startMinute,opt->endHour,opt->endMinute);
    } else
    {
      snprintf(timePart,sizeof(timePart)," %02u:%02u",opt->startHour,opt->startMinute);
    }
  }
  snprintf(buf,bufSize,"%04u-%02u-%02u%s",opt->year,opt->month,opt->day,timePart);
}


struct poll * findPoll(const char * id)
{
  if (id==0) { return 0; }
  unsigned long slot=0;
  if (! hashMap_GetULongPayload(pollHashMap,id,&slot) ) { return 0; }
  if (slot>=numberOfPolls) { return 0; }
  return &polls[slot];
}


int isOwnerTokenValid(struct poll * p , const char * token)
{
  if ( (p==0) || (token==0) || (strlen(token)==0) ) { return 0; }
  return (strcmp(p->ownerToken,token)==0);
}


int loadPoll(const char * id , struct poll * p)
{
  if ( (id==0) || (p==0) ) { return 0; }
  memset(p,0,sizeof(struct poll));
  snprintf(p->id,sizeof(p->id),"%s",id);
  p->finalizedOptionIndex=-1;

  char titlePath[MAX_STRING_SIZE*2]={0};
  snprintf(titlePath,sizeof(titlePath),"data/polls/%s/title.txt",id);
  unsigned int titleLength=0;
  char * titleContent = AmmServer_ReadFileToMemory(titlePath,&titleLength);
  if (titleContent!=0)
  {
    snprintf(p->title,sizeof(p->title),"%.*s",(int)titleLength,titleContent);
    free(titleContent);
  }

  char metaPath[MAX_STRING_SIZE*2]={0};
  snprintf(metaPath,sizeof(metaPath),"data/polls/%s/poll.ini",id);
  FILE * fp = fopen(metaPath,"r");
  if (fp==0) { fprintf(stderr,"Cannot open %s\n",metaPath); return 0; }

  struct InputParserC * ipc = InputParser_Create(LINE_MAX_LENGTH,5);
  if (ipc==0) { fclose(fp); return 0; }

  char line[LINE_MAX_LENGTH]={0};
  while (!feof(fp))
  {
    if (fgets(line,LINE_MAX_LENGTH,fp)!=0)
    {
      unsigned int words = InputParser_SeperateWords(ipc,line,0);
      if (words>0)
      {
        if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"CLOSED")==1)
        {
          p->closed = InputParser_GetWordInt(ipc,1);
        } else
        if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"FINALIZED")==1)
        {
          p->finalizedOptionIndex = InputParser_GetWordInt(ipc,1);
        } else
        if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"OWNER")==1)
        {
          InputParser_GetWord(ipc,1,p->ownerToken,sizeof(p->ownerToken));
        } else
        if (InputParser_WordCompareNoCaseAuto(ipc,0,(char*)"OPTION")==1)
        {
          if (p->numberOfOptions<MAX_OPTIONS)
          {
            struct pollOption * opt = &p->options[p->numberOfOptions];
            opt->year        = InputParser_GetWordInt(ipc,1);
            opt->month       = InputParser_GetWordInt(ipc,2);
            opt->day         = InputParser_GetWordInt(ipc,3);
            opt->hasStartTime= InputParser_GetWordInt(ipc,4);
            opt->startHour   = InputParser_GetWordInt(ipc,5);
            opt->startMinute = InputParser_GetWordInt(ipc,6);
            opt->hasEndTime  = InputParser_GetWordInt(ipc,7);
            opt->endHour     = InputParser_GetWordInt(ipc,8);
            opt->endMinute   = InputParser_GetWordInt(ipc,9);
            ++p->numberOfOptions;
          }
        }
      }
    }
  }

  InputParser_Destroy(ipc);
  fclose(fp);

  loadResponses(p);
  return 1;
}


int savePollMeta(struct poll * p)
{
  if (p==0) { return 0; }

  char titlePath[MAX_STRING_SIZE*2]={0};
  snprintf(titlePath,sizeof(titlePath),"data/polls/%s/title.txt",p->id);
  AmmServer_WriteFileFromMemory(titlePath,p->title,strlen(p->title));

  char metaPath[MAX_STRING_SIZE*2]={0};
  snprintf(metaPath,sizeof(metaPath),"data/polls/%s/poll.ini",p->id);
  FILE * fp = fopen(metaPath,"w");
  if (fp==0) { fprintf(stderr,"Cannot open %s for writing\n",metaPath); return 0; }

  fprintf(fp,"closed(%u)\n",p->closed);
  fprintf(fp,"finalized(%d)\n",p->finalizedOptionIndex);
  fprintf(fp,"owner(%s)\n",p->ownerToken);

  unsigned int i=0;
  for (i=0; i<p->numberOfOptions; i++)
  {
    struct pollOption * opt = &p->options[i];
    fprintf(fp,"option(%u,%u,%u,%u,%u,%u,%u,%u,%u)\n",
            opt->year,opt->month,opt->day,
            opt->hasStartTime,opt->startHour,opt->startMinute,
            opt->hasEndTime,opt->endHour,opt->endMinute);
  }

  fclose(fp);
  return 1;
}


int loadResponses(struct poll * p)
{
  if (p==0) { return 0; }
  p->numberOfResponses=0;

  char responsesDir[MAX_STRING_SIZE*2]={0};
  snprintf(responsesDir,sizeof(responsesDir),"data/polls/%s/responses",p->id);

  DIR * dp = opendir(responsesDir);
  if (dp==0) { return 1; } //No responses yet is not an error

  unsigned int highestIndex=0;
  unsigned int anyFound=0;
  struct dirent * ep;
  while ( (ep=readdir(dp)) != 0 )
  {
    unsigned int idx=0;
    if (sscanf(ep->d_name,"%u_votes.ini",&idx)==1)
    {
      anyFound=1;
      if (idx>highestIndex) { highestIndex=idx; }
    }
  }
  closedir(dp);

  if (!anyFound) { return 1; }

  unsigned int i=0;
  for (i=0; i<=highestIndex; i++)
  {
    if (p->numberOfResponses>=MAX_RESPONSES) { break; }

    char votesPath[MAX_STRING_SIZE*2]={0};
    snprintf(votesPath,sizeof(votesPath),"%s/%u_votes.ini",responsesDir,i);
    FILE * fp = fopen(votesPath,"r");
    if (fp==0) { continue; }

    struct pollResponse * r = &p->responses[p->numberOfResponses];
    memset(r,0,sizeof(struct pollResponse));

    struct InputParserC * ipc = InputParser_Create(LINE_MAX_LENGTH,5);
    char line[LINE_MAX_LENGTH]={0};
    while (!feof(fp))
    {
      if (fgets(line,LINE_MAX_LENGTH,fp)!=0)
      {
        unsigned int words = InputParser_SeperateWords(ipc,line,0);
        if (words>0)
        {
          char keyword[32]={0};
          InputParser_GetWord(ipc,0,keyword,sizeof(keyword));
          unsigned int voteIndex=0;
          if (sscanf(keyword,"vote%u",&voteIndex)==1)
          {
            if (voteIndex<MAX_OPTIONS) { r->votes[voteIndex] = (unsigned char) InputParser_GetWordInt(ipc,1); }
          }
        }
      }
    }
    InputParser_Destroy(ipc);
    fclose(fp);

    char namePath[MAX_STRING_SIZE*2]={0};
    snprintf(namePath,sizeof(namePath),"%s/%u_name.txt",responsesDir,i);
    unsigned int nameLength=0;
    char * nameContent = AmmServer_ReadFileToMemory(namePath,&nameLength);
    if (nameContent!=0)
    {
      snprintf(r->name,sizeof(r->name),"%.*s",(int)nameLength,nameContent);
      free(nameContent);
    } else
    {
      continue; //No name file means this slot was never really written , skip it
    }

    char emailPath[MAX_STRING_SIZE*2]={0};
    snprintf(emailPath,sizeof(emailPath),"%s/%u_email.txt",responsesDir,i);
    unsigned int emailLength=0;
    char * emailContent = AmmServer_ReadFileToMemory(emailPath,&emailLength);
    if (emailContent!=0)
    {
      r->hasEmail=1;
      snprintf(r->email,sizeof(r->email),"%.*s",(int)emailLength,emailContent);
      free(emailContent);
    }

    ++p->numberOfResponses;
  }

  return 1;
}


int saveResponse(struct poll * p , unsigned int responseIndex)
{
  if ( (p==0) || (responseIndex>=p->numberOfResponses) ) { return 0; }
  struct pollResponse * r = &p->responses[responseIndex];

  char responsesDir[MAX_STRING_SIZE*2]={0};
  snprintf(responsesDir,sizeof(responsesDir),"data/polls/%s/responses",p->id);
  mkdir(responsesDir,0755); //Fine if it already exists

  char namePath[MAX_STRING_SIZE*2]={0};
  snprintf(namePath,sizeof(namePath),"%s/%u_name.txt",responsesDir,responseIndex);
  AmmServer_WriteFileFromMemory(namePath,r->name,strlen(r->name));

  if (r->hasEmail)
  {
    char emailPath[MAX_STRING_SIZE*2]={0};
    snprintf(emailPath,sizeof(emailPath),"%s/%u_email.txt",responsesDir,responseIndex);
    AmmServer_WriteFileFromMemory(emailPath,r->email,strlen(r->email));
  }

  char votesPath[MAX_STRING_SIZE*2]={0};
  snprintf(votesPath,sizeof(votesPath),"%s/%u_votes.ini",responsesDir,responseIndex);
  FILE * fp = fopen(votesPath,"w");
  if (fp==0) { return 0; }

  unsigned int i=0;
  for (i=0; i<p->numberOfOptions; i++)
  {
    fprintf(fp,"vote%u(%u)\n",i,r->votes[i]);
  }
  fclose(fp);

  return 1;
}


int findResponseByName(struct poll * p , const char * name)
{
  if ( (p==0) || (name==0) || (strlen(name)==0) ) { return -1; }
  unsigned int i=0;
  for (i=0; i<p->numberOfResponses; i++)
  {
    if (strcasecmp(p->responses[i].name,name)==0) { return (int) i; }
  }
  return -1;
}


int findOrCreateResponseSlot(struct poll * p , const char * name)
{
  if ( (p==0) || (name==0) || (strlen(name)==0) ) { return -1; }

  int existing = findResponseByName(p,name);
  if (existing>=0) { return existing; }

  if (p->numberOfResponses>=MAX_RESPONSES) { return -1; }

  unsigned int newIndex = p->numberOfResponses;
  memset(&p->responses[newIndex],0,sizeof(struct pollResponse));
  snprintf(p->responses[newIndex].name,sizeof(p->responses[newIndex].name),"%s",name);
  ++p->numberOfResponses;
  return (int) newIndex;
}


int createPoll(const char * title , struct pollOption * options , unsigned int numberOfOptions , char * outID , unsigned int outIDSize , char * outOwnerToken , unsigned int outOwnerTokenSize)
{
  if ( (title==0) || (options==0) || (numberOfOptions==0) ) { return 0; }
  if (numberOfPolls>=MAX_POLLS) { fprintf(stderr,"createPoll : site is full\n"); return 0; }

  char id[32]={0};
  snprintf(id,sizeof(id),"a%06u",nextPollUID);

  char dirPath[MAX_STRING_SIZE*2]={0};
  snprintf(dirPath,sizeof(dirPath),"data/polls/%s",id);
  if (mkdir(dirPath,0755)!=0) { fprintf(stderr,"createPoll : cannot create %s\n",dirPath); return 0; }

  unsigned int slot = numberOfPolls;
  struct poll * p = &polls[slot];
  memset(p,0,sizeof(struct poll));
  snprintf(p->id,sizeof(p->id),"%s",id);
  snprintf(p->title,sizeof(p->title),"%s",title);
  generateRandomToken(p->ownerToken,sizeof(p->ownerToken));
  p->closed=0;
  p->finalizedOptionIndex=-1;

  unsigned int cappedCount = (numberOfOptions>MAX_OPTIONS) ? MAX_OPTIONS : numberOfOptions;
  p->numberOfOptions = cappedCount;
  memcpy(p->options,options,sizeof(struct pollOption)*cappedCount);

  savePollMeta(p);

  hashMap_AddULong(pollHashMap,id,slot);
  ++numberOfPolls;
  ++nextPollUID;

  if ( (outID!=0) && (outIDSize>0) ) { snprintf(outID,outIDSize,"%s",id); }
  if ( (outOwnerToken!=0) && (outOwnerTokenSize>0) ) { snprintf(outOwnerToken,outOwnerTokenSize,"%s",p->ownerToken); }
  return 1;
}


int loadAllPolls()
{
  pollHashMap = hashMap_Create(100,100,0,1);

  DIR * dp = opendir("data/polls");
  if (dp==0) { fprintf(stderr,"Cannot open data/polls directory\n"); return 0; }

  struct dirent * ep;
  unsigned int highestNumericID=0;

  while ( (ep=readdir(dp)) != 0 )
  {
    if (strcmp(ep->d_name,".")==0)  { continue; }
    if (strcmp(ep->d_name,"..")==0) { continue; }
    if (numberOfPolls>=MAX_POLLS) { break; }

    unsigned int slot = numberOfPolls;
    if ( loadPoll(ep->d_name,&polls[slot]) )
    {
      hashMap_AddULong(pollHashMap,ep->d_name,slot);
      ++numberOfPolls;

      unsigned int numericPart = (unsigned int) atoi(ep->d_name + ((ep->d_name[0]=='a') ? 1 : 0));
      if (numericPart>highestNumericID) { highestNumericID=numericPart; }
    }
  }

  closedir(dp);
  nextPollUID = highestNumericID+1;

  fprintf(stderr,"Loaded %u polls , nextPollUID=%u\n",numberOfPolls,nextPollUID);
  return 1;
}


int unloadAllPolls()
{
  if (pollHashMap!=0) { hashMap_Destroy(pollHashMap); pollHashMap=0; }
  return 1;
}
