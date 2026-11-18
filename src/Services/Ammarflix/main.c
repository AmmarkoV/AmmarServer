/*
Ammarflix , a private video library Service built on AmmarServer
Scans media directories for video files and lets logged in users search and watch them ( with subtitles )

URLs: http://ammar.gr
Written by Ammar Qammaz a.k.a. AmmarkoV

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
#include <unistd.h>
#include <pthread.h>
#include <termios.h>
#include <sys/stat.h>
#include <time.h>

#include "../../AmmServerlib/AmmServerlib.h"
#include "../../UserAccounts/userAccountsWeb.h"

#include "library.h"
#include "media.h"
#include "pages.h"

#define DEFAULT_LOCAL_PORT 8097
#define RESCAN_EVERY_SECONDS (10*60)
#define MAX_SUBTITLE_SIZE (8*1024*1024)

static char appRoot[MAX_FILE_PATH+1]="./";
static char dataDirectory[MAX_FILE_PATH+1];
static char cacheDirectory[MAX_FILE_PATH+1];
static char resDirectory[MAX_FILE_PATH+1];
static char webRoot[MAX_FILE_PATH+1];
static char templatesRoot[MAX_FILE_PATH+1];

struct AmmServer_Instance * default_server=0;
static struct UserAccountDatabase * uadb=0;

static volatile int keepRescanning=1;

struct AmmServer_RH_Context indexView={0};
struct AmmServer_RH_Context libraryView={0};
struct AmmServer_RH_Context loginView={0};
struct AmmServer_RH_Context logoutView={0};
struct AmmServer_RH_Context watchView={0};
struct AmmServer_RH_Context streamView={0};
struct AmmServer_RH_Context thumbView={0};
struct AmmServer_RH_Context subtitleView={0};
struct AmmServer_RH_Context randomView={0};
//Flair from res/ ( see resFiles[] ) , public since the login page needs it too
struct AmmServer_RH_Context cssView={0};
struct AmmServer_RH_Context jsView={0};
struct AmmServer_RH_Context faviconView={0};
struct AmmServer_RH_Context faviconSVGView={0};
struct AmmServer_RH_Context soundView={0};


/* 1 if this process may bind port 443 : root , CAP_NET_BIND_SERVICE ( capability bit 10 ) , or a lowered ip_unprivileged_port_start */
static int mayBindPort443()
{
  if (geteuid()==0) { return 1; }

  int canBind=0;
  char line[256];
  FILE * fp = fopen("/proc/self/status","r");
  if (fp!=0)
  {
    while (fgets(line,sizeof(line),fp)!=0)
     { if (strncmp(line,"CapEff:",7)==0) { canBind = ( (strtoull(line+7,0,16)>>10) & 1 ); break; } }
    fclose(fp);
  }
  if (canBind) { return 1; }

  fp = fopen("/proc/sys/net/ipv4/ip_unprivileged_port_start","r");
  if (fp!=0)
  {
    if ( (fgets(line,sizeof(line),fp)!=0) && (atoi(line)<=443) ) { canBind=1; }
    fclose(fp);
  }
  return canBind;
}

/* data/ and www/ live next to the executable , so the service can be launched from any working directory */
static void resolveAppRoot()
{
  char exePath[MAX_FILE_PATH+1]={0};
  ssize_t exePathLength = readlink("/proc/self/exe",exePath,sizeof(exePath)-1);
  if (exePathLength>0)
  {
    exePath[exePathLength]=0;
    char * lastSlash = strrchr(exePath,'/');
    if (lastSlash!=0) { *(lastSlash+1)=0; snprintf(appRoot,sizeof(appRoot),"%s",exePath); }
  }

  snprintf(dataDirectory,sizeof(dataDirectory),"%sdata",appRoot);
  snprintf(cacheDirectory,sizeof(cacheDirectory),"%sdata/cache",appRoot);
  snprintf(resDirectory,sizeof(resDirectory),"%sres",appRoot);
  //The web root only holds what AmmarServer itself may serve without a login , i.e. nothing ;
  //every page is a dynamic resource that checks the session first
  snprintf(webRoot,sizeof(webRoot),"%swww/",appRoot);
  snprintf(templatesRoot,sizeof(templatesRoot),"%swww/templates/",appRoot);
}

