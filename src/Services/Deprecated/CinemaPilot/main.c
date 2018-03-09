/*
AmmarServer , simple template executable

URLs: http://ammar.gr
Written by Ammar Qammaz a.k.a. AmmarkoV 2012

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "../../../AmmServerlib/AmmServerlib.h"
#include "../../../InputParser/InputParser_C.h"

#define DEFAULT_BINDING_PORT 8080  // <--- Change this to 80 if you want to bind to the default http port..!

char webserver_root[MAX_FILE_PATH]="public_html/cinemaPilot/"; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
char templates_root[MAX_FILE_PATH]="public_html/templates/";

//char mplayerControllerPath[MAX_FILE_PATH]="/tmp/mplayer";
char mplayerControllerPath[MAX_FILE_PATH]="/home/ammar/Videos/videoController";
char fullScreenViewerPath[MAX_FILE_PATH]="/home/ammar/Documents/Programming/AmmarServer/src/Services/CinemaPilot";


/*! Dynamic content code ..! START!*/
/* A few words about dynamic content here..
   This is actually one of the key features on AmmarServer and maybe the reason that I started the whole project
   What I am trying to do here is serve content by directly linking the webserver to binary ( NOT Interpreted ) code
   in order to serve pages with the maximum possible efficiency and skipping all intermediate layers..

   PHP , Ruby , Python and all other "web-languages" are very nice and handy and to be honest I can do most of my work fine using PHP , MySQL and Apache
   However setting up , configuring and maintaining large projects with different database systems , separate configuration files for each of the sub parts
   and re deploying everything is a very tiresome affair.. Not to mention that despite the great work done by the apache  , PHP etc teams performance is wasted
   due to the interpreters of the various scripting languages used..

   Things can't get any faster than AmmarServer and the whole programming interface exposed to the programmer is ( imho ) very friendly and familiar to even inexperienced
   C developer..

   What follows is the decleration of some "Dynamic Content Resources" their Constructors/Destructors and their callback routines that fill them with the content to be served
   each time a client requests one of the pages..
*/

//The decleration of some dynamic content resources..
struct AmmServer_Instance  * default_server=0;
struct AmmServer_RequestOverride_Context GET_override={{0}};


struct AmmServer_RH_Context indexPage={0};
struct AmmServer_RH_Context remoteControl={0};
struct AmmServer_RH_Context random_chars={0};
struct AmmServer_RH_Context stats={0};


enum stateType
{
 STATE_UNINITIALIZED=0,
 STATE_PLAYING,
 STATE_FINISHED,
 //----------------
 NUMBER_OF_STATES
};



enum commandType
{
 CMD_TYPE_NONE=0,
 CMD_TYPE_TRAILER,
 CMD_TYPE_MOVIE,
 CMD_TYPE_LIGHTS_ON,
 CMD_TYPE_LIGHTS_OFF,
 CMD_TYPE_SOUND_ON,
 CMD_TYPE_SOUND_OFF,
 CMD_TYPE_INTERMISSION,
 CMD_TYPE_BELL_ON,
 CMD_TYPE_BELL_OFF,
 //----------------
 NUMBER_OF_COMMANDS
};



struct playlistItem
{
  int command;
  char playFile[512];


  struct tm * triggerTime;
  struct tm * stopTime;

};


struct playlist
{
   unsigned int numberOfItems;
   unsigned int maxItems;
   struct playlistItem item[100];

   unsigned int playlistActiveItem;
   unsigned int playlistState;
};





struct playlist * movieList={0};



int issueCommandToMplayer(const char * pathToPipe ,const char * command )
{
  //http://www.mplayerhq.hu/DOCS/tech/slave.txt <- for commands used
  FILE *fp =fopen(pathToPipe,"w");
  if (fp!=0)
  {
    fprintf(fp,"%s\n",command);
    fclose(fp);
    return 1;
  }
  return 0;
}


int pauseMplayer(const char * pathToPipe)
{
  return issueCommandToMplayer( pathToPipe , "pause");
}

