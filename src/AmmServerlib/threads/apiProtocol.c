/*
AmmarServer , HTTP Server Library

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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <strings.h>

#include "apiProtocol.h"

#include "../server_configuration.h"
#include "../tools/http_tools.h"
#include "../tools/logs.h"

int ApiProtocol_AllocateResponseBuffer(struct AmmServer_Instance * instance,struct ApiResponseBuffer * buffer)
{
  buffer->bodySize = instance->apiMaxResponseSize;
  if (buffer->bodySize==0) { buffer->bodySize = API_DEFAULT_MAX_RESPONSE_SIZE; }

  buffer->memory = (char*) malloc(API_RESPONSE_HEADER_RESERVE+buffer->bodySize+1);
  if (buffer->memory==0) { errorID(ASV_ERROR_COULD_NOT_ALLOCATE_MEMORY); return 0; }

  return 1;
}

const char * AmmServer_APIRequestHeader(const struct AmmServer_APIRequest * rqst,const char * headerName,unsigned int * valueLength)
{
  *valueLength=0;
  if ( (rqst==0) || (rqst->headers==0) || (headerName==0) ) { return 0; }

  unsigned int nameLength = strlen(headerName);
  const char * line = rqst->headers;
  const char * end  = rqst->headers + rqst->headersLength;

  while (line<end)
   {
     const char * lineEnd = (const char *) memchr(line,'\n',(size_t)(end-line));
     if (lineEnd==0) { lineEnd=end; }

     //The name has to start at the line start , which is also what keeps a request target like
     ///Content-Length:5 on the request line from ever being mistaken for a header.
     if ( ((unsigned int)(lineEnd-line) > nameLength) && (line[nameLength]==':') &&
          (strncasecmp(line,headerName,nameLength)==0) )
      {
        const char * value = line+nameLength+1;
        while ( (value<lineEnd) && ((*value==' ')||(*value=='\t')) ) { ++value; }

        const char * valueEnd = lineEnd;
        while ( (valueEnd>value) && ((valueEnd[-1]=='\r')||(valueEnd[-1]==' ')||(valueEnd[-1]=='\t')) ) { --valueEnd; }

        *valueLength = (unsigned int)(valueEnd-value);
        return value;
      }

     if (lineEnd==end) { break; }
     line = lineEnd+1;
   }

  return 0;
}

//Length of the method token is always handed back , even for a method this does not recognize by name , so the
//request target is found in the same place either way and the handler is the one that decides what an
//unsupported method deserves.
/* ------------------------------------------------------------------ reading a request ---------------- */

const char * AmmServer_APIRequestBearer(const struct AmmServer_APIRequest * rqst,unsigned int * tokenLength)
{
  if (tokenLength!=0) { *tokenLength = 0; }

  unsigned int headerLength = 0;
  const char * header = AmmServer_APIRequestHeader(rqst,"Authorization",&headerLength);
  if (header==0) { return 0; }

  //"Bearer" is case insensitive as a scheme name , and exactly one space after it is what every client
  //sends , but any run of spaces is allowed before the token.
  const char * scheme = "bearer";
  if (headerLength<6) { return 0; }

  unsigned int i=0;
  for (i=0; i<6; i++)
   {
     char c = header[i];
     if ( (c>='A') && (c<='Z') ) { c = (char)(c - 'A' + 'a'); }
     if (c!=scheme[i]) { return 0; }
   }

  if ( (headerLength==6) || (header[6]!=' ') ) { return 0; }  //"Bearertoken" is not a bearer token

  unsigned int at=6;
  while ( (at<headerLength) && (header[at]==' ') ) { ++at; }

  //"Bearer" with nothing after it is a bearer header with an empty token , which is not the same thing as
  //a request that carried no token at all : one is a malformed credential and the other is a missing one ,
  //and they are usually different answers. So this comes back non-zero with length 0 , as an absent
  //parameter never does.
  if (at>=headerLength) { if (tokenLength!=0) { *tokenLength = 0; } return header+headerLength; }

  //Trailing whitespace is not part of the token , and a token with a space inside it is not a token.
  unsigned int end=at;
  while ( (end<headerLength) && (header[end]!=' ') && (header[end]!='\t') ) { ++end; }

  if (tokenLength!=0) { *tokenLength = end-at; }
  return header+at;
}