//The AmmarServer wide account list ( <repo>/db/users.db , also used by Social ) , the executable lives in src/Services/Ammarflix/
static char userDatabase[MAX_FILE_PATH+1];

static int openUserDatabase()
{
  snprintf(userDatabase,sizeof(userDatabase),"%s../../../db/users.db",appRoot);
  uadb = uadb_initializeUserAccountDatabase(userDatabase);
  return (uadb!=0);
}

//The server only ever reads accounts ( logins don't change the list ) , so it frees its copy without
//uadb_closeUserAccountDatabase() , which would rewrite the shared file from this process' startup
//snapshot and drop any account another service added since.
static void releaseUserDatabase()
{
  if (uadb==0) { return; }
  free(uadb->userList);
  free(uadb);
  uadb=0;
}


static void readPassword(const char * prompt,char * password,unsigned int passwordSize)
{
  struct termios original,silent;
  int isTerminal = isatty(0) && (tcgetattr(0,&original)==0);
  if (isTerminal)
  {
    fprintf(stderr,"%s",prompt);
    silent=original;
    silent.c_lflag &= ~ECHO;
    tcsetattr(0,TCSANOW,&silent);
  }

  password[0]=0;
  if (fgets(password,passwordSize,stdin)==0) { password[0]=0; }
  password[strcspn(password,"\r\n")]=0;

  if (isTerminal) { tcsetattr(0,TCSANOW,&original); fprintf(stderr,"\n"); }
}

//ammarflix -adduser NAME , the password is read from stdin ( never from argv , where ps and shell history would see it )
static int addUser(const char * username)
{
  if (!uadbWeb_isValidUsername(username)) { fprintf(stderr,"Usernames are 3-31 characters , letters/digits/_/- only\n"); return 1; }
  if (!openUserDatabase()) { fprintf(stderr,"Could not open the user database %s\n",userDatabase); return 1; }

  int result=1;
  if (uadbWeb_userAccountExists(uadb,username)) { fprintf(stderr,"User %s already exists\n",username); } else
  {
    char password[128],again[128];
    readPassword("Password: ",password,sizeof(password));
    if (isatty(0)) { readPassword("Repeat password: ",again,sizeof(again)); }
              else { snprintf(again,sizeof(again),"%s",password); }

    if (strlen(password)<8)                { fprintf(stderr,"Passwords need at least 8 characters\n"); } else
    if (strcmp(password,again)!=0)         { fprintf(stderr,"Passwords do not match\n"); } else
    if (!uadb_addUser(uadb,username,password,"0.0.0.0","cli")) { fprintf(stderr,"Could not add user\n"); } else
    {
      fprintf(stderr,"User %s added\n",username);
      result=0;
    }
    memset(password,0,sizeof(password));
    memset(again,0,sizeof(again));
  }

  uadb_closeUserAccountDatabase(&uadb);
  return result;
}


static int currentUser(struct AmmServer_DynamicRequest * rqst,char * username,unsigned int usernameSize)
{
  return AmmServer_CurrentUsername(rqst,username,usernameSize);
}

static int isVideoID(const char * id)
{
  unsigned int i=0;
  for (i=0; i<VIDEO_ID_SIZE-1; i++)
   { if ( !( ((id[i]>='0')&&(id[i]<='9')) || ((id[i]>='a')&&(id[i]<='f')) ) ) { return 0; } }
  return (id[VIDEO_ID_SIZE-1]==0);
}

//Resolves ?v= to a video , 1=found
static int requestedVideo(struct AmmServer_DynamicRequest * rqst,struct Video * video)
{
  char id[VIDEO_ID_SIZE+1]={0};
  if (!_GETcpy(rqst,"v",id,sizeof(id))) { return 0; }
  if (!isVideoID(id)) { return 0; }
  return library_find(id,video);
}