int resumeMplayer(const char * pathToPipe)
{
  return issueCommandToMplayer( pathToPipe , "pause");
}

int stopMplayer(const char * pathToPipe)
{
  return issueCommandToMplayer( pathToPipe , "stop");
}

int intermission(unsigned int seconds)
{
 int i=system("xdotool mousemove --sync 1920 1080 click 0");

  char command[512]={0};
  snprintf(command,512,"%s start.jpg&",fullScreenViewerPath);

   i=system(command);
 //
 //./FullScreenViewer start.jpg&
 //sleep 10
 return (i==0);
}

int startMplayer( char * movie,char * subtitles,unsigned int startAt,unsigned int duration)
{
  AmmServer_Success("startMplayer ( %s , %s , %u , %u  ) \n",movie,subtitles,startAt,duration);

  char command[512]={0};
  snprintf(command,512,"mkfifo %s",mplayerControllerPath);
  int i=system(command);

  snprintf(command,512,"killall mplayer");
  i=system(command);

  //mplayer -ss 46 -endpos 17 -utf8 -fs -slang gr,en -sub Mad.Max.1979.1080p.BluRay.x264.anoXmous.srt -v Mad.Max.1979.1080p.BluRay.x264.anoXmous_.mp4

  snprintf(command,512,"mplayer -slave -input file=\"%s\" -utf8 -fs -slang gr,en -v \"%s\" -sub %s",mplayerControllerPath,movie,subtitles);
  i=system(command);

  if (i==0) {  AmmServer_Success("Success Executing( %s ) \n",command); } else
            {  AmmServer_Error("Error Executing( %s ) \n",command); }
  return (i==0);
}



int processCommand( struct playlist * newMovie , struct InputParserC * ipc , char * line , unsigned int words_count )
{
  unsigned int thisItem = newMovie->numberOfItems;

  if (InputParser_WordCompareNoCase(ipc,0,(char*)"TRAILER",10)==1)
    {
      newMovie->item[thisItem].command  =  CMD_TYPE_TRAILER;
      InputParser_GetWord(ipc,1,newMovie->item[thisItem].playFile,512);
      ++newMovie->numberOfItems;
    } else
  if (InputParser_WordCompareNoCase(ipc,0,(char*)"MOVIE",5)==1)
    {
      newMovie->item[thisItem].command  =  CMD_TYPE_MOVIE;
      InputParser_GetWord(ipc,1,newMovie->item[thisItem].playFile,512);
      ++newMovie->numberOfItems;
    } else
  if (InputParser_WordCompareNoCase(ipc,0,(char*)"LIGHTS",6)==1)
    {
      if (InputParser_GetWordInt(ipc,1)==1) {  newMovie->item[thisItem].command  =  CMD_TYPE_LIGHTS_ON;  } else
      if (InputParser_GetWordInt(ipc,1)==0) {  newMovie->item[thisItem].command  =  CMD_TYPE_LIGHTS_OFF;  }
      ++newMovie->numberOfItems;
    } else
  if (InputParser_WordCompareNoCase(ipc,0,(char*)"SOUND",5)==1)
    {
      if (InputParser_GetWordInt(ipc,1)==1) {  newMovie->item[thisItem].command  =  CMD_TYPE_SOUND_ON;  } else
      if (InputParser_GetWordInt(ipc,1)==0) {  newMovie->item[thisItem].command  =  CMD_TYPE_SOUND_OFF;  }
      ++newMovie->numberOfItems;
    } else
  if (InputParser_WordCompareNoCase(ipc,0,(char*)"INTERMISSION",12)==1)
    {
      newMovie->item[thisItem].command  =  CMD_TYPE_INTERMISSION;
      InputParser_GetWord(ipc,1,newMovie->item[thisItem].playFile,512);
      ++newMovie->numberOfItems;
    } else
  if (InputParser_WordCompareNoCase(ipc,0,(char*)"BELL",4)==1)
    {
      if (InputParser_GetWordInt(ipc,1)==1) {  newMovie->item[thisItem].command  =  CMD_TYPE_BELL_ON;  } else
      if (InputParser_GetWordInt(ipc,1)==0) {  newMovie->item[thisItem].command  =  CMD_TYPE_BELL_OFF;  }
      ++newMovie->numberOfItems;
    } else
    {
      fprintf(stderr,"Unknown string,treated as comment\n");
      return 0;
    }
  return 1;
}


