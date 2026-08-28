#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>

#include "page.h"
#include "json.h"

/* ---- small safe buffer-append helpers --------------------------------
   appendStr()  : raw memcpy , so literal HTML/CSS/JS chunks that happen to
                  contain '%' ( CSS percentages, printf-looking JS strings )
                  never get mistaken for format specifiers.
   appendFmt()  : the only place that goes through vsnprintf , used solely
                  for the handful of genuinely dynamic substitutions , so
                  every format string here is short and hand-checked.
   ------------------------------------------------------------------- */
static void appendStr(char * buf,unsigned int bufSize,unsigned int * pos,const char * s)
{
  if (*pos+1>=bufSize) { return; }
  unsigned int L=(unsigned int) strlen(s);
  unsigned int avail=bufSize-*pos-1;
  if (L>avail) { L=avail; }
  memcpy(buf+*pos,s,L);
  *pos+=L;
  buf[*pos]=0;
}

static void appendFmt(char * buf,unsigned int bufSize,unsigned int * pos,const char * fmt,...)
{
  if (*pos+1>=bufSize) { return; }
  va_list ap;
  va_start(ap,fmt);
  int n=vsnprintf(buf+*pos,bufSize-*pos,fmt,ap);
  va_end(ap);
  if (n>0)
  {
    *pos+=(unsigned int) n;
    if (*pos>=bufSize) { *pos=bufSize-1; }
  }
}

//Appends a JS/JSON string literal ( quotes included ) for str , via jsonAppendEscaped which is valid in both contexts
static void appendJSString(char * buf,unsigned int bufSize,unsigned int * pos,const char * str)
{
  jsonAppendEscaped(buf,bufSize,str); //operates at strlen(buf) , which is exactly *pos since we always keep buf nul terminated
  *pos=(unsigned int) strlen(buf);
}


/* ===================================================================
   Landing page : no token yet
   =================================================================== */
void * renderLandingPage(struct AmmServer_DynamicRequest * rqst)
{
  unsigned int pos=0;
  char * buf=rqst->content;
  unsigned int bufSize=(unsigned int) rqst->MAXcontentSize;

  appendStr(buf,bufSize,&pos,
"<!DOCTYPE html>\n"
"<html lang=\"el\">\n"
"<head>\n"
"<meta charset=\"utf-8\">\n"
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
"<meta name=\"theme-color\" content=\"#ff7e5f\">\n"
"<title>" DEFAULT_TITLE " \xf0\x9f\x9b\x92</title>\n"
"<style>\n"
" body{margin:0;font-family:-apple-system,\"Segoe UI\",Roboto,sans-serif;\n"
"      background:linear-gradient(160deg,#ff7e5f,#feb47b);min-height:100vh;\n"
"      display:flex;align-items:center;justify-content:center;text-align:center}\n"
" .card{background:#fffdf8;border-radius:24px;padding:36px 28px;margin:20px;\n"
"       box-shadow:0 12px 40px rgba(0,0,0,.2);max-width:340px}\n"
" h1{margin:0 0 8px;color:#e85d3d;font-size:1.5em}\n"
" p{color:#7a6a5f;line-height:1.5}\n"
" a.btn{display:inline-block;margin-top:14px;background:#2bb3a3;color:#fff;\n"
"       text-decoration:none;font-weight:700;font-size:1.1em;\n"
"       padding:14px 28px;border-radius:999px;box-shadow:0 4px 14px rgba(43,179,163,.4)}\n"
"</style>\n"
"</head>\n"
"<body>\n"
"<div class=\"card\">\n"
"  <div style=\"font-size:3em\">\xf0\x9f\x9b\x92\xe2\x9c\xa8</div>\n"
"  <h1>" DEFAULT_TITLE "</h1>\n"
"  <p>\xce\xa6\xcf\x84\xce\xb9\xce\xac\xce\xbe\xce\xb5 \xce\xbc\xce\xb9\xce\xb1 \xce\xbb\xce\xaf\xcf\x83\xcf\x84\xce\xb1 \xce\xba\xce\xb1\xce\xb9 \xce\xbc\xce\xbf\xce\xb9\xcf\x81\xce\xac\xcf\x83\xce\xbf\xcf\x85 \xcf\x84\xce\xbf\xce\xbd \xcf\x83\xcf\x8d\xce\xbd\xce\xb4\xce\xb5\xcf\x83\xce\xbc\xce\xbf\n"
"     \xcf\x8c\xcf\x80\xce\xbf\xce\xb9\xce\xbf\xcf\x82 \xcf\x84\xce\xbf\xce\xbd \xce\xad\xcf\x87\xce\xb5\xce\xb9, \xce\xb2\xce\xbb\xce\xad\xcf\x80\xce\xb5\xce\xb9 \xce\xba\xce\xb1\xce\xb9 \xce\xb1\xce\xbb\xce\xbb\xce\xac\xce\xb6\xce\xb5\xce\xb9 \xcf\x84\xce\xb7 \xce\xbb\xce\xaf\xcf\x83\xcf\x84\xce\xb1.</p>\n"
"  <a class=\"btn\" href=\"go.php?new=1\">\xe2\x9e\x95 \xce\x9d\xce\xad\xce\xb1 \xce\xbb\xce\xaf\xcf\x83\xcf\x84\xce\xb1</a>\n"
"</div>\n"
"</body>\n"
"</html>");

  rqst->contentSize=pos;
  return 0;
}


