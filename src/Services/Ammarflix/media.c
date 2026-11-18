#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "media.h"

#define MAX_PROBE_OUTPUT 65536

static char thumbsDirectory[MAX_FILE_PATH+1];
static char subsDirectory[MAX_FILE_PATH+1];

//ffmpeg runs one at a time , a page full of thumbnails would otherwise spawn dozens at once
static pthread_mutex_t ffmpegLock=PTHREAD_MUTEX_INITIALIZER;

static const char * textSubtitleCodecs[]={ "subrip","srt","ass","ssa","webvtt","mov_text","text",0 };

struct EmbeddedStream
{
  unsigned int index;
  char codec[32];
  char language[32];
  char title[128];
};


int media_init(const char * cacheDirectory)
{
  snprintf(thumbsDirectory,sizeof(thumbsDirectory),"%s/thumbs",cacheDirectory);
  snprintf(subsDirectory,sizeof(subsDirectory),"%s/subs",cacheDirectory);
  mkdir(cacheDirectory,0750);
  mkdir(thumbsDirectory,0750);
  mkdir(subsDirectory,0750);

  struct stat st;
  return ( (stat(thumbsDirectory,&st)==0) && (stat(subsDirectory,&st)==0) );
}

//Cache files are keyed on id + mtime so a replaced file gets a fresh thumbnail/subtitles
static void cacheKey(const struct Video * video,char * out,unsigned int outSize)
{
  snprintf(out,outSize,"%s_%lx",video->id,(unsigned long) video->mtime);
}

static int nonEmptyFileExists(const char * path)
{
  struct stat st;
  return ( (stat(path,&st)==0) && (S_ISREG(st.st_mode)) && (st.st_size>0) );
}

//fork+exec without a shell , so file names never get interpreted. stdout goes to output ( if given ).
//Returns 1 if the program ran and exited with 0.
static int runProgram(char * const argv[],char * output,unsigned int outputSize)
{
  int pipeFD[2]={-1,-1};
  if ( (output!=0) && (pipe(pipeFD)!=0) ) { return 0; }

  pid_t pid=fork();
  if (pid<0)
  {
    if (output!=0) { close(pipeFD[0]); close(pipeFD[1]); }
    return 0;
  }

  if (pid==0)
  {
    int devNull=open("/dev/null",O_RDWR);
    dup2(devNull,0);
    dup2( (output!=0) ? pipeFD[1] : devNull ,1);
    dup2(devNull,2);
    execvp(argv[0],argv);
    _exit(127);
  }

  if (output!=0)
  {
    close(pipeFD[1]);
    unsigned int total=0;
    ssize_t got;
    while ( (got=read(pipeFD[0],output+total,outputSize-1-total))>0 )
    {
      total+=(unsigned int) got;
      if (total>=outputSize-1) { break; }
    }
    output[total]=0;
    close(pipeFD[0]);
  }

  int status=0;
  if (waitpid(pid,&status,0)!=pid) { return 0; }
  return ( (WIFEXITED(status)) && (WEXITSTATUS(status)==0) );
}


