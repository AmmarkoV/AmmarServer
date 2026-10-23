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

/* The JSON an API mode handler needs and nothing more : read one string field out of a request body ,
 * escape text into a response. A hand written parser rather than a dependency , because that whole surface
 * is one object with one string field coming in and fixed-shape objects going out. See AmmServerlib.h for
 * what these two do ; everything else in here is private to them. */
#include <string.h>

#include "json_tools.h"

struct JsonParser
{
   const char * doc;
   unsigned int length;
   unsigned int at;

   /* Set while descending into the one field the caller asked for, so its string value is captured in the
    * same pass that validates it. */
   const char * wantedField;
   unsigned int depth;
   char * out;
   unsigned int outSize;
   unsigned int outLength;     /* bytes the value needs, which can exceed outSize */
   int found;
   int foundIsString;
};

static void SkipWhitespace(struct JsonParser * p)
{
  while (p->at < p->length)
   {
     char c = p->doc[p->at];
     if ( (c==' ') || (c=='\t') || (c=='\n') || (c=='\r') ) { ++p->at; } else { break; }
   }
}

static void EmitDecodedByte(struct JsonParser * p,int capture,char byte)
{
  if (!capture) { return; }
  if (p->outLength < p->outSize) { p->out[p->outLength] = byte; }
  ++p->outLength;
}

/* A \uXXXX escape, encoded out as UTF-8. Surrogate pairs are joined so characters outside the BMP survive. */
static int ParseUnicodeEscape(struct JsonParser * p,int capture)
{
  unsigned int codepoint=0;
  unsigned int i=0;

  if (p->at+4 > p->length) { return 0; }
  for (i=0; i<4; i++)
   {
     char c = p->doc[p->at+i];
     unsigned int digit=0;
     if      ( (c>='0') && (c<='9') ) { digit = (unsigned int)(c-'0');      }
     else if ( (c>='a') && (c<='f') ) { digit = (unsigned int)(c-'a')+10;   }
     else if ( (c>='A') && (c<='F') ) { digit = (unsigned int)(c-'A')+10;   }
     else { return 0; }
     codepoint = (codepoint<<4) | digit;
   }
  p->at += 4;

  if ( (codepoint>=0xD800) && (codepoint<=0xDBFF) )
   {
     /* high surrogate - a low one has to follow for this to be a character at all */
     if ( (p->at+6 <= p->length) && (p->doc[p->at]=='\\') && (p->doc[p->at+1]=='u') )
      {
        unsigned int low=0;
        for (i=0; i<4; i++)
         {
           char c = p->doc[p->at+2+i];
           unsigned int digit=0;
           if      ( (c>='0') && (c<='9') ) { digit = (unsigned int)(c-'0');    }
           else if ( (c>='a') && (c<='f') ) { digit = (unsigned int)(c-'a')+10; }
           else if ( (c>='A') && (c<='F') ) { digit = (unsigned int)(c-'A')+10; }
           else { return 0; }
           low = (low<<4) | digit;
         }
        if ( (low<0xDC00) || (low>0xDFFF) ) { return 0; }
        p->at += 6;
        codepoint = 0x10000 + ((codepoint-0xD800)<<10) + (low-0xDC00);
      } else { return 0; }
   }
  else if ( (codepoint>=0xDC00) && (codepoint<=0xDFFF) ) { return 0; } /* lone low surrogate */

  if (codepoint<0x80)
   { EmitDecodedByte(p,capture,(char) codepoint); }
  else if (codepoint<0x800)
   {
     EmitDecodedByte(p,capture,(char)(0xC0 | (codepoint>>6)));
     EmitDecodedByte(p,capture,(char)(0x80 | (codepoint & 0x3F)));
   }
  else if (codepoint<0x10000)
   {
     EmitDecodedByte(p,capture,(char)(0xE0 | (codepoint>>12)));
     EmitDecodedByte(p,capture,(char)(0x80 | ((codepoint>>6) & 0x3F)));
     EmitDecodedByte(p,capture,(char)(0x80 | (codepoint & 0x3F)));
   }
  else
   {
     EmitDecodedByte(p,capture,(char)(0xF0 | (codepoint>>18)));
     EmitDecodedByte(p,capture,(char)(0x80 | ((codepoint>>12) & 0x3F)));
     EmitDecodedByte(p,capture,(char)(0x80 | ((codepoint>>6) & 0x3F)));
     EmitDecodedByte(p,capture,(char)(0x80 | (codepoint & 0x3F)));
   }

  return 1;
}

/* Consume a string. With capture set, its decoded bytes go to p->out ; nameStart/nameLength come back
 * pointing at the raw ( still escaped ) bytes, which is all a key comparison needs. */