struct playlist * readPlaylist(char * filename)
{

  FILE * fp = fopen(filename,"r");
  if (fp == 0 ) { fprintf(stderr,"Cannot open trajectory stream %s \n",filename); return 0; }


 unsigned int readOpResult = 0;
 char line [512]={0};

  struct playlist * newMovie=0;
  newMovie = (struct playlist *) malloc(sizeof(struct playlist) );


  struct InputParserC * ipc=0;
  ipc = InputParser_Create(1024,5);
  if (ipc==0) { fprintf(stderr,"Cannot allocate memory for new stream\n"); return 0; }

  while (!feof(fp))
  {
   //We get a new line out of the file
   readOpResult = (fgets(line,512,fp)!=0);
   if ( readOpResult != 0 )
    {
     //We tokenize it
     unsigned int words_count = InputParser_SeperateWords(ipc,line,0);
    if ( words_count > 0 )
      {
       processCommand(newMovie,ipc,line,words_count);
      } // End of line containing tokens
    } //End of getting a line while reading the file
   }

 fclose(fp);
 InputParser_Destroy(ipc);

 return newMovie;
}



int executePlaylistCurrentItem(struct playlist * thePlaylist)
{
  unsigned int currentItem = thePlaylist->playlistActiveItem;
  AmmServer_Success("executePlaylistCurrentItem ( %u , command type %u ) \n",currentItem,thePlaylist->item[currentItem].command);
      switch (thePlaylist->item[currentItem].command)
      {
        case  CMD_TYPE_NONE         : ++thePlaylist->playlistActiveItem; thePlaylist->playlistState=STATE_UNINITIALIZED; break;
        case  CMD_TYPE_TRAILER      : ++thePlaylist->playlistActiveItem; startMplayer(thePlaylist->item[currentItem].playFile,"nothing.srt",0,0);  break;
        case  CMD_TYPE_MOVIE        : ++thePlaylist->playlistActiveItem; startMplayer(thePlaylist->item[currentItem].playFile,"nothing.srt",0,0);  break;
        case  CMD_TYPE_LIGHTS_ON    : ++thePlaylist->playlistActiveItem;  break;
        case  CMD_TYPE_LIGHTS_OFF   : ++thePlaylist->playlistActiveItem;  break;
        case  CMD_TYPE_SOUND_ON     : ++thePlaylist->playlistActiveItem;  break;
        case  CMD_TYPE_SOUND_OFF    : ++thePlaylist->playlistActiveItem;  break;
        case  CMD_TYPE_INTERMISSION : ++thePlaylist->playlistActiveItem;  intermission(0 /*TODO ADD TIME HERE*/);  break;
        case  CMD_TYPE_BELL_ON      : ++thePlaylist->playlistActiveItem;  break;
        case  CMD_TYPE_BELL_OFF     : ++thePlaylist->playlistActiveItem;  break;
        default :
          AmmServer_Warning("Unrecognizable playlist item , bad condition\n",currentItem);

        break;
      };
}



int executePlaylist(struct playlist * thePlaylist)
{
  unsigned int transitionedToNext=0;
  if ( thePlaylist->playlistState == STATE_UNINITIALIZED )
  {
    //roll on next
    transitionedToNext=1;
    AmmServer_Success("First Hit on playlist , we will start without de-initializing anything\n");
  }



  if (transitionedToNext)
    {
       executePlaylistCurrentItem(thePlaylist);
    } else
    {

    }

  return 0;
}