static void denyRequest(struct AmmServer_DynamicRequest * rqst,unsigned int httpCode)
{
  rqst->headerResponse=httpCode;
  snprintf(rqst->content,rqst->MAXcontentSize,"%u",httpCode);
  rqst->contentSize=strlen(rqst->content);
}


//index.html is only the login page ( never cached ) , once logged in it sends the browser on to library.html
void * index_callback(struct AmmServer_DynamicRequest * rqst)
{
  char username[64];
  if (currentUser(rqst,username,sizeof(username))) { page_redirect(rqst,"library.html"); return 0; }
  page_login(rqst,0);
  return 0;
}

void * library_callback(struct AmmServer_DynamicRequest * rqst)
{
  char username[64];
  if (!currentUser(rqst,username,sizeof(username))) { page_redirect(rqst,"index.html"); return 0; }

  char query[512]={0};
  char pageString[16]={0};
  _GETcpy(rqst,"q",query,sizeof(query));
  _GETcpy(rqst,"p",pageString,sizeof(pageString));
  urlDecodeInPlace(query);

  page_library(rqst,username,query,(unsigned int) strtoul(pageString,0,10));
  return 0;
}

void * login_callback(struct AmmServer_DynamicRequest * rqst)
{
  char username[64]={0};
  char password[128]={0};
  _POSTcpy(rqst,"username",username,sizeof(username));
  _POSTcpy(rqst,"password",password,sizeof(password));

  if ( (username[0]!=0) && (password[0]!=0) && (AmmServer_Login(rqst,uadb,username,password)) )
       { page_redirect(rqst,"library.html"); }
  else
  {
    //A failed attempt is a strike against the client IP , enough of them ban it from every AmmarServer service for a while
    if ( (username[0]!=0) || (password[0]!=0) ) { AmmServer_SignalCountAsBadClientBehaviour(rqst); }
    page_login(rqst,"Wrong username or password");
  }

  memset(password,0,sizeof(password));
  return 0;
}

void * logout_callback(struct AmmServer_DynamicRequest * rqst)
{
  AmmServer_Logout(rqst);
  page_redirect(rqst,"index.html");
  return 0;
}

void * watch_callback(struct AmmServer_DynamicRequest * rqst)
{
  char username[64];
  if (!currentUser(rqst,username,sizeof(username))) { page_redirect(rqst,"index.html"); return 0; }

  struct Video video;
  if (!requestedVideo(rqst,&video)) { page_message(rqst,404,"This video is not in the library ( anymore ? )"); return 0; }

  struct SubtitleTrack tracks[MAX_SUBTITLE_TRACKS];
  unsigned int trackCount=media_listSubtitles(&video,tracks,MAX_SUBTITLE_TRACKS);
  page_watch(rqst,username,&video,tracks,trackCount);
  return 0;
}

void * stream_callback(struct AmmServer_DynamicRequest * rqst)
{
  char username[64];
  if (!currentUser(rqst,username,sizeof(username))) { denyRequest(rqst,403); return 0; }

  struct Video video;
  if (!requestedVideo(rqst,&video)) { denyRequest(rqst,404); return 0; }

  //Streamed by AmmarServer itself , which also answers Range requests ( seeking )
  if (!AmmServer_DynamicRequestReturnFile(rqst,video.path)) { denyRequest(rqst,500); }
  return 0;
}

void * thumb_callback(struct AmmServer_DynamicRequest * rqst)
{
  char username[64];
  if (!currentUser(rqst,username,sizeof(username))) { denyRequest(rqst,403); return 0; }

  struct Video video;
  if (!requestedVideo(rqst,&video)) { denyRequest(rqst,404); return 0; }

  char thumbnail[MAX_FILE_PATH+1];
  if ( (!media_thumbnail(&video,thumbnail,sizeof(thumbnail))) ||
       (!AmmServer_DynamicRequestReturnFile(rqst,thumbnail)) ) { denyRequest(rqst,404); }
  return 0;
}

