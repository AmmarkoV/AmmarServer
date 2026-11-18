#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "pages.h"

#define VIDEOS_PER_PAGE 60

struct Writer
{
  char * buffer;
  unsigned long length;
  unsigned long size;
};

static void startWriter(struct Writer * w,struct AmmServer_DynamicRequest * rqst)
{
  w->buffer=rqst->content;
  w->size=rqst->MAXcontentSize;
  w->length=0;
  if (w->size>0) { w->buffer[0]=0; }
}

static void finishWriter(struct Writer * w,struct AmmServer_DynamicRequest * rqst)
{
  rqst->contentSize=w->length;
}

static void put(struct Writer * w,const char * format,...)
{
  if (w->length+1>=w->size) { return; }
  va_list args;
  va_start(args,format);
  int written=vsnprintf(w->buffer+w->length,w->size-w->length,format,args);
  va_end(args);
  if (written<0) { return; }
  w->length += ( (unsigned long) written < w->size-w->length ) ? (unsigned long) written : w->size-w->length-1;
}

static void putEscaped(struct Writer * w,const char * text)
{
  for (; *text; ++text)
  {
    switch (*text)
    {
      case '<'  : put(w,"&lt;");   break;
      case '>'  : put(w,"&gt;");   break;
      case '&'  : put(w,"&amp;");  break;
      case '"'  : put(w,"&quot;"); break;
      case '\'' : put(w,"&#39;");  break;
      default   : put(w,"%c",*text); break;
    }
  }
}

//URL-encoded and safe to place inside an HTML attribute
static void putURLEncoded(struct Writer * w,const char * text)
{
  for (; *text; ++text)
  {
    unsigned char c=(unsigned char) *text;
    if ( ((c>='a')&&(c<='z')) || ((c>='A')&&(c<='Z')) || ((c>='0')&&(c<='9')) || (c=='-') || (c=='_') || (c=='.') || (c=='~') )
         { put(w,"%c",c); }
    else { put(w,"%%%02X",c); }
  }
}

static int hexValue(char c)
{
  if ((c>='0')&&(c<='9')) { return c-'0'; }
  if ((c>='a')&&(c<='f')) { return c-'a'+10; }
  if ((c>='A')&&(c<='F')) { return c-'A'+10; }
  return -1;
}

void urlDecodeInPlace(char * text)
{
  char * out=text;
  while (*text)
  {
    if ( (*text=='%') && (hexValue(text[1])>=0) && (hexValue(text[2])>=0) )
    {
      char decoded=(char) ((hexValue(text[1])<<4)|hexValue(text[2]));
      if (decoded!=0) { *out++=decoded; }
      text+=3;
    } else
    if (*text=='+') { *out++=' '; ++text; }
    else            { *out++=*text++; }
  }
  *out=0;
}

static void formatSize(unsigned long long bytes,char * out,unsigned int outSize)
{
  if (bytes>=1024ULL*1024*1024) { snprintf(out,outSize,"%.2f GB",(double) bytes/(1024.0*1024*1024)); } else
                                { snprintf(out,outSize,"%.0f MB",(double) bytes/(1024.0*1024)); }
}


//Look , behaviour and the intro sound live in res/ ( ammarflix.css , ammarflix.js , sound.mp3 , favicon ) , served as they are

static void putHead(struct Writer * w,const char * title)
{
  put(w,"<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\">"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<meta name=\"referrer\" content=\"no-referrer\">"
        "<link rel=\"icon\" href=\"favicon.svg\" type=\"image/svg+xml\"><link rel=\"alternate icon\" href=\"favicon.ico\">"
        "<link rel=\"stylesheet\" href=\"ammarflix.css\"><script src=\"ammarflix.js\" defer></script><title>");
  putEscaped(w,title);
  put(w,"</title></head><body>");
}

static void putHeader(struct Writer * w,const char * username,const char * query)
{
  put(w,"<header><a class=\"logo\" href=\"library.html\">Ammarflix</a>"
        "<form action=\"library.html\" method=\"get\"><input type=\"search\" name=\"q\" placeholder=\"Search titles and folders\" value=\"");
  putEscaped(w,query);
  put(w,"\"></form><a class=\"rnd\" href=\"random.html\" title=\"Random video ( R ) , press ? for all shortcuts\">&#9858; Random</a><span class=\"user\">");
  putEscaped(w,username);
  put(w," &middot; <a href=\"logout.html\">Log out</a></span></header>");
}


void page_login(struct AmmServer_DynamicRequest * rqst,const char * message)
{
  struct Writer w;
  startWriter(&w,rqst);
  putHead(&w,"Ammarflix");
  //multipart : AmmarServer's POST parser does not understand urlencoded bodies
  put(&w,"<form class=\"login\" method=\"post\" action=\"login.html\" enctype=\"multipart/form-data\">"
         "<div class=\"logo\">Ammarflix</div>");
  if ( (message!=0) && (message[0]!=0) ) { put(&w,"<p class=\"msg\">"); putEscaped(&w,message); put(&w,"</p>"); }
  put(&w,"<input name=\"username\" placeholder=\"Username\" autocomplete=\"username\" required autofocus>"
         "<input name=\"password\" type=\"password\" placeholder=\"Password\" autocomplete=\"current-password\" required>"
         "<button type=\"submit\">Sign in</button></form></body></html>");
  finishWriter(&w,rqst);
}

