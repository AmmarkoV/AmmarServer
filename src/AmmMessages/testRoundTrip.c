//Compiled by testAmmMessages.sh against a freshly generated person.h ( its mmap path points into the test's temp directory )
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "InputParser_C.h"
#include "person.h"

static unsigned int failures=0;
#define CHECK(cond) do { if (!(cond)) { fprintf(stderr,"FAIL %s:%d : %s\n",__FILE__,__LINE__,#cond); ++failures; } } while(0)

int main(int argc,char * argv[])
{
  if (argc<2) { return 2; }
  const char * tempDir = argv[1];

  //Pack , wrap in an HTTP response like /view/person.html returns , unpack into another message
  struct personMessage sent; memset(&sent,0,sizeof(sent));
  sent.confidence=0.75f; sent.source=-5; sent.inFieldOfView=1; sent.x=-1.5f; sent.y=2.25f; sent.z=0.0f; sent.theta=3.0f; sent.timestamp=-123456;
  char packed[HTTP_MESSAGE_SIZE];
  packToHTTPGETRequest_person(packed,sizeof(packed),&sent);
  char response[HTTP_MESSAGE_SIZE+256];
  //'=' in a header used to shift every field , the body was never found because only \n\n was looked for
  int length = snprintf(response,sizeof(response),"HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: %zu\r\n\r\n%s",strlen(packed),packed);

  struct personMessage received; memset(&received,0,sizeof(received));
  memset(&personStatic,0,sizeof(personStatic));
  CHECK(unpackFromHTTPGETRequest_person(&received,response,(unsigned int) length));
  CHECK(received.source==-5);           //Signed , used to come back as 4294967291
  CHECK(received.timestamp==-123456);
  CHECK(received.inFieldOfView==1);
  CHECK( (received.x>-1.51f) && (received.x<-1.49f) );
  CHECK( (received.confidence>0.74f) && (received.confidence<0.76f) );
  CHECK(personStatic.source==0);        //unpack used to write the global instead of the message it was given

  //mmap bridge : write , read back
  struct personMessage fromBridge; memset(&fromBridge,0,sizeof(fromBridge));
  CHECK(initializeForWriting_person());
  CHECK(write_person(&personBridge,&sent,0));
  struct bridgeContext reader;
  CHECK(initializeReadingBridge(&reader,pathToMMAPperson,sizeof(struct personMessage)));
  CHECK(readBridge(&reader,&fromBridge,sizeof(fromBridge)));
  CHECK( (fromBridge.source==-5) && (fromBridge.timestamp==-123456) );
  CHECK(!readBridge(&reader,&fromBridge,sizeof(fromBridge)*4)); //Larger than the mapping , was unchecked
  CHECK(closeReadingBridge(&reader));
  CHECK(closeWritingBridge(&personBridge));

  //A file smaller than the message used to be mapped at full size and read : SIGBUS
  char smallPath[512]; snprintf(smallPath,sizeof(smallPath),"%s/small.mmap",tempDir);
  int fd = open(smallPath,O_WRONLY|O_CREAT|O_TRUNC,0600); if (write(fd,"x",1)!=1) { } close(fd);
  CHECK(!initializeReadingBridge(&reader,smallPath,sizeof(struct personMessage)));
  CHECK(!initializeReadingBridge(&reader,"/nonexistent/bridge.mmap",sizeof(struct personMessage)));

  //Closing a bridge that never opened used to munmap(0) and close(0) , which is stdin
  struct bridgeContext never; memset(&never,0,sizeof(never));
  CHECK(!closeReadingBridge(&never));
  CHECK(!closeWritingBridge(&never));
  CHECK(fcntl(0,F_GETFD)!=-1);
  CHECK(!closeReadingBridge(0));

  //strstrDoubleNewline finds \r\n\r\n and \n\n , and returns the whole buffer when there is neither
  char a[]="HEAD\r\n\r\nBODY"; unsigned int end=0;
  CHECK(strcmp(strstrDoubleNewline(a,strlen(a),&end),"BODY")==0 && (end==4) && (strcmp(a,"HEAD")==0));
  char b[]="HEAD\n\nBODY";
  CHECK(strcmp(strstrDoubleNewline(b,strlen(b),&end),"BODY")==0 && (end==4));
  char c[]="no separator here";
  CHECK(strstrDoubleNewline(c,strlen(c),&end)==c);

  if (failures) { fprintf(stderr,"AmmMessages round trip : %u failures\n",failures); return 1; }
  return 0;
}