//r on any page , or the Random button
void * random_callback(struct AmmServer_DynamicRequest * rqst)
{
  char username[64];
  if (!currentUser(rqst,username,sizeof(username))) { page_redirect(rqst,"index.html"); return 0; }

  char target[64]="library.html";
  library_lock();
  unsigned int count=library_count();
  if (count>0) { snprintf(target,sizeof(target),"watch.html?v=%s",library_get((unsigned int) random()%count)->id); }
  library_unlock();
  page_redirect(rqst,target);
  return 0;
}

//res/ files go out exactly as they are on disk ( edit them without rebuilding ) , Content-Type follows the extension
static void serveResFile(struct AmmServer_DynamicRequest * rqst,const char * name)
{
  char path[MAX_FILE_PATH+1];
  snprintf(path,sizeof(path),"%s/%s",resDirectory,name);
  if (!AmmServer_DynamicRequestReturnFile(rqst,path)) { denyRequest(rqst,404); }
}
void * css_callback(struct AmmServer_DynamicRequest * rqst)        { serveResFile(rqst,"ammarflix.css"); return 0; }
void * js_callback(struct AmmServer_DynamicRequest * rqst)         { serveResFile(rqst,"ammarflix.js");  return 0; }
void * favicon_callback(struct AmmServer_DynamicRequest * rqst)    { serveResFile(rqst,"favicon.ico");   return 0; }
void * faviconSVG_callback(struct AmmServer_DynamicRequest * rqst) { serveResFile(rqst,"favicon.svg");   return 0; }
void * sound_callback(struct AmmServer_DynamicRequest * rqst)      { serveResFile(rqst,"sound.mp3");     return 0; }

//Subtitles go out as text/plain ( /subtitle.txt ) , the page's script parses them into cues
void * subtitle_callback(struct AmmServer_DynamicRequest * rqst)
{
  char username[64];
  if (!currentUser(rqst,username,sizeof(username))) { denyRequest(rqst,403); return 0; }

  struct Video video;
  if (!requestedVideo(rqst,&video)) { denyRequest(rqst,404); return 0; }

  char path[MAX_FILE_PATH+1];
  char value[256]={0};
  int found=0;
  if (_GETcpy(rqst,"s",value,sizeof(value)))
  {
    urlDecodeInPlace(value);
    found=media_sidecarSubtitlePath(&video,value,path,sizeof(path));
  } else
  if (_GETcpy(rqst,"e",value,sizeof(value)))
  {
    char * end=0;
    unsigned long streamIndex=strtoul(value,&end,10);
    if ( (end!=value) && (*end==0) ) { found=media_embeddedSubtitlePath(&video,(unsigned int) streamIndex,path,sizeof(path)); }
  }
  if (!found) { denyRequest(rqst,404); return 0; }

  FILE * fp=fopen(path,"rb");
  if (fp==0) { denyRequest(rqst,404); return 0; }
  size_t got=fread(rqst->content,1,rqst->MAXcontentSize-1,fp);
  fclose(fp);
  rqst->content[got]=0;
  rqst->contentSize=got;
  return 0;
}


static void * rescanThread(void * unused)
{
  unsigned int seconds=0;
  while (keepRescanning)
  {
    sleep(1);
    if (++seconds>=RESCAN_EVERY_SECONDS)
    {
      seconds=0;
      unsigned int count=library_scan();
      fprintf(stderr,"Ammarflix rescan : %u videos\n",count);
    }
  }
  return 0;
}


//alwaysFresh : the page depends on being logged in , so neither the server nor the browser may reuse an older copy
//( a cached logged-out answer would bounce a freshly logged in user back to the login page )
static void addPage(struct AmmServer_RH_Context * context,const char * resource,unsigned int memory,void * callback,unsigned int scenario,unsigned char alwaysFresh)
{
  AmmServer_AddResourceHandler(default_server,context,resource,memory,0,callback,scenario);
  if (alwaysFresh) { AmmServer_DoNOTCacheResourceHandler(default_server,context); }
  //Cookie sessions : AmmServer_Login issues the AMMSESSID cookie , AmmServer_CurrentUsername reads it
  context->requestContext.useSessionLifecycle=1;
}