int media_thumbnail(const struct Video * video,char * outPath,unsigned int outPathSize)
{
  char key[64];
  cacheKey(video,key,sizeof(key));

  char failMarker[MAX_FILE_PATH+1];
  char temporary[MAX_FILE_PATH+1];
  snprintf(outPath,outPathSize,"%s/%s.jpg",thumbsDirectory,key);
  snprintf(failMarker,sizeof(failMarker),"%s/%s.fail",thumbsDirectory,key);
  snprintf(temporary,sizeof(temporary),"%s/%s.tmp.jpg",thumbsDirectory,key);

  if (nonEmptyFileExists(outPath)) { return 1; }

  int success=0;
  pthread_mutex_lock(&ffmpegLock);

  struct stat st;
  if (nonEmptyFileExists(outPath))       { success=1; } else //Generated while we waited
  if (stat(failMarker,&st)==0)           { success=0; } else //Already failed once , don't retry on every page view
  {
    //A minute in skips most intros/black frames , short clips fall back to their first frames
    const char * seekPositions[]={ "60","2" };
    unsigned int i=0;
    for (i=0; (i<2) && (!success); i++)
    {
      char * argv[]={ "ffmpeg","-nostdin","-v","error","-y",
                      "-ss",(char*) seekPositions[i],"-i",(char*) video->path,
                      "-frames:v","1","-vf","scale=480:-2","-q:v","4",
                      temporary,0 };
      runProgram(argv,0,0);
      if ( (nonEmptyFileExists(temporary)) && (rename(temporary,outPath)==0) ) { success=1; }
    }

    if (!success)
    {
      unlink(temporary);
      FILE * fp=fopen(failMarker,"w");
      if (fp!=0) { fclose(fp); }
    }
  }

  pthread_mutex_unlock(&ffmpegLock);
  return success;
}


static int isTextSubtitleCodec(const char * codec)
{
  unsigned int i=0;
  for (i=0; textSubtitleCodecs[i]!=0; i++)
   { if (strcmp(codec,textSubtitleCodecs[i])==0) { return 1; } }
  return 0;
}

//Copies a value of ffprobe's "flat" output ( key="value" or key=value ) without its quotes
static void copyFlatValue(const char * value,char * out,unsigned int outSize)
{
  if (*value=='"') { ++value; }
  unsigned int length=0;
  while ( (*value!=0) && (*value!='\n') && (length<outSize-1) ) { out[length++]=*value; ++value; }
  if ( (length>0) && (out[length-1]=='"') ) { --length; }
  out[length]=0;
}

//Lists the text subtitle streams inside the video ( ffprobe output is cached in subs/<key>.probe )
static unsigned int probeEmbeddedSubtitles(const struct Video * video,struct EmbeddedStream * streams,unsigned int maxStreams)
{
  char key[64];
  cacheKey(video,key,sizeof(key));
  char probePath[MAX_FILE_PATH+1];
  snprintf(probePath,sizeof(probePath),"%s/%s.probe",subsDirectory,key);

  char * probe=(char*) malloc(MAX_PROBE_OUTPUT);
  if (probe==0) { return 0; }
  probe[0]=0;

  FILE * fp=fopen(probePath,"r");
  if (fp!=0)
  {
    size_t got=fread(probe,1,MAX_PROBE_OUTPUT-1,fp);
    probe[got]=0;
    fclose(fp);
  } else
  {
    char * argv[]={ "ffprobe","-v","error","-select_streams","s",
                    "-show_entries","stream=index,codec_name:stream_tags=language,title",
                    "-of","flat",(char*) video->path,0 };
    if (!runProgram(argv,probe,MAX_PROBE_OUTPUT)) { probe[0]=0; }

    //Cached even when empty , so files without subtitles are only probed once
    fp=fopen(probePath,"w");
    if (fp!=0) { fputs(probe,fp); fclose(fp); }
  }

  //Lines look like : streams.stream.0.index=2 , streams.stream.0.codec_name="subrip" , streams.stream.0.tags.language="eng"
  struct EmbeddedStream all[MAX_SUBTITLE_TRACKS];
  int highestEntry=-1;
  memset(all,0,sizeof(all));

  char * line=probe;
  while ( (line!=0) && (*line!=0) )
  {
    char * next=strchr(line,'\n');
    if (strncmp(line,"streams.stream.",15)==0)
    {
      char * field=0;
      long entry=strtol(line+15,&field,10);
      char * equals=strchr(line,'=');
      if ( (field!=0) && (*field=='.') && (equals!=0) && (entry>=0) && (entry<MAX_SUBTITLE_TRACKS) )
      {
        ++field;
        struct EmbeddedStream * s=&all[entry];
        if (strncmp(field,"index=",6)==0)         { s->index=(unsigned int) atoi(equals+1); } else
        if (strncmp(field,"codec_name=",11)==0)   { copyFlatValue(equals+1,s->codec,sizeof(s->codec)); } else
        if (strncmp(field,"tags.language=",14)==0){ copyFlatValue(equals+1,s->language,sizeof(s->language)); } else
        if (strncmp(field,"tags.title=",11)==0)   { copyFlatValue(equals+1,s->title,sizeof(s->title)); }
        if (entry>highestEntry) { highestEntry=(int) entry; }
      }
    }
    line = (next!=0) ? next+1 : 0;
  }
  free(probe);

  unsigned int count=0;
  int i=0;
  for (i=0; i<=highestEntry; i++)
  {
    if ( (count<maxStreams) && (isTextSubtitleCodec(all[i].codec)) ) { streams[count++]=all[i]; }
  }
  return count;
}