const char * AmmServer_APIRequestQueryParam(const struct AmmServer_APIRequest * rqst,const char * name,unsigned int * valueLength)
{
  if (valueLength!=0) { *valueLength = 0; }
  if ( (rqst==0) || (name==0) || (rqst->query==0) ) { return 0; }

  unsigned int nameLength = (unsigned int) strlen(name);
  if (nameLength==0) { return 0; }

  unsigned int at=0;
  while (at < rqst->queryLength)
   {
     //One name=value pair , ending at '&' or at the end of the query string.
     unsigned int pairEnd=at;
     while ( (pairEnd<rqst->queryLength) && (rqst->query[pairEnd]!='&') ) { ++pairEnd; }

     unsigned int equals=at;
     while ( (equals<pairEnd) && (rqst->query[equals]!='=') ) { ++equals; }

     if ( (equals-at==nameLength) && (memcmp(rqst->query+at,name,nameLength)==0) )
      {
        //A name with no '=' at all is present but valueless , which is not the same as absent : the value
        //comes back empty and non-zero so that "?debug" can be told from no debug.
        const char * value = rqst->query + ( (equals<pairEnd) ? equals+1 : pairEnd );
        if (valueLength!=0) { *valueLength = (unsigned int)( (rqst->query+pairEnd) - value ); }
        return value;
      }

     at = (pairEnd<rqst->queryLength) ? pairEnd+1 : pairEnd;
   }

  return 0;
}

//One %lld conversion : a run of decimal digits that is a positive number and cannot overflow. Deliberately
//strict , because a path is not a scanf format - "1.5" , "-1" , "0" , "1 " and "" are all a bad request
//rather than something to take the leading digits of.
static int MatchPositiveInteger(const char * s,unsigned int length,long long * out)
{
  if ( (length==0) || (length>18) ) { return 0; }  //18 digits can never overflow a long long

  long long value=0;
  unsigned int i=0;
  for (i=0; i<length; i++)
   {
     if ( (s[i]<'0') || (s[i]>'9') ) { return 0; }
     value = (value*10) + (s[i]-'0');
   }

  if (value<=0) { return 0; }
  *out = value;
  return 1;
}

int AmmServer_APIRequestMatch(const struct AmmServer_APIRequest * rqst,const char * pathTemplate,...)
{
  if ( (rqst==0) || (rqst->path==0) || (pathTemplate==0) ) { return 0; }

  va_list captures;
  va_start(captures,pathTemplate);

  const char * path  = rqst->path;
  unsigned int at    = 0;
  unsigned int tAt   = 0;
  unsigned int tLength = (unsigned int) strlen(pathTemplate);
  int matched = 1;

  while (tAt < tLength)
   {
     if ( (pathTemplate[tAt]=='%') && (tAt+1<tLength) )
      {
        //How far the capture runs : up to whatever literal character follows it in the template , or to the
        //end of the path. That is what makes "/posts/%lld/like" able to tell the id from the suffix.
        unsigned int conversion = 0;
        if      ( (tAt+3<tLength) && (memcmp(pathTemplate+tAt,"%lld",4)==0) ) { conversion=4; }
        else if ( (pathTemplate[tAt+1]=='s') )                               { conversion=2; }
        else { matched=0; break; }  //an unsupported conversion never matches rather than matching loosely

        char stopAt = ( (tAt+conversion) < tLength ) ? pathTemplate[tAt+conversion] : 0;

        unsigned int runEnd = at;
        while ( (path[runEnd]!=0) && (path[runEnd]!=stopAt) && (path[runEnd]!='/') ) { ++runEnd; }
        //A %s or %lld is one path segment , so it never swallows a '/' - unless a '/' is what follows it in
        //the template , where stopAt is that '/' and the loop above has already ended on it.

        if (conversion==4)
         {
           long long * into = va_arg(captures,long long *);
           long long value=0;
           if (!MatchPositiveInteger(path+at,runEnd-at,&value)) { matched=0; break; }
           if (into!=0) { *into = value; }
         } else
         {
           const char ** intoText = va_arg(captures,const char **);
           unsigned int * intoLength = va_arg(captures,unsigned int *);
           if (runEnd==at) { matched=0; break; }  //an empty segment is not a match
           if (intoText!=0)   { *intoText = path+at; }
           if (intoLength!=0) { *intoLength = runEnd-at; }
         }

        at  = runEnd;
        tAt += conversion;
        continue;
      }

     if (path[at]!=pathTemplate[tAt]) { matched=0; break; }
     ++at;
     ++tAt;
   }

  va_end(captures);

  //Anchored at both ends : a template is the whole path or it is not a match.
  if (matched && (path[at]!=0)) { matched=0; }

  return matched;
}