static void init_dynamic_content()
{
  addPage(&indexView,"/index.html",16384,&index_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT,1);
  addPage(&libraryView,"/library.html",1024*1024,&library_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT,1);
  addPage(&loginView,"/login.html",16384,&login_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES,1);
  addPage(&logoutView,"/logout.html",4096,&logout_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT,1);
  addPage(&watchView,"/watch.html",128*1024,&watch_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT,1);
  //Video , thumbnail and subtitle bytes never change for a given ?v= , so let the browser keep them
  addPage(&streamView,"/stream",4096,&stream_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT,0);
  addPage(&thumbView,"/thumb.jpg",4096,&thumb_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT,0);
  addPage(&subtitleView,"/subtitle.txt",MAX_SUBTITLE_SIZE,&subtitle_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT,0);
  addPage(&randomView,"/random.html",4096,&random_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT,1);
  addPage(&cssView,"/ammarflix.css",4096,&css_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT,0);
  addPage(&jsView,"/ammarflix.js",4096,&js_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT,0);
  addPage(&faviconView,"/favicon.ico",4096,&favicon_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT,0);
  addPage(&faviconSVGView,"/favicon.svg",4096,&faviconSVG_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT,0);
  addPage(&soundView,"/sound.mp3",4096,&sound_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT,0);
}

static void close_dynamic_content()
{
  AmmServer_RemoveResourceHandler(default_server,&indexView,1);
  AmmServer_RemoveResourceHandler(default_server,&libraryView,1);
  AmmServer_RemoveResourceHandler(default_server,&loginView,1);
  AmmServer_RemoveResourceHandler(default_server,&logoutView,1);
  AmmServer_RemoveResourceHandler(default_server,&watchView,1);
  AmmServer_RemoveResourceHandler(default_server,&streamView,1);
  AmmServer_RemoveResourceHandler(default_server,&thumbView,1);
  AmmServer_RemoveResourceHandler(default_server,&subtitleView,1);
  AmmServer_RemoveResourceHandler(default_server,&randomView,1);
  AmmServer_RemoveResourceHandler(default_server,&cssView,1);
  AmmServer_RemoveResourceHandler(default_server,&jsView,1);
  AmmServer_RemoveResourceHandler(default_server,&faviconView,1);
  AmmServer_RemoveResourceHandler(default_server,&faviconSVGView,1);
  AmmServer_RemoveResourceHandler(default_server,&soundView,1);
}


static void usage()
{
  fprintf(stderr,"Usage : ammarflix -sslcert CERT.pem -sslkey KEY.pem -media DIR [-media DIR2 ..] [-port LOCALPORT]\n");
  fprintf(stderr,"        ammarflix -adduser USERNAME   ( reads the password from stdin )\n");
  fprintf(stderr,"Ammarflix only serves HTTPS , on port 443 ( needs root or CAP_NET_BIND_SERVICE ).\n");
  fprintf(stderr,"Plain HTTP is bound to 127.0.0.1:%u only.\n",DEFAULT_LOCAL_PORT);
}