//Splits a video path into its directory and its file name without extension
static int splitVideoPath(const struct Video * video,char * directory,unsigned int directorySize,char * stem,unsigned int stemSize)
{
  const char * slash=strrchr(video->path,'/');
  if (slash==0) { return 0; }
  snprintf(directory,directorySize,"%.*s",(int) (slash-video->path),video->path);
  snprintf(stem,stemSize,"%s",slash+1);
  char * dot=strrchr(stem,'.');
  if (dot!=0) { *dot=0; }
  return 1;
}

//"Movie.srt" , "Movie.en.srt" , "Movie.el.forced.vtt" all belong to "Movie.mkv" , label = the part in between
static int isSidecarOf(const char * name,const char * stem,char * label,unsigned int labelSize)
{
  unsigned int stemLength=strlen(stem);
  if (strncmp(name,stem,stemLength)!=0) { return 0; }
  if (name[stemLength]!='.') { return 0; }

  const char * extension=strrchr(name,'.');
  if ( (strcasecmp(extension,".srt")!=0) && (strcasecmp(extension,".vtt")!=0) ) { return 0; }

  const char * middle=name+stemLength+1;
  if (middle>=extension) { snprintf(label,labelSize,"%s",extension+1); } //Movie.srt
                    else { snprintf(label,labelSize,"%.*s",(int) (extension-middle),middle); }
  return 1;
}

static int compareTracks(const void * a,const void * b)
{
  return strcmp( ((const struct SubtitleTrack *) a)->key , ((const struct SubtitleTrack *) b)->key );
}

unsigned int media_listSubtitles(const struct Video * video,struct SubtitleTrack * tracks,unsigned int maxTracks)
{
  unsigned int count=0;

  char directory[MAX_FILE_PATH+1];
  char stem[256];
  if (splitVideoPath(video,directory,sizeof(directory),stem,sizeof(stem)))
  {
    DIR * dir=opendir(directory);
    if (dir!=0)
    {
      struct dirent * entry;
      while ( ( (entry=readdir(dir))!=0 ) && (count<maxTracks) )
      {
        char label[256];
        if ( (strlen(entry->d_name)<sizeof(tracks[count].key)) && (isSidecarOf(entry->d_name,stem,label,sizeof(label))) )
        {
          tracks[count].kind=SUBTITLE_SIDECAR;
          snprintf(tracks[count].key,sizeof(tracks[count].key),"%s",entry->d_name);
          snprintf(tracks[count].label,sizeof(tracks[count].label),"%s (file)",label);
          ++count;
        }
      }
      closedir(dir);
      if (count>1) { qsort(tracks,count,sizeof(struct SubtitleTrack),compareTracks); }
    }
  }

  struct EmbeddedStream streams[MAX_SUBTITLE_TRACKS];
  unsigned int streamCount=probeEmbeddedSubtitles(video,streams,MAX_SUBTITLE_TRACKS);
  unsigned int i=0;
  for (i=0; (i<streamCount) && (count<maxTracks); i++)
  {
    tracks[count].kind=SUBTITLE_EMBEDDED;
    snprintf(tracks[count].key,sizeof(tracks[count].key),"%u",streams[i].index);
    snprintf(tracks[count].label,sizeof(tracks[count].label),"%s%s%s (embedded #%u)",
             (streams[i].language[0]!=0) ? streams[i].language : "unknown",
             (streams[i].title[0]!=0) ? " - " : "",
             streams[i].title,
             streams[i].index);
    ++count;
  }

  return count;
}