int keepalivePlaylist(struct playlist * thePlaylist)
{
    /*
    ti ginete?
    ti na ginei
    ola kala.esu
    kala k egw ftiaxnw ayto gia tin katw aithousa
    k pio meta  mallon tha tin kanw na paw na paiksw kammia dota
    egw tha fugw se 5 lepta tha paw mia volta
    na prosexeis <3
    8eeeenx*/


  time_t clock = time(NULL);
  struct tm * ptm = gmtime ( &clock );
  //snprintf(output,maxOutput,"%s: %s, %u %s %u %02u:%02u:%02u GMT\n",label,days[ptm->tm_wday],ptm->tm_mday,months[ptm->tm_mon],EPOCH_YEAR_IN_TM_YEAR+ptm->tm_year,ptm->tm_hour,ptm->tm_min,ptm->tm_sec);

 return 1;
}


//This function prepares the content of  stats context , ( stats.content )
void * prepare_stats_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);

  fprintf(stderr,"Trying to write dynamic request to %p , max size %lu \n",rqst->content , rqst->MAXcontentSize);

  //No range check but since everything here is static max_stats_size should be big enough not to segfault with the strcat calls!
  snprintf(rqst->content,rqst->MAXcontentSize,
           "<html><head><title>Dynamic Content Enabled</title><meta http-equiv=\"refresh\" content=\"1\"></head>\
            <body>The date and time in AmmarServer is<br><h2>%02d-%02d-%02d %02d:%02d:%02d\n</h2>\
            The string you see is updated dynamically every time you get a fresh copy of this file!<br><br>\n\
            To include your own content see the <a href=\"https://github.com/AmmarkoV/AmmarServer/blob/master/src/main.c#L46\">\
            Dynamic content code label in ammarserver main.c</a><br>\n\
            If you dont need dynamic content at all consider disabling it from ammServ.conf or by setting DYNAMIC_CONTENT_RESOURCE_MAPPING_ENABLED=0; in \
            <a href=\"https://github.com/AmmarkoV/AmmarServer/blob/master/src/AmmServerlib/file_caching.c\">file_caching.c</a> and recompiling.!</body></html>",
           tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,   tm.tm_hour, tm.tm_min, tm.tm_sec);

  rqst->contentSize=strlen(rqst->content);
  return 0;
}


//This function prepares the content of form context , ( content )
void * prepare_remoteControl_callback(struct AmmServer_DynamicRequest * rqst)
{
 char data[256]={0};
 AmmServer_Success("New Control Request");

      if ( _GETcpy(rqst,"play",data,128) )
       {
         AmmServer_Success("Play pressed \n");
         //issueCommandToMplayer(mplayerControllerPath,"play");

         executePlaylist(movieList);
       }
      if ( _GETcpy(rqst,"pause",data,128) )
       {
         AmmServer_Success("Pause pressed\n");
         //issueCommandToMplayer("/tmp/mplayer","pause");
         pauseMplayer(mplayerControllerPath);
       }
      if ( _GETcpy(rqst,"previous",data,128) )
       {
         AmmServer_Success("Previous\n");
         issueCommandToMplayer(mplayerControllerPath,"previous");
       }
      if ( _GETcpy(rqst,"next",data,128) )
       {
         AmmServer_Success("Next\n");
         issueCommandToMplayer(mplayerControllerPath,"next");
       }

  strncpy(rqst->content,"<html><head><meta http-equiv=\"refresh\" content=\"0; url=control.html\" ></head><body>Ack</body></html>",rqst->MAXcontentSize);
  rqst->contentSize=strlen(rqst->content);
return 0;
}

//This function prepares the content of  form context , ( content )
void * prepare_indexPage(struct AmmServer_DynamicRequest  * rqst)
{
  strncpy(rqst->content,"<html><head><meta http-equiv=\"refresh\" content=\"0; url=cinema.html\"></head><body><a href=\"cinema.html\">Accessing</a></body></html>",rqst->MAXcontentSize);
  rqst->contentSize=strlen(rqst->content);
  return 0;
}


