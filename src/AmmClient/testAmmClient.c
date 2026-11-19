//Regression tests for AmmClient , against a small forked HTTP server on a free loopback port
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "AmmClient.h"

static unsigned int failures=0;
#define CHECK(cond) do { if (!(cond)) { fprintf(stderr,"FAIL %s:%d : %s\n",__FILE__,__LINE__,#cond); ++failures; } } while(0)

static unsigned int openFileDescriptors()
{
  unsigned int count=0;
  DIR * d = opendir("/proc/self/fd");
  if (d==0) { return 0; }
  while (readdir(d)!=0) { ++count; }
  closedir(d);
  return count;
}

//Serves one connection : reads the request ( saved to requestOut ) , sends response in two pieces ( split at
//splitAt bytes , 50ms apart ) and then holds the connection open for holdMs before closing it
struct Server { pid_t pid; int port; int requestPipe; };

static struct Server startServer(const char * response,size_t length,size_t splitAt,unsigned int holdMs)
{
  struct Server server={0};
  int listener = socket(AF_INET,SOCK_STREAM,0);
  int yes=1; setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));
  struct sockaddr_in address; memset(&address,0,sizeof(address));
  address.sin_family=AF_INET; address.sin_addr.s_addr=htonl(INADDR_LOOPBACK); address.sin_port=0;
  bind(listener,(struct sockaddr*)&address,sizeof(address));
  listen(listener,4);
  socklen_t addressLength=sizeof(address);
  getsockname(listener,(struct sockaddr*)&address,&addressLength);
  server.port = ntohs(address.sin_port);

  int pipeFds[2]; if (pipe(pipeFds)!=0) { perror("pipe"); exit(2); }
  server.pid = fork();
  if (server.pid==0)
  {
    close(pipeFds[0]);
    int client = accept(listener,0,0);
    char request[4096]; size_t got=0;
    while (got<sizeof(request)-1)
    {
      ssize_t r = recv(client,request+got,sizeof(request)-1-got,0);
      if (r<=0) { break; }
      got+=r; request[got]=0;
      if (strstr(request,"\r\n\r\n")) { usleep(20000); r=recv(client,request+got,sizeof(request)-1-got,MSG_DONTWAIT); if (r>0) { got+=r; } break; }
    }
    if (write(pipeFds[1],request,got)<0) { }
    close(pipeFds[1]);
    if (splitAt>length) { splitAt=length; }
    if (send(client,response,splitAt,MSG_NOSIGNAL)<0) { }
    usleep(50000);
    if (send(client,response+splitAt,length-splitAt,MSG_NOSIGNAL)<0) { }
    usleep(holdMs*1000);
    close(client);
    _exit(0);
  }
  close(pipeFds[1]);
  close(listener);
  server.requestPipe = pipeFds[0];
  return server;
}

static void stopServer(struct Server * server,char * requestOut,size_t requestOutSize)
{
  size_t got=0; ssize_t r;
  while ( (got<requestOutSize-1) && ((r=read(server->requestPipe,requestOut+got,requestOutSize-1-got))>0) ) { got+=r; }
  requestOut[got]=0;
  close(server->requestPipe);
  kill(server->pid,SIGKILL);
  waitpid(server->pid,0,0);
}

//Downloads URI from a server sending response , into a buffer of bufferSize bytes prefilled with junk
static int download(const char * response,size_t splitAt,unsigned int holdMs,unsigned int bufferSize,int keepAlive,int fast,
                    char ** body,unsigned int * bodySize,char * request,size_t requestSize)
{
  struct Server server = startServer(response,strlen(response),splitAt,holdMs);
  struct AmmClient_Instance * client = AmmClient_Initialize("127.0.0.1",server.port,2);
  char * buffer = (char*) malloc(bufferSize); //Exactly bufferSize , so reading past it is caught by AddressSanitizer
  memset(buffer,'J',bufferSize);             //No terminator anywhere , like a buffer reused from an earlier response
  buffer[bufferSize-1]=0;
  unsigned int size = bufferSize;
  int ok = AmmClient_RecvFile(client,"/file.txt",buffer,&size,keepAlive,fast);
  AmmClient_Close(client); //First , a server still waiting for a request that was never sent then sees the close
  stopServer(&server,request,requestSize);
  *body = 0; *bodySize = 0;
  static char copy[8192];
  if (ok)
  {
    unsigned int s = size;
    char * start = AmmClient_seekEndOfHeader(buffer,&s);
    if (start) { memcpy(copy,start,s); copy[s]=0; *body=copy; *bodySize=s; }
  }
  free(buffer);
  return ok;
}

//The header size used to be measured up to the Content-Length value , so headers after it made a download stop early
static void test_headersAfterContentLength()
{
  const char * response = "HTTP/1.1 200 OK\r\nContent-Length: 26\r\nServer: Test\r\nContent-Type: text/plain\r\nX-Padding: aaaaaaaaaaaaaaaaaaaaaaa\r\n\r\nabcdefghijklmnopqrstuvwxyz";
  char * body; unsigned int bodySize; char request[4096];
  int ok = download(response,strlen(response)-20,0,4096,1,0,&body,&bodySize,request,sizeof(request));
  CHECK(ok);
  CHECK( (body!=0) && (bodySize==26) && (strcmp(body,"abcdefghijklmnopqrstuvwxyz")==0) );
}