static int ParseString(struct JsonParser * p,int capture,const char ** rawStart,unsigned int * rawLength)
{
  if ( (p->at>=p->length) || (p->doc[p->at]!='"') ) { return 0; }
  ++p->at;

  unsigned int start=p->at;
  if (capture) { p->outLength=0; }

  while (p->at < p->length)
   {
     unsigned char c = (unsigned char) p->doc[p->at];

     if (c=='"')
      {
        if (rawStart!=0)  { *rawStart = p->doc+start;  }
        if (rawLength!=0) { *rawLength = p->at-start;  }
        ++p->at;
        return 1;
      }

     if (c=='\\')
      {
        ++p->at;
        if (p->at>=p->length) { return 0; }
        char escaped = p->doc[p->at];
        ++p->at;
        switch (escaped)
         {
           case '"'  : EmitDecodedByte(p,capture,'"');  break;
           case '\\' : EmitDecodedByte(p,capture,'\\'); break;
           case '/'  : EmitDecodedByte(p,capture,'/');  break;
           case 'b'  : EmitDecodedByte(p,capture,'\b'); break;
           case 'f'  : EmitDecodedByte(p,capture,'\f'); break;
           case 'n'  : EmitDecodedByte(p,capture,'\n'); break;
           case 'r'  : EmitDecodedByte(p,capture,'\r'); break;
           case 't'  : EmitDecodedByte(p,capture,'\t'); break;
           case 'u'  : if (!ParseUnicodeEscape(p,capture)) { return 0; } break;
           default   : return 0; /* not an escape JSON defines */
         };
        continue;
      }

     if (c<0x20) { return 0; } /* a raw control character is not allowed inside a JSON string */

     EmitDecodedByte(p,capture,(char) c);
     ++p->at;
   }

  return 0; /* ran out of document before the closing quote */
}

static int ParseNumber(struct JsonParser * p)
{
  unsigned int start=p->at;

  if ( (p->at<p->length) && (p->doc[p->at]=='-') ) { ++p->at; }

  if ( (p->at>=p->length) || (p->doc[p->at]<'0') || (p->doc[p->at]>'9') ) { return 0; }
  if (p->doc[p->at]=='0') { ++p->at; }
  else { while ( (p->at<p->length) && (p->doc[p->at]>='0') && (p->doc[p->at]<='9') ) { ++p->at; } }

  if ( (p->at<p->length) && (p->doc[p->at]=='.') )
   {
     ++p->at;
     if ( (p->at>=p->length) || (p->doc[p->at]<'0') || (p->doc[p->at]>'9') ) { return 0; }
     while ( (p->at<p->length) && (p->doc[p->at]>='0') && (p->doc[p->at]<='9') ) { ++p->at; }
   }

  if ( (p->at<p->length) && ((p->doc[p->at]=='e')||(p->doc[p->at]=='E')) )
   {
     ++p->at;
     if ( (p->at<p->length) && ((p->doc[p->at]=='+')||(p->doc[p->at]=='-')) ) { ++p->at; }
     if ( (p->at>=p->length) || (p->doc[p->at]<'0') || (p->doc[p->at]>'9') ) { return 0; }
     while ( (p->at<p->length) && (p->doc[p->at]>='0') && (p->doc[p->at]<='9') ) { ++p->at; }
   }

  return (p->at>start);
}

static int ParseLiteral(struct JsonParser * p,const char * literal)
{
  unsigned int length = strlen(literal);
  if (p->at+length > p->length) { return 0; }
  if (memcmp(p->doc+p->at,literal,length)!=0) { return 0; }
  p->at += length;
  return 1;
}

static int ParseValue(struct JsonParser * p);

static int ParseObject(struct JsonParser * p)
{
  ++p->at; /* '{' */
  ++p->depth;
  SkipWhitespace(p);

  if ( (p->at<p->length) && (p->doc[p->at]=='}') ) { ++p->at; --p->depth; return 1; }

  while (1)
   {
     SkipWhitespace(p);

     const char * nameStart=0;
     unsigned int nameLength=0;
     if (!ParseString(p,0,&nameStart,&nameLength)) { return 0; }

     SkipWhitespace(p);
     if ( (p->at>=p->length) || (p->doc[p->at]!=':') ) { return 0; }
     ++p->at;
     SkipWhitespace(p);

     /* The wanted field is only the wanted field at the top level, and only the first time it appears. */
     int isWanted = ( (p->depth==1) && (!p->found) &&
                      (nameLength==strlen(p->wantedField)) &&
                      (memcmp(nameStart,p->wantedField,nameLength)==0) );

     if (isWanted)
      {
        p->found=1;
        p->foundIsString = ( (p->at<p->length) && (p->doc[p->at]=='"') );
        if (p->foundIsString)
         {
           if (!ParseString(p,1,0,0)) { return 0; }
         } else
         {
           if (!ParseValue(p)) { return 0; }
         }
      } else
      {
        if (!ParseValue(p)) { return 0; }
      }

     SkipWhitespace(p);
     if (p->at>=p->length) { return 0; }
     if (p->doc[p->at]==',') { ++p->at; continue; }
     if (p->doc[p->at]=='}') { ++p->at; --p->depth; return 1; }
     return 0;
   }
}