/* ------------------------------------------------------------------ writing a response -------------- */

void AmmServer_APIResponseReset(struct AmmServer_APIResponse * res,unsigned int statusCode,const char * contentType)
{
  if (res==0) { return; }

  res->statusCode  = statusCode;
  if (contentType!=0) { res->contentType = contentType; }
  res->bodyLength  = 0;
  res->overflowed  = 0;
}

int AmmServer_APIResponseWrite(struct AmmServer_APIResponse * res,const char * text,unsigned int length)
{
  if (res==0) { return 0; }
  if (res->overflowed) { return 0; }
  if (length==0) { return 1; }
  if ( (text==0) || (res->body==0) ) { res->overflowed=1; return 0; }

  //The one check that makes the response buffer safe to write into : once it has failed it stays failed ,
  //so a handler can write a whole response without testing every step and still never overrun.
  if (res->bodyLength+length > res->maxBodyLength) { res->overflowed=1; return 0; }

  memcpy(res->body+res->bodyLength,text,length);
  res->bodyLength += length;
  return 1;
}

int AmmServer_APIResponseWriteText(struct AmmServer_APIResponse * res,const char * text)
{
  if (text==0) { return 1; }
  return AmmServer_APIResponseWrite(res,text,(unsigned int) strlen(text));
}

int AmmServer_APIResponseWriteInteger(struct AmmServer_APIResponse * res,long long value)
{
  char digits[24];
  int at = (int) sizeof(digits);
  int negative = (value<0);
  unsigned long long magnitude = negative ? (unsigned long long)(-(value+1))+1ULL : (unsigned long long) value;

  if (magnitude==0) { digits[--at]='0'; }
  while (magnitude>0) { digits[--at] = (char)('0'+(magnitude%10)); magnitude/=10; }
  if (negative) { digits[--at]='-'; }

  return AmmServer_APIResponseWrite(res,digits+at,(unsigned int)((int) sizeof(digits)-at));
}

int AmmServer_APIResponseWriteJSONString(struct AmmServer_APIResponse * res,const char * text,unsigned int length)
{
  if (res==0) { return 0; }
  if (res->overflowed) { return 0; }
  if (length==0) { return 1; }
  if ( (text==0) || (res->body==0) ) { res->overflowed=1; return 0; }

  //Escaped straight into the response buffer rather than through anything of its own , so text of any size
  //costs no copy and no scratch space. A value that will not fit comes back as an overflow , like any other.
  unsigned int written = AmmServer_JSONEscape(text,length,res->body+res->bodyLength,res->maxBodyLength-res->bodyLength);
  if (written==0) { res->overflowed=1; return 0; }

  res->bodyLength += written;
  return 1;
}

int AmmServer_APIResponseOverflowed(const struct AmmServer_APIResponse * res)
{
  if (res==0) { return 1; }
  return (res->overflowed!=0);
}

