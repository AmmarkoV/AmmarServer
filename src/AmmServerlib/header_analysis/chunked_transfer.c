#include "chunked_transfer.h"

#include <string.h>
#include <ctype.h>

/*
   Chunked framing ( RFC 9112 section 7.1 ) :

     chunk-size [ chunk-ext ] CRLF
     chunk-data CRLF
     ...
     0 [ chunk-ext ] CRLF
     trailer-part
     CRLF

   The scanner is incremental : it is called after every recv() with whatever
   bytes have arrived so far.  It never writes , so an incomplete request can
   be re-scanned.  The decoder runs once , after the scanner has said COMPLETE ,
   and compacts the payload down over the framing in place.
*/

static int HexValue(unsigned char c)
{
  if ( (c>='0') && (c<='9') ) { return c-'0'; }
  if ( (c>='a') && (c<='f') ) { return 10+(c-'a'); }
  if ( (c>='A') && (c<='F') ) { return 10+(c-'A'); }
  return -1;
}

/*Consume a chunk-size line ( size + optional extensions + EOL).
  Returns 1=ok , 0=need more data , -1=malformed , -2=too big.*/
static int ParseChunkSizeLine(
                               const char * body,
                               unsigned int bodyLength,
                               unsigned int * position,
                               unsigned long maxDecodedSize,
                               unsigned long decodedSoFar,
                               unsigned long * chunkSize
                             )
{
  unsigned int pos=*position;
  unsigned long size=0;
  unsigned int digits=0;

  while (pos<bodyLength)
   {
     int value=HexValue((unsigned char) body[pos]);
     if (value<0) { break; }
     if (size > ((((unsigned long)-1)-15)/16)) { return -1; }
     size=(size*16)+(unsigned long) value;
     ++digits;
     ++pos;
   }

  if (digits==0) { return -1; }

  //Chunk extensions ( anything between ';' and the end of the line ) are ignored
  while ( (pos<bodyLength) && (body[pos]!='\r') && (body[pos]!='\n') ) { ++pos; }
  if (pos>=bodyLength) { return 0; } //The EOL itself has not arrived yet

  if (body[pos]=='\r')
   {
     if (pos+1>=bodyLength) { return 0; }
     if (body[pos+1]!='\n') { return -1; }
     pos+=2;
   } else
   {
     ++pos; //Bare LF tolerance
   }

  if ( (size>maxDecodedSize) || (decodedSoFar>maxDecodedSize-size) ) { return -2; }

  *position=pos;
  *chunkSize=size;
  return 1;
}

/*Consume the trailer section after the zero chunk.
  Returns 1=ok , 0=need more data , -1=malformed.*/
static int ParseTrailerSection(const char * body,unsigned int bodyLength,unsigned int * position)
{
  unsigned int pos=*position;

  //An empty trailer section is just one blank line ( CRLF or LF )
  if (pos>=bodyLength) { return 0; }
  if (body[pos]=='\r')
   {
     if (pos+1>=bodyLength) { return 0; }
     if (body[pos+1]!='\n') { return -1; }
     *position=pos+2;
     return 1;
   }
  if (body[pos]=='\n') { *position=pos+1; return 1; }

  while (pos<bodyLength)
   {
     unsigned int lineStart=pos;
     while ( (pos<bodyLength) && (body[pos]!='\r') && (body[pos]!='\n') ) { ++pos; }
     if (pos>=bodyLength) { return 0; }

     if (pos==lineStart)
      { //Blank line ends the trailer section
        if (body[pos]=='\r')
         {
           if (pos+1>=bodyLength) { return 0; }
           if (body[pos+1]!='\n') { return -1; }
           pos+=2;
         } else
         { ++pos; }
        *position=pos;
        return 1;
      }

     if (body[pos]=='\r')
      {
        if (pos+1>=bodyLength) { return 0; }
        if (body[pos+1]!='\n') { return -1; }
        pos+=2;
      } else
      { ++pos; }
   }

  return 0;
}

/*Shared walker.  writeBody==0 means scan only ; otherwise decoded bytes are
  copied down ( write pointer never overtakes the read pointer ).*/
static enum HTTPChunkedScanResult WalkChunkedBody(
                                                   char * writeBody,
                                                   const char * body,
                                                   unsigned int bodyLength,
                                                   unsigned long maxDecodedSize,
                                                   unsigned int * decodedSize
                                                 )
{
  unsigned int readPos=0;
  unsigned int writePos=0;
  unsigned long decoded=0;

  for (;;)
   {
     unsigned long chunkSize=0;
     int res=ParseChunkSizeLine(body,bodyLength,&readPos,maxDecodedSize,decoded,&chunkSize);
     if (res==0) { return HTTP_CHUNKED_INCOMPLETE; } else
     if (res==-1) { return HTTP_CHUNKED_ERROR_BAD; } else
     if (res==-2) { return HTTP_CHUNKED_ERROR_TOO_BIG; }

     if (chunkSize==0)
      {
        res=ParseTrailerSection(body,bodyLength,&readPos);
        if (res==0) { return HTTP_CHUNKED_INCOMPLETE; } else
        if (res==-1) { return HTTP_CHUNKED_ERROR_BAD; }

        if (decodedSize!=0) { *decodedSize=(unsigned int) decoded; }
        return HTTP_CHUNKED_COMPLETE;
      }

     //The chunk's data must be fully here before we can move past it
     if ( (bodyLength-readPos)<chunkSize ) { return HTTP_CHUNKED_INCOMPLETE; }

     if (writeBody!=0)
      {
        if (writePos!=(unsigned int) readPos)
         { memmove(writeBody+writePos,body+readPos,(size_t) chunkSize); }
        writePos+=(unsigned int) chunkSize;
      }
     readPos+=(unsigned int) chunkSize;
     decoded+=chunkSize;

     //CRLF after the chunk data
     if (readPos>=bodyLength) { return HTTP_CHUNKED_INCOMPLETE; }
     if (body[readPos]=='\r')
      {
        if (readPos+1>=bodyLength) { return HTTP_CHUNKED_INCOMPLETE; }
        if (body[readPos+1]!='\n') { return HTTP_CHUNKED_ERROR_BAD; }
        readPos+=2;
      } else
     if (body[readPos]=='\n') { ++readPos; } else
                              { return HTTP_CHUNKED_ERROR_BAD; }
   }
}

enum HTTPChunkedScanResult HTTPChunkedBodyScan(
                                                const char * body,
                                                unsigned int bodyLength,
                                                unsigned long maxDecodedSize,
                                                unsigned int * decodedSize
                                              )
{
  if (body==0) { return HTTP_CHUNKED_ERROR_BAD; }
  return WalkChunkedBody(0,body,bodyLength,maxDecodedSize,decodedSize);
}

enum HTTPChunkedScanResult HTTPChunkedBodyDecode(
                                                  char * body,
                                                  unsigned int bodyLength,
                                                  unsigned long maxDecodedSize,
                                                  unsigned int * decodedSize
                                                )
{
  if (body==0) { return HTTP_CHUNKED_ERROR_BAD; }
  return WalkChunkedBody(body,body,bodyLength,maxDecodedSize,decodedSize);
}