int main(int argc, char *argv[])
{
    printf("\nAmmar Server %s starting up (Ammarflix)..\n",AmmServer_Version());
    AmmServer_CheckIfHeaderBinaryAreTheSame(AMMAR_SERVER_HTTP_HEADER_SPEC);

    resolveAppRoot();
    mkdir(dataDirectory,0750);
    mkdir(webRoot,0755);

    char sslCert[MAX_FILE_PATH+1]={0};
    char sslKey[MAX_FILE_PATH+1]={0};
    unsigned int port=DEFAULT_LOCAL_PORT;

    int i=0;
    for (i=1; i<argc; i++)
    {
      if ( (strcmp(argv[i],"-adduser")==0) && (i+1<argc) ) { return addUser(argv[i+1]); } else
      if ( (strcmp(argv[i],"-sslcert")==0) && (i+1<argc) ) { snprintf(sslCert,sizeof(sslCert),"%s",argv[++i]); } else
      if ( (strcmp(argv[i],"-sslkey")==0)  && (i+1<argc) ) { snprintf(sslKey,sizeof(sslKey),"%s",argv[++i]); } else
      if ( (strcmp(argv[i],"-port")==0)    && (i+1<argc) ) { port=(unsigned int) atoi(argv[++i]); } else
      if ( (strcmp(argv[i],"-media")==0)   && (i+1<argc) )
      {
        ++i;
        if (!library_addRoot(argv[i]))                        { fprintf(stderr,"Cannot use media directory %s\n",argv[i]); return 1; }
        if (!AmmServer_AddTrustedStreamDirectory(library_getRoot(library_rootCount()-1))) { fprintf(stderr,"Cannot stream from %s\n",argv[i]); return 1; }
      } else
      { usage(); return 1; }
    }

    if ( (sslCert[0]==0) || (sslKey[0]==0) ) { fprintf(stderr,"Ammarflix refuses to run without HTTPS.\n"); usage(); return 1; }
    if (library_rootCount()==0)              { fprintf(stderr,"No media directories given.\n"); usage(); return 1; }

    if ( (!media_init(cacheDirectory)) || (!AmmServer_AddTrustedStreamDirectory(cacheDirectory)) )
      { fprintf(stderr,"Cannot use cache directory %s\n",cacheDirectory); return 1; }
    if (!AmmServer_AddTrustedStreamDirectory(resDirectory)) { fprintf(stderr,"Cannot serve %s\n",resDirectory); return 1; }
    srandom((unsigned int) time(0) ^ (unsigned int) getpid()); //Only picks random videos
    if (!openUserDatabase()) { fprintf(stderr,"Could not open the user database %s\n",userDatabase); return 1; }

    fprintf(stderr,"Ammarflix : %u videos in %u media directories\n",library_scan(),library_rootCount());

    //Plain HTTP only on loopback , the outside world only reaches the HTTPS listener ( all interfaces , port 443 )
    default_server = AmmServer_StartSSL("ammarflix","127.0.0.1",port,"",webRoot,templatesRoot,sslCert,sslKey);
    if (!default_server) { AmmServer_Error("Could not start server , shutting down everything.."); return 1; }

    if ( (!default_server->sslAvailable) || (default_server->sslserversock<0) )
    {
      AmmServer_Error("HTTPS did not come up ( bad certificate/key , OpenSSL support not built in , or port 443 needs root/CAP_NET_BIND_SERVICE ) , refusing to run without it");
      if (!mayBindPort443())
      {
        //Rebuilding the binary drops the capability , so this is needed again after every build
        fprintf(stderr,"\nThis process is not allowed to bind port 443 , run Ammarflix as root or allow it once ( again after every rebuild ) with :\n");
        fprintf(stderr,"  sudo setcap cap_net_bind_service=+ep %sammarflix\n\n",appRoot);
      }
      AmmServer_Stop(default_server);
      releaseUserDatabase();
      return 1;
    }

    init_dynamic_content();
    AmmServer_RegisterTerminationSignal(0); //SIGINT/SIGTERM make AmmServer_Running() return 0 , so the teardown below runs

    pthread_t rescanner;
    int rescanning = (pthread_create(&rescanner,0,rescanThread,0)==0);

    while (AmmServer_Running(default_server)) { sleep(1); }

    keepRescanning=0;
    if (rescanning) { pthread_join(rescanner,0); }

    close_dynamic_content();
    AmmServer_Stop(default_server);
    releaseUserDatabase();
    library_free();
    AmmServer_Warning("Ammarflix stopped\n");
    return 0;
}