static int MethodFromRequestLine(const char * line,unsigned int length,unsigned int * methodLength)
{
  const char * space = (const char *) memchr(line,' ',length);
  *methodLength = (space!=0) ? (unsigned int)(space-line) : 0;

  switch (*methodLength)
   {
     case 3 : if (memcmp(line,"GET",3)==0)    { return GET;    }
              if (memcmp(line,"PUT",3)==0)    { return PUT;    }
     break;
     case 4 : if (memcmp(line,"POST",4)==0)   { return POST;   }
              if (memcmp(line,"HEAD",4)==0)   { return HEAD;   }
     break;
     case 6 : if (memcmp(line,"DELETE",6)==0) { return DELETE; }
     break;
   };

  return NONE;
}

//Where the header block ends , and how long its terminator was. Tolerates a bare LF between lines the same way
//find_header_terminator ( epollFastPathServer.c ) does , since real clients occasionally send it.
static int FindRequestHeaderEnd(const char * buf,unsigned int length,unsigned int * headerEnd)
{
  unsigned int i=0;
  for (i=0; i+1<length; i++)
   {
     if ( (buf[i]=='\n') && (buf[i+1]=='\n') )  { *headerEnd=i+2; return 1; }
     if ( (i+3<length) && (buf[i]=='\r') && (buf[i+1]=='\n') && (buf[i+2]=='\r') && (buf[i+3]=='\n') ) { *headerEnd=i+4; return 1; }
   }
  return 0;
}

enum ApiFrameResult ApiProtocol_NextFrame(char * buf,unsigned int used,unsigned int offset,struct ApiFrame * frame)
{
  memset(frame,0,sizeof(struct ApiFrame));

  if (offset>=used) { return API_FRAME_INCOMPLETE; }

  char * request = buf+offset;
  unsigned int available = used-offset;

  unsigned int headerEnd=0;
  if (!FindRequestHeaderEnd(request,available,&headerEnd)) { return API_FRAME_INCOMPLETE; }

  frame->request   = request;
  frame->headerEnd = headerEnd;

  struct AmmServer_APIRequest probe={0};
  probe.headers = request;
  probe.headersLength = headerEnd;
  unsigned int valueLength=0;

  //Content-Length is what says where this request ends and the next one begins. Guessing wrong about that
  //would desynchronize every request behind it on this connection , so a framing this cannot work out
  //exactly is refused instead ( and a REST API has no use for a chunked request body ).
  const char * transferEncoding = AmmServer_APIRequestHeader(&probe,"Transfer-Encoding",&valueLength);
  if ( (transferEncoding!=0) && (valueLength>0) && (strncasecmp(transferEncoding,"identity",8)!=0) )
   {
     frame->refuseStatus = 501;
     return API_FRAME_REFUSED;
   }

  unsigned long bodyLength=0;
  const char * contentLength = AmmServer_APIRequestHeader(&probe,"Content-Length",&valueLength);
  if ( (contentLength!=0) && (valueLength>0) )
   {
     unsigned int i=0;
     for (i=0; i<valueLength; i++)
      {
        if ( (contentLength[i]<'0') || (contentLength[i]>'9') )
         {
           frame->refuseStatus = 400;
           return API_FRAME_REFUSED;
         }
        bodyLength = (bodyLength*10) + (unsigned long)(contentLength[i]-'0');
        if (bodyLength>API_MAX_REQUEST_SIZE)
         {
           frame->refuseStatus = 413;
           return API_FRAME_REFUSED;
         }
      }
   }

  if (available < headerEnd+bodyLength) { return API_FRAME_INCOMPLETE; } //body still arriving

  frame->bodyLength  = (unsigned int) bodyLength;
  frame->totalLength = headerEnd + (unsigned int) bodyLength;

  //Keep-alive is HTTP/1.1's default ; HTTP/1.0 only gets it if it asks.
  frame->keepAlive = 1;
  if (memmem(request,headerEnd,"HTTP/1.0",8)!=0) { frame->keepAlive=0; }