void page_redirect(struct AmmServer_DynamicRequest * rqst,const char * target)
{
  struct Writer w;
  startWriter(&w,rqst);
  put(&w,"<!DOCTYPE html><html><head><meta http-equiv=\"refresh\" content=\"0; url=");
  putEscaped(&w,target);
  put(&w,"\"></head><body><a href=\"");
  putEscaped(&w,target);
  put(&w,"\">Continue</a></body></html>");
  finishWriter(&w,rqst);
}

void page_message(struct AmmServer_DynamicRequest * rqst,unsigned int httpCode,const char * message)
{
  struct Writer w;
  startWriter(&w,rqst);
  rqst->headerResponse=httpCode;
  putHead(&w,"Ammarflix");
  put(&w,"<main><p>");
  putEscaped(&w,message);
  put(&w,"</p><p><a href=\"library.html\" style=\"color:var(--accent)\">Back to the library</a></p></main></body></html>");
  finishWriter(&w,rqst);
}


static void putPageLink(struct Writer * w,const char * query,unsigned int page,const char * text)
{
  put(w,"<a href=\"library.html?p=%u",page);
  if (query[0]!=0) { put(w,"&amp;q="); putURLEncoded(w,query); }
  put(w,"\">%s</a>",text);
}

void page_library(struct AmmServer_DynamicRequest * rqst,const char * username,const char * query,unsigned int page)
{
  struct Writer w;
  startWriter(&w,rqst);
  putHead(&w,(query[0]!=0) ? query : "Ammarflix");
  putHeader(&w,username,query);
  put(&w,"<main>");

  library_lock();

  unsigned int matches=0;
  unsigned int i=0;
  for (i=0; i<library_count(); i++) { if (library_matches(library_get(i),query)) { ++matches; } }

  unsigned int pages=(matches+VIDEOS_PER_PAGE-1)/VIDEOS_PER_PAGE;
  if (page>=pages) { page = (pages>0) ? pages-1 : 0; }
  unsigned int first=page*VIDEOS_PER_PAGE;

  put(&w,"<p class=\"count\">%u video%s",matches,(matches==1) ? "" : "s");
  if (query[0]!=0) { put(&w," matching &ldquo;"); putEscaped(&w,query); put(&w,"&rdquo;"); }
  put(&w,"</p><div class=\"grid\">");

  unsigned int seen=0;
  for (i=0; i<library_count(); i++)
  {
    const struct Video * v=library_get(i);
    if (!library_matches(v,query)) { continue; }
    if ( (seen>=first) && (seen<first+VIDEOS_PER_PAGE) )
    {
      put(&w,"<a class=\"card\" data-id=\"%s\" href=\"watch.html?v=%s\" title=\"",v->id,v->id);
      putEscaped(&w,v->title);
      put(&w,"\"><div class=\"thumb\"><img loading=\"lazy\" alt=\"\" src=\"thumb.jpg?v=%s\" onerror=\"this.remove()\"></div><div class=\"t\">",v->id);
      putEscaped(&w,v->title);
      put(&w,"</div><div class=\"f\">");
      putEscaped(&w,v->folder);
      put(&w,"</div></a>");
    }
    ++seen;
  }

  library_unlock();

  put(&w,"</div>");
  if (pages>1)
  {
    put(&w,"<nav class=\"pager\">");
    if (page>0) { putPageLink(&w,query,page-1,"&larr; Previous"); }
    put(&w,"<span>Page %u of %u</span>",page+1,pages);
    if (page+1<pages) { putPageLink(&w,query,page+1,"Next &rarr;"); }
    put(&w,"</nav>");
  }
  put(&w,"</main></body></html>");
  finishWriter(&w,rqst);
}


void page_watch(struct AmmServer_DynamicRequest * rqst,const char * username,const struct Video * video,
                const struct SubtitleTrack * tracks,unsigned int trackCount)
{
  struct Writer w;
  startWriter(&w,rqst);
  putHead(&w,video->title);
  putHeader(&w,username,"");

  put(&w,"<main class=\"watch\"><h1>");
  putEscaped(&w,video->title);
  //No autoplay : ammarflix.js plays the intro ( res/sound.mp3 ) first and then starts the video
  put(&w,"</h1><div class=\"stage\"><video id=\"player\" data-id=\"%s\" controls preload=\"auto\" src=\"stream?v=%s\"></video>"
         "<div id=\"intro\" class=\"intro\" hidden><div class=\"mark\">AMMARFLIX</div>"
         "<button class=\"play\" type=\"button\">&#9654; Play</button><span class=\"skip\">click or Space to skip &middot; I turns the intro off</span></div></div>",
         video->id,video->id);

  put(&w,"<div class=\"bar\"><label>Subtitles <select id=\"subs\"><option value=\"\">Off</option>");
  unsigned int i=0;
  for (i=0; i<trackCount; i++)
  {
    put(&w,"<option value=\"subtitle.txt?v=%s&amp;%s=",video->id,(tracks[i].kind==SUBTITLE_SIDECAR) ? "s" : "e");
    putURLEncoded(&w,tracks[i].key);
    put(&w,"\">");
    putEscaped(&w,tracks[i].label);
    put(&w,"</option>");
  }
  put(&w,"</select></label><span id=\"substatus\"></span>");

  char size[32];
  formatSize(video->size,size,sizeof(size));
  put(&w,"<span style=\"margin-left:auto\">");
  putEscaped(&w,video->folder);
  put(&w," &middot; %s</span></div><p class=\"keys\">Press <kbd>?</kbd> for keyboard shortcuts</p></main></body></html>",size);
  finishWriter(&w,rqst);
}