//This function prepares the content of  random_chars context , ( random_chars.content )
void * prepare_random_content_callback(struct AmmServer_DynamicRequest  * rqst)
{
  //No range check but since everything here is static max_stats_size should be big enough not to segfault with the strcat calls!
  strncpy(rqst->content,"<html><head><title>Random Number Generator</title><meta http-equiv=\"refresh\" content=\"1\"></head><body>",rqst->MAXcontentSize);

  char hex[16+1]={0};
  unsigned int i=0;
  for (i=0; i<1024; i++)
    {
        snprintf(hex,16, "%x ", rand()%256 );
        strcat(rqst->content,hex);
    }

  strcat(rqst->content,"</body></html>");

  rqst->contentSize=strlen(rqst->content);
  return 0;
}


//This function could alter the content of the URI requested and then return 1
void request_override_callback(void * request)
{
  //struct AmmServer_RequestOverride_Context * rqstContext = (struct AmmServer_RequestOverride_Context *) request;
  return;
}

//This function adds a Resource Handler for the pages stats.html and formtest.html and associates stats , form and their callback functions
void init_dynamic_content()
{
  AmmServer_AddRequestHandler(default_server,&GET_override,"GET",&request_override_callback);

  AmmServer_AddResourceHandler(default_server,&stats,"/stats.html",4096,0,&prepare_stats_content_callback,SAME_PAGE_FOR_ALL_CLIENTS);
  AmmServer_AddResourceHandler(default_server,&random_chars,"/random.html",4096,0,&prepare_random_content_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);

  AmmServer_AddResourceHandler(default_server,&remoteControl,"/remoteControl.html",4096,0,&prepare_remoteControl_callback,SAME_PAGE_FOR_ALL_CLIENTS);



  AmmServer_AddResourceHandler(default_server,&indexPage,"/index.html",4096,0,&prepare_indexPage,DIFFERENT_PAGE_FOR_EACH_CLIENT);
}

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(default_server,&stats,1);
    AmmServer_RemoveResourceHandler(default_server,&random_chars,1);
    AmmServer_RemoveResourceHandler(default_server,&indexPage,1);
}
/*! Dynamic content code ..! END ------------------------*/




int main(int argc, char *argv[])
{
    printf("\nAmmar Server %s starting up..\n",AmmServer_Version());
    //Check binary and header spec
    AmmServer_CheckIfHeaderBinaryAreTheSame(AMMAR_SERVER_HTTP_HEADER_SPEC);
    //Register termination signal for when we receive SIGKILL etc
    AmmServer_RegisterTerminationSignal(&close_dynamic_content);

    char bindIP[MAX_IP_STRING_SIZE];
    strcpy(bindIP,"0.0.0.0");

    unsigned int port=DEFAULT_BINDING_PORT;

    //Kick start AmmarServer , bind the ports , create the threads and get things going..!
    default_server = AmmServer_StartWithArgs(
                                             "CinemaPilot",
                                              argc,argv , //The internal server will use the arguments to change settings
                                              //If you don't want this look at the AmmServer_Start call
                                              bindIP,
                                              port,
                                              0, /*This means we don't want a specific configuration file*/
                                              webserver_root,
                                              templates_root
                                              );


    if (!default_server) { AmmServer_Error("Could not start server , shutting down everything.."); exit(1); }

    //Create dynamic content allocations and associate context to the correct files
    init_dynamic_content();


    movieList = readPlaylist("/home/ammar/Documents/Programming/AmmarServer/src/Services/CinemaPilot/playlist.ini");
    //stats.html and formtest.html should be availiable from now on..!

    //startMplayer(char * path, char * movie,char * subtitles,unsigned int startAt,unsigned int duration)



         while ( (AmmServer_Running(default_server))  )
           {
             //Main thread should just sleep and let the background threads do the hard work..!
             //In other applications the programmer could use the main thread to do anything he likes..
             //The only caveat is that he would takeup more CPU time from the server and that he would have to poll
             //the AmmServer_Running() call once in a while to make sure everything is in order
             //usleep(60000);
             sleep(1);

             keepalivePlaylist(movieList);
           }

    //Delete dynamic content allocations and remove stats.html and formtest.html from the server
    close_dynamic_content();

    //Stop the server and clean state
    AmmServer_Stop(default_server);
    AmmServer_Warning("Ammar Server stopped\n");
    return 0;
}