int media_sidecarSubtitlePath(const struct Video * video,const char * name,char * outPath,unsigned int outPathSize)
{
  if ( (strchr(name,'/')!=0) || (name[0]=='.') ) { return 0; }

  char directory[MAX_FILE_PATH+1];
  char stem[256];
  char label[256];
  if (!splitVideoPath(video,directory,sizeof(directory),stem,sizeof(stem))) { return 0; }
  if (!isSidecarOf(name,stem,label,sizeof(label))) { return 0; }

  if (snprintf(outPath,outPathSize,"%s/%s",directory,name)>=(int) outPathSize) { return 0; }
  return nonEmptyFileExists(outPath);
}

int media_embeddedSubtitlePath(const struct Video * video,unsigned int streamIndex,char * outPath,unsigned int outPathSize)
{
  char key[64];
  cacheKey(video,key,sizeof(key));
  snprintf(outPath,outPathSize,"%s/%s.%u.vtt",subsDirectory,key,streamIndex);
  if (nonEmptyFileExists(outPath)) { return 1; }

  struct EmbeddedStream streams[MAX_SUBTITLE_TRACKS];
  unsigned int streamCount=probeEmbeddedSubtitles(video,streams,MAX_SUBTITLE_TRACKS);

  unsigned int i=0;
  int known=0;
  for (i=0; i<streamCount; i++) { if (streams[i].index==streamIndex) { known=1; } }
  if (!known) { return 0; }

  pthread_mutex_lock(&ffmpegLock);
  if (!nonEmptyFileExists(outPath)) //Not extracted while we waited
  {
    //ffmpeg has to read the whole file to extract a subtitle stream , so extract every text stream in one pass
    char mapArgs[MAX_SUBTITLE_TRACKS][16];
    char temporaries[MAX_SUBTITLE_TRACKS][MAX_FILE_PATH+1];
    char finals[MAX_SUBTITLE_TRACKS][MAX_FILE_PATH+1];
    char * argv[16+(MAX_SUBTITLE_TRACKS*7)];
    unsigned int argc=0;
    argv[argc++]="ffmpeg"; argv[argc++]="-nostdin"; argv[argc++]="-v"; argv[argc++]="error"; argv[argc++]="-y";
    argv[argc++]="-i"; argv[argc++]=(char*) video->path;

    for (i=0; i<streamCount; i++)
    {
      snprintf(mapArgs[i],sizeof(mapArgs[i]),"0:%u",streams[i].index);
      snprintf(temporaries[i],sizeof(temporaries[i]),"%s/%s.%u.tmp.vtt",subsDirectory,key,streams[i].index);
      snprintf(finals[i],sizeof(finals[i]),"%s/%s.%u.vtt",subsDirectory,key,streams[i].index);
      argv[argc++]="-map"; argv[argc++]=mapArgs[i];
      argv[argc++]="-c:s"; argv[argc++]="webvtt";
      argv[argc++]="-f";   argv[argc++]="webvtt";
      argv[argc++]=temporaries[i];
    }
    argv[argc]=0;

    runProgram(argv,0,0);

    for (i=0; i<streamCount; i++)
    {
      if (nonEmptyFileExists(temporaries[i])) { rename(temporaries[i],finals[i]); }
                                         else { unlink(temporaries[i]); }
    }
  }
  pthread_mutex_unlock(&ffmpegLock);

  return nonEmptyFileExists(outPath);
}