/* ===================================================================
   ?new=1 : mint a token and bounce there
   =================================================================== */
void * renderNewCartRedirect(struct AmmServer_DynamicRequest * rqst)
{
  char token[9]={0};
  generateHexID(token,8);

  unsigned int pos=0;
  appendFmt(rqst->content,(unsigned int) rqst->MAXcontentSize,&pos,
    "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
    "<meta http-equiv=\"refresh\" content=\"0; url=go.php?i=%s\"></head>"
    "<body>Redirecting..</body></html>",token);

  rqst->contentSize=pos;
  return 0;
}


/* ===================================================================
   The cart page itself
   =================================================================== */
void * renderCartPage(struct AmmServer_DynamicRequest * rqst,const char * token,struct cart * cart)
{
  char * buf=rqst->content;
  unsigned int bufSize=(unsigned int) rqst->MAXcontentSize;
  unsigned int pos=0;

  const char * title=(cart->title[0]!=0) ? cart->title : DEFAULT_TITLE;

  char escapedTitle[TITLE_BUF_SIZE*6]={0};
  AmmServer_HTMLEscape(title,escapedTitle,sizeof(escapedTitle));

  unsigned int activeCount=0,i=0;
  for (i=0; i<cart->numberOfItems; i++) { if (!cart->items[i].checked) { ++activeCount; } }

  char coverPath[512]={0};
  photoPathFor(token,"cover",coverPath,sizeof(coverPath));
  struct stat coverStat; unsigned int coverMTime=0;
  if (stat(coverPath,&coverStat)==0) { coverMTime=(unsigned int) coverStat.st_mtime; }

  appendStr(buf,bufSize,&pos,
"<!DOCTYPE html>\n<html lang=\"el\">\n<head>\n"
"<meta charset=\"utf-8\">\n"
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
"<meta name=\"theme-color\" content=\"#ff7e5f\">\n");

  appendFmt(buf,bufSize,&pos,"<title>\xf0\x9f\x9b\x92 %s</title>\n",escapedTitle);

  appendStr(buf,bufSize,&pos,"<meta property=\"og:type\" content=\"website\">\n");
  appendFmt(buf,bufSize,&pos,"<meta property=\"og:site_name\" content=\"%s \xf0\x9f\x9b\x92\">\n",DEFAULT_TITLE);
  appendFmt(buf,bufSize,&pos,"<meta property=\"og:title\" content=\"\xf0\x9f\x9b\x92 %s\">\n",escapedTitle);
  appendFmt(buf,bufSize,&pos,"<meta property=\"og:description\" content=\"%s\">\n",
            activeCount ? "\xcf\x80\xcf\x81\xce\xbf\xcf\x8a\xcf\x8c\xce\xbd\xcf\x84\xce\xb1 \xce\xb3\xce\xb9\xce\xb1 \xce\xb1\xce\xb3\xce\xbf\xcf\x81\xce\xac" : "\xce\x8c\xce\xbb\xce\xb1 \xce\xad\xcf\x84\xce\xbf\xce\xb9\xce\xbc\xce\xb1! \xe2\x9c\xa8");
  appendFmt(buf,bufSize,&pos,"<meta property=\"og:url\" content=\"go.php?i=%s\">\n",token);
  if (coverMTime!=0)
  {
    appendFmt(buf,bufSize,&pos,"<meta property=\"og:image\" content=\"go.php?i=%s&amp;img=cover&amp;t=%u\">\n",token,coverMTime);
    appendStr(buf,bufSize,&pos,"<meta name=\"twitter:card\" content=\"summary\">\n");
  }

  appendStr(buf,bufSize,&pos,
"<style>\n"
" *{box-sizing:border-box}\n"
" body{margin:0;font-family:-apple-system,\"Segoe UI\",Roboto,sans-serif;background:#fff7ef;color:#4a3f38}\n"
" header{background:linear-gradient(135deg,#ff7e5f,#feb47b);color:#fff;\n"
"        padding:14px 16px 12px;position:sticky;top:0;z-index:10;\n"
"        box-shadow:0 2px 12px rgba(232,93,61,.35)}\n"
" header h1{margin:0;font-size:1.25em;display:flex;align-items:center;gap:8px}\n"
" #coverbtn{background:rgba(255,255,255,.2);border:none;width:40px;height:40px;\n"
"        border-radius:50%;font-size:1.15em;padding:0;flex-shrink:0;overflow:hidden}\n"
" #coverbtn img{width:100%;height:100%;object-fit:cover}\n"
" #title{min-width:0;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;cursor:pointer}\n"
" #title::after{content:' \xe2\x9c\x8f\xef\xb8\x8f';font-size:.7em;opacity:.7}\n"
" header .sub{font-size:.8em;opacity:.9;margin-top:2px}\n"
" #sync{margin-left:auto;background:rgba(255,255,255,.25);border:none;color:#fff;\n"
"        border-radius:999px;padding:8px 10px;font-size:.85em;font-weight:600;\n"
"        flex-shrink:0;font-variant-numeric:tabular-nums}\n"
" #share{background:rgba(255,255,255,.25);border:none;color:#fff;\n"
"        border-radius:999px;padding:8px 14px;font-size:.85em;font-weight:600;flex-shrink:0}\n"
" .addbar{display:flex;gap:8px;padding:12px 12px 4px;position:sticky;top:62px;z-index:9;\n"
"         background:linear-gradient(#fff7ef 80%,rgba(255,247,239,0))}\n"
" .addbar input{flex:1;min-width:0;font-size:1.05em;padding:12px 16px;border-radius:999px;\n"
"        border:2px solid #ffd9c4;background:#fff;outline:none}\n"
" .addbar input:focus{border-color:#ff9d78}\n"
" .addbar button{background:#2bb3a3;color:#fff;border:none;border-radius:999px;\n"
"        font-size:1.4em;width:52px;height:52px;flex-shrink:0;font-weight:700;\n"
"        box-shadow:0 3px 10px rgba(43,179,163,.4)}\n"
" main{padding:8px 12px 90px;max-width:560px;margin:0 auto}\n"
" .row{display:flex;align-items:center;gap:10px;background:#fff;border-radius:16px;\n"
"      padding:10px 12px;margin-bottom:8px;box-shadow:0 1px 4px rgba(120,80,50,.08)}\n"
" .circ{width:34px;height:34px;border-radius:50%;border:3px solid #ffb08e;background:#fff;\n"
"       flex-shrink:0;font-size:1em;color:#fff;padding:0}\n"
" .row.done .circ{background:#7ac74f;border-color:#7ac74f}\n"
" .name{flex:1;min-width:0;font-size:1.05em;overflow-wrap:anywhere}\n"
" .row.done .name{color:#b0a79e;text-decoration:line-through}\n"
" .qty{display:flex;align-items:center;gap:4px;flex-shrink:0}\n"
" .qty button{width:40px;height:40px;border-radius:12px;border:none;font-size:1.3em;\n"
"      font-weight:700;color:#fff;padding:0}\n"
" .qty .minus{background:#ffa270}\n"
" .qty .plus{background:#2bb3a3}\n"
" .qty input{width:44px;height:40px;text-align:center;font-size:1.05em;border-radius:10px;\n"
"      border:2px solid #ffd9c4;background:#fff}\n"
" .del{background:none;border:none;font-size:1.1em;color:#d9c6ba;flex-shrink:0;padding:6px}\n"
" .cam{background:none;border:none;font-size:1.05em;flex-shrink:0;padding:4px;opacity:.4}\n"
" .thumb{width:38px;height:38px;object-fit:cover;border-radius:10px;flex-shrink:0;\n"
"        border:2px solid #ffd9c4}\n"
" #viewer{position:fixed;inset:0;background:rgba(30,20,15,.92);z-index:30;\n"
"         display:flex;flex-direction:column;align-items:center;justify-content:center;gap:16px}\n"
" #viewer img{max-width:92vw;max-height:70vh;border-radius:16px}\n"
" #viewer .vname{color:#fff;font-size:1.1em;font-weight:600;max-width:90vw;text-align:center}\n"
" #viewer button{border:none;border-radius:999px;padding:12px 20px;font-size:1em;\n"
"         font-weight:600;color:#fff;margin:0 6px}\n"
" #viewer .vchg{background:#2bb3a3}\n"
" #viewer .vdel{background:#e85d3d}\n"
" .sect{display:flex;align-items:center;gap:8px;width:100%;background:#ffe8d6;border:none;\n"
"       border-radius:14px;padding:12px 16px;margin:14px 0 10px;font-size:1em;\n"
"       font-weight:700;color:#a4643f}\n"
" .sect .arrow{margin-left:auto;transition:transform .2s}\n"
" .sect.open .arrow{transform:rotate(180deg)}\n"
" .empty{text-align:center;color:#c7a893;padding:30px 10px;font-size:1.05em}\n"
" #toast{position:fixed;bottom:20px;left:50%;transform:translateX(-50%);\n"
"        background:#4a3f38;color:#fff;padding:10px 20px;border-radius:999px;\n"
"        opacity:0;transition:opacity .3s;pointer-events:none;font-size:.9em;z-index:20}\n"
"</style>\n"
"</head>\n<body>\n"
"<header>\n"
"  <h1><button id=\"coverbtn\" onclick=\"coverClick()\" aria-label=\"\xce\x95\xce\xb9\xce\xba\xcf\x8c\xce\xbd\xce\xb1 \xce\xbb\xce\xaf\xcf\x83\xcf\x84\xce\xb1\xcf\x82\">\xf0\x9f\x9b\x92</button>\n"
"      <span id=\"title\" onclick=\"editName()\"></span>\n"
"      <button id=\"sync\" onclick=\"refreshNow()\" aria-label=\"\xce\x91\xce\xbd\xce\xb1\xce\xbd\xce\xad\xcf\x89\xcf\x83\xce\xb7\">\xf0\x9f\x94\x84 \xcf\x84\xcf\x8e\xcf\x81\xce\xb1</button>\n"
"      <button id=\"share\" onclick=\"share()\">\xf0\x9f\x94\x97 \xce\x9a\xce\xbf\xce\xb9\xce\xbd\xce\xae \xcf\x87\xcf\x81\xce\xae\xcf\x83\xce\xb7</button></h1>\n"
"  <div class=\"sub\" id=\"counter\"></div>\n"
"</header>\n"
"<div class=\"addbar\">\n"
"  <input id=\"newname\" type=\"text\" placeholder=\"\xce\xa0\xcf\x81\xcf\x8c\xcf\x83\xce\xb8\xce\xb5\xcf\x83\xce\xb5 \xcf\x80\xcf\x81\xce\xbf\xcf\x8a\xcf\x8c\xce\xbd... \xf0\x9f\x8d\x85\" ");
  appendFmt(buf,bufSize,&pos,"maxlength=\"%d\" ",MAX_NAME_LEN);
  appendStr(buf,bufSize,&pos,
"onkeydown=\"if(event.key==='Enter')addItem()\">\n"
"  <button onclick=\"addItem()\" aria-label=\"\xce\xa0\xcf\x81\xce\xbf\xcf\x83\xce\xb8\xce\xae\xce\xba\xce\xb7\">\xef\xbc\x8b</button>\n"
"</div>\n"
"<main>\n"
"  <div id=\"list\"></div>\n"
"  <button class=\"sect\" id=\"donehdr\" onclick=\"toggleDone()\">\n"
"     \xe2\x9c\x85 \xce\xa3\xcf\x84\xce\xbf \xce\xba\xce\xb1\xce\xbb\xce\xac\xce\xb8\xce\xb9 <span id=\"donecount\"></span><span class=\"arrow\">\xe2\x96\xbc</span>\n"
"  </button>\n"
"  <div id=\"donelist\" style=\"display:none\"></div>\n"
"</main>\n"
"<div id=\"toast\"></div>\n"
"<input type=\"file\" id=\"imgfile\" accept=\"image/*\" style=\"display:none\">\n"
"<div id=\"viewer\" style=\"display:none\" onclick=\"closeViewer()\">\n"
"  <div class=\"vname\" id=\"vname\"></div>\n"
"  <img id=\"vimg\" alt=\"\">\n"
"  <div>\n"
"    <button class=\"vchg\" onclick=\"event.stopPropagation();pickImg(VIEWID)\">\xf0\x9f\x93\xb7 \xce\x91\xce\xbb\xce\xbb\xce\xb1\xce\xb3\xce\xae</button>\n"
"    <button class=\"vdel\" onclick=\"event.stopPropagation();delImg(VIEWID)\">\xf0\x9f\x97\x91\xef\xb8\x8f \xce\x94\xce\xb9\xce\xb1\xce\xb3\xcf\x81\xce\xb1\xcf\x86\xce\xae</button>\n"
"  </div>\n"
"</div>\n"
"<script>\n");

  appendStr(buf,bufSize,&pos,"const TOKEN=");
  appendJSString(buf,bufSize,&pos,token);
  appendStr(buf,bufSize,&pos,";\nconst DEFTITLE=");
  appendJSString(buf,bufSize,&pos,DEFAULT_TITLE);
  appendFmt(buf,bufSize,&pos,";\nconst MAXQ=%d;\n",MAX_QTY);

  appendStr(buf,bufSize,&pos,"let CART=");
  {
    unsigned int cartJSONCapacity=192*1024;
    char * cartJSON=(char*) malloc(cartJSONCapacity);
    if (cartJSON!=0)
    {
      buildCartJSON(token,cart,cartJSON,cartJSONCapacity);
      appendStr(buf,bufSize,&pos,cartJSON);
      free(cartJSON);
    }
  }
  appendStr(buf,bufSize,&pos,";\nlet showDone=false;\n");

  appendStr(buf,bufSize,&pos,
"function esc(s){const d=document.createElement('div');d.textContent=s;return d.innerHTML;}\n"
"function toast(msg){\n"
"  const t=document.getElementById('toast');\n"
"  t.textContent=msg;t.style.opacity=1;\n"
"  clearTimeout(t._h);t._h=setTimeout(()=>t.style.opacity=0,1800);\n"
"}\n"
"async function post(data){\n"
"  try{\n"
"    const fd=new FormData();\n"
"    for(const k in data)fd.append(k,data[k]);\n"
"    const r=await fetch('go.php?i='+encodeURIComponent(TOKEN),{method:'POST',body:fd});\n"
"    if(!r.ok)throw 0;\n"
"    CART=await r.json();\n"
"    render();\n"
"    synced();\n"
"  }catch(e){OFFLINE=true;updSync();toast('\xe2\x9a\xa0\xef\xb8\x8f \xce\xa0\xcf\x81\xcf\x8c\xce\xb2\xce\xbb\xce\xb7\xce\xbc\xce\xb1 \xcf\x83\xcf\x8d\xce\xbd\xce\xb4\xce\xb5\xcf\x83\xce\xb7\xcf\x82, \xce\xb4\xce\xbf\xce\xba\xce\xaf\xce\xbc\xce\xb1\xcf\x83\xce\xb5 \xce\xbe\xce\xb1\xce\xbd\xce\xac');}\n"
"}\n"
"let SYNCAT=Date.now(), OFFLINE=false;\n"
"function synced(){SYNCAT=Date.now();OFFLINE=false;updSync();}\n"
"function updSync(){\n"
"  const s=Math.floor((Date.now()-SYNCAT)/1000);\n"
"  const t=s<10?'\xcf\x84\xcf\x8e\xcf\x81\xce\xb1':s<60?s+'\xe2\x80\xb3':s<3600?Math.floor(s/60)+'\xe2\x80\xb2':Math.floor(s/3600)+'\xcf\x89';\n"
"  document.getElementById('sync').textContent=(OFFLINE?'\xe2\x9a\xa0\xef\xb8\x8f':'\xf0\x9f\x94\x84')+' '+t;\n"
"}\n"
"async function poll(){\n"
"  if(document.hidden)return;\n"
"  if(document.activeElement&&document.activeElement.closest&&\n"
"     document.activeElement.closest('.qty'))return;\n"
"  const base='go.php?i='+encodeURIComponent(TOKEN);\n"
"  try{\n"
"    const j=await(await fetch(base+'&rev=1')).json();\n"
"    if(j.rev!==(CART.rev||0)){\n"
"      CART=await(await fetch(base+'&api=1')).json();\n"
"      render();\n"
"    }\n"
"    synced();\n"
"  }catch(e){OFFLINE=true;updSync();}\n"
"}\n"
"function refreshNow(){poll().then(()=>{if(!OFFLINE)toast('\xe2\x9c\x85 \xce\x95\xce\xbd\xce\xb7\xce\xbc\xce\xb5\xcf\x81\xcf\x8e\xce\xb8\xce\xb7\xce\xba\xce\xb5!');});}\n");

  appendFmt(buf,bufSize,&pos,"setInterval(poll,%d);\n",30*1000);
  appendFmt(buf,bufSize,&pos,"setInterval(updSync,%d);\n",5*1000);

  appendStr(buf,bufSize,&pos,
"function imgURL(it){\n"
"  return 'go.php?i='+encodeURIComponent(TOKEN)+'&img='+it.id+'&t='+it.img;\n"
"}\n"
"function rowHTML(it){\n"
"  const done=it.c?' done':'';\n"
"  const qty=it.c?'':`\n"
"    <span class=\"qty\">\n"
"      <button class=\"minus\" onclick=\"qty('${it.id}',-1)\">\xe2\x88\x92</button>\n"
"      <input type=\"number\" inputmode=\"numeric\" min=\"1\" max=\"${MAXQ}\" value=\"${it.q}\"\n"
"             onchange=\"qtySet('${it.id}',this.value)\">\n"
"      <button class=\"plus\" onclick=\"qty('${it.id}',1)\">\xef\xbc\x8b</button>\n"
"    </span>`;\n"
"  const photo=it.img\n"
"    ? `<img class=\"thumb\" src=\"${imgURL(it)}\" onclick=\"viewImg('${it.id}')\" alt=\"\">`\n"
"    : `<button class=\"cam\" onclick=\"pickImg('${it.id}')\" aria-label=\"\xce\xa6\xcf\x89\xcf\x84\xce\xbf\xce\xb3\xcf\x81\xce\xb1\xcf\x86\xce\xaf\xce\xb1\">\xf0\x9f\x93\xb7</button>`;\n"
"  return `<div class=\"row${done}\">\n"
"    <button class=\"circ\" onclick=\"toggle('${it.id}')\">${it.c?'\xe2\x9c\x93':''}</button>\n"
"    ${photo}\n"
"    <span class=\"name\">${esc(it.n)}${it.c&&it.q>1?' \xc3\x97'+it.q:''}</span>\n"
"    ${qty}\n"
"    <button class=\"del\" onclick=\"del('${it.id}',this)\">\xf0\x9f\x97\x91\xef\xb8\x8f</button>\n"
"  </div>`;\n"
"}\n"
"function listName(){return CART.name||DEFTITLE;}\n"
"function render(){\n"
"  document.getElementById('title').textContent=listName();\n"
"  document.title='\xf0\x9f\x9b\x92 '+listName();\n"
"  document.getElementById('coverbtn').innerHTML=CART.cimg\n"
"    ? `<img src=\"go.php?i=${encodeURIComponent(TOKEN)}&img=cover&t=${CART.cimg}\" alt=\"\">`\n"
"    : '\xf0\x9f\x9b\x92';\n"
"  const items=CART.items;\n"
"  const todo=items.filter(i=>!i.c);\n"
"  const done=items.filter(i=>i.c)\n"
"                  .slice().sort((a,b)=>a.n.localeCompare(b.n,'el'));\n"
"  document.getElementById('list').innerHTML =\n"
"     todo.length ? todo.map(rowHTML).join('')\n"
"                 : '<div class=\"empty\">\xce\x97 \xce\xbb\xce\xaf\xcf\x83\xcf\x84\xce\xb1 \xce\xb5\xce\xaf\xce\xbd\xce\xb1\xce\xb9 \xce\xac\xce\xb4\xce\xb5\xce\xb9\xce\xb1! \xf0\x9f\x8e\x89<br>\xce\xa0\xcf\x81\xcf\x8c\xcf\x83\xce\xb8\xce\xb5\xcf\x83\xce\xb5 \xce\xba\xce\xac\xcf\x84\xce\xb9 \xce\xb1\xcf\x80\xcf\x8c \xcf\x80\xce\xac\xce\xbd\xcf\x89 \xe2\xac\x86\xef\xb8\x8f</div>';\n"
"  document.getElementById('donelist').innerHTML = done.map(rowHTML).join('');\n"
"  document.getElementById('donecount').textContent = '('+done.length+')';\n"
"  document.getElementById('counter').textContent =\n"
"     todo.length ? todo.length+' \xcf\x80\xcf\x81\xce\xbf\xcf\x8a\xcf\x8c\xce\xbd\xcf\x84\xce\xb1 \xce\xb3\xce\xb9\xce\xb1 \xce\xb1\xce\xb3\xce\xbf\xcf\x81\xce\xac' : '\xce\x8c\xce\xbb\xce\xb1 \xce\xad\xcf\x84\xce\xbf\xce\xb9\xce\xbc\xce\xb1! \xe2\x9c\xa8';\n"
"}\n"
"function addItem(){\n"
"  const inp=document.getElementById('newname');\n"
"  const n=inp.value.trim();\n"
"  if(!n)return;\n"
"  inp.value='';inp.focus();\n"
"  post({a:'add',n:n});\n"
"}\n"
"function qty(id,d){post({a:'qty',id:id,d:d});}\n"
"function qtySet(id,v){post({a:'qty',id:id,v:v});}\n"
"function toggle(id){post({a:'toggle',id:id});}\n"
"function del(id,btn){\n"
"  const name=btn.parentNode.querySelector('.name').textContent;\n"
"  if(confirm('\xce\x94\xce\xb9\xce\xb1\xce\xb3\xcf\x81\xce\xb1\xcf\x86\xce\xae \xc2\xab'+name+'\xc2\xbb ;'))post({a:'del',id:id});\n"
"}\n"
"let IMGID=null, VIEWID=null;\n"
"function pickImg(id){\n"
"  IMGID=id;\n"
"  document.getElementById('imgfile').click();\n"
"}\n"
"document.getElementById('imgfile').addEventListener('change',async function(){\n"
"  const file=this.files[0];\n"
"  this.value='';\n"
"  if(!file||!IMGID)return;\n"
"  toast('\xe2\x8f\xb3 \xce\x91\xce\xbd\xce\xad\xce\xb2\xce\xb1\xcf\x83\xce\xbc\xce\xb1 \xcf\x86\xcf\x89\xcf\x84\xce\xbf\xce\xb3\xcf\x81\xce\xb1\xcf\x86\xce\xaf\xce\xb1\xcf\x82...');\n"
"  try{\n"
"    const fd=new FormData();\n"
"    fd.append('a','img');fd.append('id',IMGID);fd.append('f',file);\n"
"    const r=await fetch('go.php?i='+encodeURIComponent(TOKEN),{method:'POST',body:fd});\n"
"    const c=await r.json();\n"
"    CART=c;render();closeViewer();synced();\n"
"    toast(c.error?'\xe2\x9a\xa0\xef\xb8\x8f '+c.error:'\xf0\x9f\x93\xb7 \xce\x97 \xcf\x86\xcf\x89\xcf\x84\xce\xbf\xce\xb3\xcf\x81\xce\xb1\xcf\x86\xce\xaf\xce\xb1 \xce\xb1\xce\xbd\xce\xad\xce\xb2\xce\xb7\xce\xba\xce\xb5!');\n"
"  }catch(e){toast('\xe2\x9a\xa0\xef\xb8\x8f \xce\xa0\xcf\x81\xcf\x8c\xce\xb2\xce\xbb\xce\xb7\xce\xbc\xce\xb1 \xcf\x83\xcf\x8d\xce\xbd\xce\xb4\xce\xb5\xcf\x83\xce\xb7\xcf\x82, \xce\xb4\xce\xbf\xce\xba\xce\xaf\xce\xbc\xce\xb1\xcf\x83\xce\xb5 \xce\xbe\xce\xb1\xce\xbd\xce\xac');}\n"
"});\n"
"function viewImg(id){\n"
"  let name,t;\n"
"  if(id==='cover'){name=listName();t=CART.cimg;}\n"
"  else{\n"
"    const it=CART.items.find(i=>i.id===id);\n"
"    if(!it)return;\n"
"    name=it.n;t=it.img;\n"
"  }\n"
"  if(!t)return;\n"
"  VIEWID=id;\n"
"  document.getElementById('vname').textContent=name;\n"
"  document.getElementById('vimg').src=\n"
"    'go.php?i='+encodeURIComponent(TOKEN)+'&img='+id+'&t='+t;\n"
"  document.getElementById('viewer').style.display='flex';\n"
"}\n"
"function coverClick(){CART.cimg?viewImg('cover'):pickImg('cover');}\n"
"function editName(){\n"
"  const v=prompt('\xce\x8c\xce\xbd\xce\xbf\xce\xbc\xce\xb1 \xce\xbb\xce\xaf\xcf\x83\xcf\x84\xce\xb1\xcf\x82:',CART.name||'');\n"
"  if(v!==null)post({a:'name',n:v.trim()});\n"
"}\n"
"function closeViewer(){\n"
"  document.getElementById('viewer').style.display='none';\n"
"  VIEWID=null;\n"
"}\n"
"function delImg(id){\n"
"  if(confirm('\xce\x94\xce\xb9\xce\xb1\xce\xb3\xcf\x81\xce\xb1\xcf\x86\xce\xae \xcf\x86\xcf\x89\xcf\x84\xce\xbf\xce\xb3\xcf\x81\xce\xb1\xcf\x86\xce\xaf\xce\xb1\xcf\x82;')){closeViewer();post({a:'imgdel',id:id});}\n"
"}\n"
"function toggleDone(){\n"
"  showDone=!showDone;\n"
"  document.getElementById('donelist').style.display=showDone?'':'none';\n"
"  document.getElementById('donehdr').classList.toggle('open',showDone);\n"
"}\n"
"function share(){\n"
"  const url=location.origin+location.pathname+'?i='+encodeURIComponent(TOKEN);\n"
"  if(navigator.share){\n"
"    navigator.share({title:listName()+' \xf0\x9f\x9b\x92',url:url}).catch(()=>{});\n"
"    return;\n"
"  }\n"
"  if(navigator.clipboard&&navigator.clipboard.writeText){\n"
"    navigator.clipboard.writeText(url)\n"
"      .then(()=>toast('\xf0\x9f\x93\x8b \xce\x9f \xcf\x83\xcf\x8d\xce\xbd\xce\xb4\xce\xb5\xcf\x83\xce\xbc\xce\xbf\xcf\x82 \xce\xb1\xce\xbd\xcf\x84\xce\xb9\xce\xb3\xcf\x81\xce\xac\xcf\x86\xce\xb7\xce\xba\xce\xb5!'),()=>showLink(url));\n"
"    return;\n"
"  }\n"
"  const ta=document.createElement('textarea');\n"
"  ta.value=url;ta.style.position='fixed';ta.style.opacity='0';\n"
"  document.body.appendChild(ta);ta.select();\n"
"  let ok=false;\n"
"  try{ok=document.execCommand('copy');}catch(e){}\n"
"  ta.remove();\n"
"  if(ok)toast('\xf0\x9f\x93\x8b \xce\x9f \xcf\x83\xcf\x8d\xce\xbd\xce\xb4\xce\xb5\xcf\x83\xce\xbc\xce\xbf\xcf\x82 \xce\xb1\xce\xbd\xcf\x84\xce\xb9\xce\xb3\xcf\x81\xce\xac\xcf\x86\xce\xb7\xce\xba\xce\xb5!');\n"
"  else showLink(url);\n"
"}\n"
"function showLink(url){prompt('\xce\x91\xce\xbd\xcf\x84\xce\xaf\xce\xb3\xcf\x81\xce\xb1\xcf\x88\xce\xb5 \xcf\x84\xce\xbf\xce\xbd \xcf\x83\xcf\x8d\xce\xbd\xce\xb4\xce\xb5\xcf\x83\xce\xbc\xce\xbf:',url);}\n"
"render();\n"
"updSync();\n"
"document.addEventListener('visibilitychange',()=>{if(!document.hidden)poll();});\n"
"</script>\n"
"</body>\n</html>");

  rqst->contentSize=pos;
  return 0;
}
