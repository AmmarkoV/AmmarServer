/* Minimal AmmarServer application used only by the test suite ( tests/test_application.py ).
 *
 *   testservice <ammarserver arguments>          dynamic pages : /cookie.html , /session.html , /echo.html , /typed ,
 *                                                /virtual/folder/echo ( a folder that does not exist on disk )
 *   testservice -api <rings> -p PORT -bind IP    API mode ( rings=0 : epoll layer , >0 : io_uring )
 *
 * It exists so that cookies , sessions and API mode , which the vanilla ammarserver binary has no handler
 * for , are exercised by the same tests ( and therefore by the valgrind / sanitizer runs ). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../../src/AmmServerlib/AmmServerlib.h"

static struct AmmServer_Instance * server=0;
static struct AmmServer_RH_Context cookiePage={0};
static struct AmmServer_RH_Context sessionPage={0};
static struct AmmServer_RH_Context echoPage={0};
static struct AmmServer_RH_Context typedPage={0};
static struct AmmServer_RH_Context nestedPage={0};

//Echoes the "flavor" cookie back and , when asked with ?set=value , sets it
static void * cookie_callback(struct AmmServer_DynamicRequest * rqst)
{
  char flavor[128]={0};
  char wanted[128]={0};
  if (!_COOKIEcpy(rqst,"flavor",flavor,sizeof(flavor))) { snprintf(flavor,sizeof(flavor),"none"); }
  if (_GETcpy(rqst,"set",wanted,sizeof(wanted))) { AmmServer_SetCookie(rqst,"flavor",wanted,3600,1); }
  snprintf(rqst->content,rqst->MAXcontentSize,"flavor=%s",flavor);
  rqst->contentSize=strlen(rqst->content);
  return 0;
}

//Counts the visits of one session
static void * session_callback(struct AmmServer_DynamicRequest * rqst)
{
  char value[32];
  unsigned int visits=_SESSIONuint(rqst,"visits")+1;
  snprintf(value,sizeof(value),"%u",visits);
  _SESSIONset(rqst,"visits",value);
  snprintf(rqst->content,rqst->MAXcontentSize,"visits=%u",visits);
  rqst->contentSize=strlen(rqst->content);
  return 0;
}

//Echoes the GET field "a" and the POST field "b"
static void * echo_callback(struct AmmServer_DynamicRequest * rqst)
{
  char a[256]={0};
  char b[1024]={0};
  _GETcpy(rqst,"a",a,sizeof(a));
  _POSTcpy(rqst,"b",b,sizeof(b));
  snprintf(rqst->content,rqst->MAXcontentSize,"a=[%s] b=[%s]",a,b);
  rqst->contentSize=strlen(rqst->content);
  return 0;
}

//An extension-less resource whose handler declares its Content-Type ; ?status=N answers with that status
static void * typed_callback(struct AmmServer_DynamicRequest * rqst)
{
  unsigned int status=_GETuint(rqst,"status");
  if (status!=0) { rqst->headerResponse=status; }
  snprintf(rqst->content,rqst->MAXcontentSize,"{\"typed\":true}");
  rqst->contentSize=strlen(rqst->content);
  return 0;
}

static void api_callback(struct AmmServer_APIRequest * rqst,struct AmmServer_APIResponse * res)
{
  long long id=0;
  if (AmmServer_APIRequestMatch(rqst,"/item/%lld",&id))
   {
     AmmServer_APIResponseWriteText(res,"{\"item\":");
     AmmServer_APIResponseWriteInteger(res,id);
     AmmServer_APIResponseWriteText(res,"}");
     return;
   }

  if (strcmp(rqst->path,"/echo")==0)
   {
     unsigned int agentLength=0;
     const char * agent=AmmServer_APIRequestHeader(rqst,"X-Test",&agentLength);
     AmmServer_APIResponseWriteText(res,"{\"query\":\"");
     if (rqst->query!=0) { AmmServer_APIResponseWriteJSONString(res,rqst->query,rqst->queryLength); }
     AmmServer_APIResponseWriteText(res,"\",\"header\":\"");
     if (agent!=0) { AmmServer_APIResponseWriteJSONString(res,agent,agentLength); }
     AmmServer_APIResponseWriteText(res,"\",\"body\":\"");
     if (rqst->body!=0) { AmmServer_APIResponseWriteJSONString(res,rqst->body,rqst->bodyLength); }
     AmmServer_APIResponseWriteText(res,"\"}");
     return;
   }

  res->statusCode=404;
  AmmServer_APIResponseWriteText(res,"{\"error\":\"not found\"}");
}

int main(int argc,char *argv[])
{
  AmmServer_RegisterTerminationSignal(0);

  if ( (argc>2) && (strcmp(argv[1],"-api")==0) )
   {
     const char * ip="127.0.0.1";
     unsigned int port=8080;
     int i=0;
     for (i=3; i+1<argc; i++)
      {
        if (strcmp(argv[i],"-p")==0)    { port=atoi(argv[i+1]); }
        if (strcmp(argv[i],"-bind")==0) { ip=argv[i+1]; }
      }
     unsigned int rings=atoi(argv[2]);
     if (rings==0) { server=AmmServer_StartAPI("testservice",ip,port,0,&api_callback,65536); } else
                   { server=AmmServer_StartAPIOnRings("testservice",ip,port,0,&api_callback,65536,rings); }
     if (!server) { return 1; }
   } else
   {
     server=AmmServer_StartWithArgs("testservice",argc,argv,"127.0.0.1",8080,0,"public_html/","public_html/templates/");
     if (!server) { return 1; }
     AmmServer_AddResourceHandler(server,&cookiePage,"/cookie.html",4096,0,&cookie_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
     AmmServer_AddResourceHandler(server,&sessionPage,"/session.html",4096,0,&session_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
     sessionPage.requestContext.useSessionLifecycle=1;
     AmmServer_AddResourceHandler(server,&echoPage,"/echo.html",4096,0,&echo_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
     AmmServer_AddResourceHandler(server,&typedPage,"/typed",4096,0,&typed_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
     if (!AmmServer_SetResourceHandlerContentType(server,&typedPage,"application/json")) { return 1; }
     AmmServer_AddResourceHandler(server,&nestedPage,"/virtual/folder/echo",4096,0,&echo_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
     //Must be refused ( it would inject a header ) , so /echo.html keeps the type of its extension
     if (AmmServer_SetResourceHandlerContentType(server,&echoPage,"text/plain\r\nX-Injected: 1")) { return 1; }
   }

  while (AmmServer_Running(server)) { usleep(50000); }

  AmmServer_RemoveResourceHandler(server,&cookiePage,1);
  AmmServer_RemoveResourceHandler(server,&sessionPage,1);
  AmmServer_RemoveResourceHandler(server,&echoPage,1);
  AmmServer_RemoveResourceHandler(server,&typedPage,1);
  AmmServer_RemoveResourceHandler(server,&nestedPage,1);
  AmmServer_Stop(server);
  return 0;
}