static int ParseArray(struct JsonParser * p)
{
  ++p->at; /* '[' */
  ++p->depth;
  SkipWhitespace(p);

  if ( (p->at<p->length) && (p->doc[p->at]==']') ) { ++p->at; --p->depth; return 1; }

  while (1)
   {
     if (!ParseValue(p)) { return 0; }
     SkipWhitespace(p);
     if (p->at>=p->length) { return 0; }
     if (p->doc[p->at]==',') { ++p->at; SkipWhitespace(p); continue; }
     if (p->doc[p->at]==']') { ++p->at; --p->depth; return 1; }
     return 0;
   }
}

static int ParseValue(struct JsonParser * p)
{
  SkipWhitespace(p);
  if (p->at>=p->length) { return 0; }

  /* Nesting this deep is not something the API has any use for, and the recursion here follows the
   * document's own shape, so it is capped rather than left to the stack to discover. */
  if (p->depth>32) { return 0; }

  switch (p->doc[p->at])
   {
     case '{'  : return ParseObject(p);
     case '['  : return ParseArray(p);
     case '"'  : return ParseString(p,0,0,0);
     case 't'  : return ParseLiteral(p,"true");
     case 'f'  : return ParseLiteral(p,"false");
     case 'n'  : return ParseLiteral(p,"null");
     default   : return ParseNumber(p);
   };
}

enum AmmServer_JSONFieldResult AmmServer_JSONGetStringField(
                                         const char * doc,
                                         unsigned int docLength,
                                         const char * field,
                                         char * out,
                                         unsigned int outSize,
                                         unsigned int * outLength
                                       )
{
  struct JsonParser p={0};
  p.doc=doc;
  p.length=docLength;
  p.wantedField=field;
  p.out=out;
  p.outSize=outSize;

  *outLength=0;
  out[0]=0;

  if ( (doc==0) || (docLength==0) ) { return AMMSERVER_JSON_MALFORMED; }

  if (!ParseValue(&p)) { return AMMSERVER_JSON_MALFORMED; }

  /* Trailing garbage makes the whole document invalid, not just ignorable. */
  SkipWhitespace(&p);
  if (p.at!=p.length) { return AMMSERVER_JSON_MALFORMED; }

  if ( (!p.found) || (!p.foundIsString) ) { return AMMSERVER_JSON_FIELD_NOT_A_STRING; }

  *outLength = p.outLength;
  if (p.outLength < outSize) { out[p.outLength]=0; } else { out[outSize-1]=0; }

  return AMMSERVER_JSON_FIELD_OK;
}

unsigned int AmmServer_JSONEscape(const char * in,unsigned int inLength,char * out,unsigned int outSize)
{
  static const char hex[]="0123456789abcdef";
  unsigned int written=0;
  unsigned int i=0;

  for (i=0; i<inLength; i++)
   {
     unsigned char c = (unsigned char) in[i];

     if ( (c!='"') && (c!='\\') && (c>=0x20) )
      {
        if (written+1>outSize) { return 0; }
        out[written++]=(char) c;
        continue;
      }

     switch (c)
      {
        case '"'  : if (written+2>outSize) { return 0; } out[written++]='\\'; out[written++]='"';  break;
        case '\\' : if (written+2>outSize) { return 0; } out[written++]='\\'; out[written++]='\\'; break;
        case '\b' : if (written+2>outSize) { return 0; } out[written++]='\\'; out[written++]='b';  break;
        case '\f' : if (written+2>outSize) { return 0; } out[written++]='\\'; out[written++]='f';  break;
        case '\n' : if (written+2>outSize) { return 0; } out[written++]='\\'; out[written++]='n';  break;
        case '\r' : if (written+2>outSize) { return 0; } out[written++]='\\'; out[written++]='r';  break;
        case '\t' : if (written+2>outSize) { return 0; } out[written++]='\\'; out[written++]='t';  break;
        default   :
          if (written+6>outSize) { return 0; }
          out[written++]='\\'; out[written++]='u'; out[written++]='0'; out[written++]='0';
          out[written++]=hex[(c>>4) & 0xF];
          out[written++]=hex[c & 0xF];
        break;
      };
   }

  return written;
}