//Header names are case-insensitive , and Content-Length: 0 is a complete response
static void test_lowercaseAndEmpty()
{
  char * body; unsigned int bodySize; char request[4096];
  int ok = download("HTTP/1.1 200 OK\r\ncontent-length: 5\r\n\r\nhello",10,1500,4096,1,0,&body,&bodySize,request,sizeof(request));
  CHECK(ok);
  CHECK( (body!=0) && (bodySize==5) && (memcmp(body,"hello",5)==0) );
  ok = download("HTTP/1.1 204 No Content\r\nContent-Length: 0\r\n\r\n",10,1500,4096,1,0,&body,&bodySize,request,sizeof(request));
  CHECK(ok);
}

//The buffer was never terminated after recv() , so strstr() read old bytes past the data ( and past the buffer when full )
static void test_unterminatedBuffer()
{
  char * body; unsigned int bodySize; char request[4096];
  char response[700];
  memset(response,'x',sizeof(response)); response[sizeof(response)-1]=0;
  memcpy(response,"HTTP/1.1 200 OK\r\nX:",19); //No Content-Length , the server just closes
  download(response,300,0,200,0,0,&body,&bodySize,request,sizeof(request)); //Bigger than the buffer : must stop cleanly
  int ok = download("HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nabc",20,0,100,1,1,&body,&bodySize,request,sizeof(request)); //Request just fits
  (void) ok; //The "fast" mode gives up after 1ms without data , so only memory safety is checked here
  ok = download("HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nabc",20,0,40,1,0,&body,&bodySize,request,sizeof(request));
  CHECK(!ok); //Too small for the request itself , refused without sending
  CHECK(request[0]==0);
}

//A request without keep-alive has to say Connection: close , and the headers end with exactly one blank line
static void test_requestFormat()
{
  char * body; unsigned int bodySize; char request[4096];
  download("HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nok",10,0,4096,0,0,&body,&bodySize,request,sizeof(request));
  CHECK(strstr(request,"Connection: close\r\n")!=0);
  size_t length = strlen(request);
  CHECK( (length>=4) && (strcmp(request+length-4,"\r\n\r\n")==0) && ( (length<6) || (strncmp(request+length-6,"\r\n\r\n\r\n",6)!=0) ) );
  download("HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nok",10,0,4096,1,0,&body,&bodySize,request,sizeof(request));
  CHECK(strstr(request,"Connection: keep-alive\r\n")!=0);
}

//Every reconnection attempt created a new socket without closing the previous one , and Close() never closed it
static void test_noDescriptorLeaks()
{
  int probe = socket(AF_INET,SOCK_STREAM,0);
  struct sockaddr_in address; memset(&address,0,sizeof(address));
  address.sin_family=AF_INET; address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
  bind(probe,(struct sockaddr*)&address,sizeof(address));
  socklen_t addressLength=sizeof(address); getsockname(probe,(struct sockaddr*)&address,&addressLength);
  int closedPort = ntohs(address.sin_port);
  close(probe); //Nothing listens here now

  unsigned int before = openFileDescriptors();
  struct AmmClient_Instance * client = AmmClient_Initialize("127.0.0.1",closedPort,1);
  CHECK(client!=0);
  unsigned int i=0;
  for (i=0; i<50; i++) { CHECK(!AmmClient_CheckConnection(client)); }
  CHECK(openFileDescriptors()<=before+1);
  AmmClient_Close(client);
  CHECK(openFileDescriptors()==before);

  struct Server server = startServer("HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n",38,10,0);
  before = openFileDescriptors();
  client = AmmClient_Initialize("127.0.0.1",server.port,1);
  CHECK(AmmClient_CheckConnection(client));
  AmmClient_Close(client);
  CHECK(openFileDescriptors()==before);
  char request[64]; stopServer(&server,request,sizeof(request));
}

//Microseconds and milliseconds shared one base counter stored in different units
static void test_tickCounts()
{
  unsigned long ms = AmmClient_GetTickCountMilliseconds();
  unsigned long us = AmmClient_GetTickCountMicroseconds();
  CHECK(ms<10000);
  CHECK(us<10000000);
  usleep(20000);
  unsigned long us2 = AmmClient_GetTickCountMicroseconds();
  unsigned long ms2 = AmmClient_GetTickCountMilliseconds();
  CHECK( (us2>us) && (us2-us>=15000) && (us2-us<5000000) );
  CHECK( (ms2>=ms) && (ms2-ms<5000) );
}

//Form and file names too long for the stack buffers used to make SendFile send whatever followed them
static void test_sendFileLimits()
{
  struct Server server = startServer("HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n",38,10,0);
  struct AmmClient_Instance * client = AmmClient_Initialize("127.0.0.1",server.port,1);
  char hugeName[6000]; memset(hugeName,'n',sizeof(hugeName)-1); hugeName[sizeof(hugeName)-1]=0;
  CHECK(!AmmClient_SendFile(client,"/upload.html","file",hugeName,"text/plain","abc",3,0));
  AmmClient_Close(client);
  char request[64]; stopServer(&server,request,sizeof(request));
  CHECK(AmmClient_Close(0)==0);
}

int main()
{
  signal(SIGPIPE,SIG_IGN);
  test_tickCounts(); //First , while the tick bases are still unset
  test_headersAfterContentLength();
  test_lowercaseAndEmpty();
  test_unterminatedBuffer();
  test_requestFormat();
  test_noDescriptorLeaks();
  test_sendFileLimits();

  if (failures) { fprintf(stderr,"AmmClient tests : %u failures\n",failures); return 1; }
  fprintf(stderr,"AmmClient tests : all passed\n");
  return 0;
}