  const char * connectionHeader = AmmServer_APIRequestHeader(&probe,"Connection",&valueLength);
  if ( (connectionHeader!=0) && (valueLength>0) )
   {
     if (strncasecmp(connectionHeader,"close",5)==0)       { frame->keepAlive=0; } else
     if (strncasecmp(connectionHeader,"keep-alive",10)==0) { frame->keepAlive=1; }
   }

  return API_FRAME_READY;
}

/**
* @brief Make sure a partial request buffer can hold at least `needed` bytes , doubling it rather than
*        reallocating by the byte.
* @retval 1=it can , 0=it cannot ( needed is past the request cap , or out of memory ) */
static int EnsurePartialCapacity(struct ApiPartialRequest * partial,unsigned int needed)
{
  if (needed>API_MAX_REQUEST_SIZE)  { return 0; }
  if (partial->size>=needed)        { return 1; }

  unsigned int size = (partial->size>0) ? partial->size : API_INITIAL_BUFFER_SIZE;
  while (size<needed) { size*=2; }
  if (size>API_MAX_REQUEST_SIZE) { size=API_MAX_REQUEST_SIZE; }

  char * grown = (char*) realloc(partial->data,size+1);
  if (grown==0) { errorID(ASV_ERROR_COULD_NOT_ALLOCATE_MEMORY); return 0; }

  partial->data = grown;
  partial->size = size;
  return 1;
}

int ApiProtocol_PartialReserve(struct ApiPartialRequest * partial,unsigned int needed)
{
  return EnsurePartialCapacity(partial,needed);
}

void ApiProtocol_PartialFree(struct ApiPartialRequest * partial)
{
  if (partial->data!=0) { free(partial->data); partial->data=0; }
  partial->size=0;
  partial->used=0;
}

int ApiProtocol_PartialKeepTail(struct ApiPartialRequest * partial,const char * buf,unsigned int used,unsigned int consumed)
{
  unsigned int leftover = used-consumed;

  if (leftover==0)
   {
     ApiProtocol_PartialFree(partial);
     return 1;
   }

  //When buf is already the partial buffer , leftover can never exceed what is allocated , so this cannot
  //realloc and cannot move the block out from under the memmove below. It only allocates for the other case.
  if (!EnsurePartialCapacity(partial,leftover)) { return 0; }

  memmove(partial->data,buf+consumed,leftover); //source and destination are the same buffer in the one case
  partial->used = leftover;
  return 1;
}

int ApiProtocol_PartialAppend(struct ApiPartialRequest * partial,const char * from,unsigned int length)
{
  if (length==0) { return 1; }

  if (!EnsurePartialCapacity(partial,partial->used+length)) { return 0; }

  memcpy(partial->data+partial->used,from,length);
  partial->used += length;
  return 1;
}

unsigned int ApiProtocol_StatusOnlyResponse(unsigned int statusCode,char * into,unsigned int intoSize)
{
  int length = snprintf(into,intoSize,
                        "HTTP/1.1 %u %s\r\nContent-Length: 0\r\nConnection: close\r\n\r\n",
                        statusCode,GetHTTPReasonPhrase(statusCode));

  if (length<0) { return 0; }
  return (unsigned int) length;
}

unsigned int ApiProtocol_Serve(
                                struct AmmServer_Instance * instance,
                                struct ApiFrame * frame,
                                struct ApiResponseBuffer * buffer,
                                char ** response,
                                unsigned int * failStatus
                              )
{
  *response = 0;
  *failStatus = 400; //a request line this cannot even split ; the 500 below is the other way out of here

  char * request = frame->request;
  unsigned int headerEnd = frame->headerEnd;

  char * lineEnd = (char *) memchr(request,'\n',headerEnd);
  if (lineEnd==0) { return 0; }  //unreachable while NextFrame found a header terminator , kept for safety

  unsigned int methodLength=0;
  int method = MethodFromRequestLine(request,(unsigned int)(lineEnd-request),&methodLength);

  //Request line : METHOD SP target SP HTTP/x.y
  char * target = request + ( (methodLength>0) ? methodLength+1 : 0 );
  if (target>lineEnd) { return 0; }

  char * targetEnd = (char *) memchr(target,' ',(size_t)(lineEnd-target));
  if (targetEnd==0)
   {
     targetEnd=lineEnd; //No version token ( HTTP/0.9 style )
     while ( (targetEnd>target) && ((targetEnd[-1]=='\r')||(targetEnd[-1]==' ')) ) { --targetEnd; }
   }

  struct AmmServer_APIRequest rqst={0};
  rqst.method = method;

  //NUL terminating the path ( and query ) in place , over the request line's own delimiters. Nothing else in
  //the buffer can be terminated this cheaply : a NUL after the header block would land on the first body byte ,
  //so the handler gets headers as a pointer plus a length instead.
  char * queryStart = (char *) memchr(target,'?',(size_t)(targetEnd-target));
  *targetEnd=0;
  if (queryStart!=0)
   {
     *queryStart=0;
     rqst.query = queryStart+1;
     rqst.queryLength = (unsigned int)(targetEnd-(queryStart+1));
   }
  rqst.path = target;
  rqst.pathLength = strlen(target);

  char * headersStart = lineEnd+1;
  unsigned int headersLength = 0;
  if (headerEnd>(unsigned int)(headersStart-request))
   {
     headersLength = headerEnd - (unsigned int)(headersStart-request);
     while ( (headersLength>0) && ((headersStart[headersLength-1]=='\n')||(headersStart[headersLength-1]=='\r')) ) { --headersLength; }
   }
  rqst.headers = headersStart;
  rqst.headersLength = headersLength;

  rqst.body = (frame->bodyLength>0) ? request+headerEnd : 0;
  rqst.bodyLength = frame->bodyLength;

  struct AmmServer_APIResponse responseToFill={0};
  responseToFill.statusCode = 200;
  responseToFill.contentType = "application/json";
  responseToFill.body = buffer->memory + API_RESPONSE_HEADER_RESERVE;
  responseToFill.bodyLength = 0;
  responseToFill.maxBodyLength = buffer->bodySize;

  void ( *DoCallback) (struct AmmServer_APIRequest *,struct AmmServer_APIResponse *) = 0;
  DoCallback = instance->apiHandler;
  DoCallback(&rqst,&responseToFill);

  if (responseToFill.bodyLength>buffer->bodySize)
   {
     //The handler has already overrun the buffer it was given , so the damage is done ; all this can do is
     //refuse to compound it by sending whatever is in the memory past it as if it were a response.
     errorID(ASV_ERROR_CLIENT_CAUSED_AN_OVERFLOW);
     fprintf(stderr,"API handler wrote %u bytes into a %u byte response buffer , serving %s \n",responseToFill.bodyLength,buffer->bodySize,rqst.path);
     responseToFill.bodyLength = buffer->bodySize;
   }

  char header[API_RESPONSE_HEADER_RESERVE];
  int headerLength = snprintf(header,sizeof(header),
                              "HTTP/1.1 %u %s\r\nContent-Type: %s\r\nContent-Length: %u\r\n%s\r\n",
                              responseToFill.statusCode,
                              GetHTTPReasonPhrase(responseToFill.statusCode),
                              (responseToFill.contentType!=0) ? responseToFill.contentType : "text/plain",
                              responseToFill.bodyLength,
                              frame->keepAlive ? "" : "Connection: close\r\n" );

  if ( (headerLength<=0) || (headerLength>API_RESPONSE_HEADER_RESERVE) )
   {
     //Only reachable if a handler supplied an absurdly long Content-Type , which would truncate the header.
     *failStatus = 500;
     return 0;
   }

  //Place the header immediately in front of the body the handler just wrote , so the whole response is one
  //contiguous block and leaves in a single send() with the body never being copied.
  char * responseStart = buffer->memory + API_RESPONSE_HEADER_RESERVE - headerLength;
  memcpy(responseStart,header,(size_t) headerLength);

  *response = responseStart;

  //A HEAD response carries the Content-Length the GET would have , but none of the body.
  return (unsigned int) headerLength + ( (method==HEAD) ? 0 : responseToFill.bodyLength );
}
