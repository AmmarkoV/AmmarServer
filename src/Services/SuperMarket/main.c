/*
SuperMarket , a shared shopping-list Service built on AmmarServer ( a C clone of the standalone go.php tool
in MyScripts/Tools/SuperMarket , wire-compatible with its supermarket.py CLI front-end )

URLs: http://ammar.gr

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
#include "../../AmmServerlib/AmmServerlib.h"

#include "state.h"
#include "json.h"
#include "page.h"
#include "photo.h"

#define DEFAULT_BINDING_PORT 8092
#define MAX_BINDING_PORT 65534

#define WEBSERVERROOT "public_html/"
char webserver_root[MAX_FILE_PATH]=WEBSERVERROOT;
char templates_root[MAX_FILE_PATH]=WEBSERVERROOT "templates/";

#define GO_PAGE_BUFFER_CAPACITY (512*1024) //room for the full mobile UI + a few hundred items worth of embedded JSON

//30MB : comfortably above a phone-camera JPEG , matches the PHP tool's raised upload limit
#define MAX_UPLOAD_TRANSACTION_SIZE (30*1024*1024)

struct AmmServer_Instance * default_server=0;
struct AmmServer_RH_Context goView={0};


void * go_callback(struct AmmServer_DynamicRequest * rqst)
{
  char rawToken[TOKEN_BUF_SIZE]={0};
  _GETcpy(rqst,"i",rawToken,sizeof(rawToken));
  char token[TOKEN_BUF_SIZE]={0};
  int hasToken=sanitizeToken(rawToken,token,sizeof(token));

  if (!hasToken)
  {
    if (_GETexists(rqst,"new")) { return renderNewCartRedirect(rqst); }
    return renderLandingPage(rqst);
  }

  if (_GETexists(rqst,"img"))
  {
    char rawImgID[64]={0};
    _GETcpy(rqst,"img",rawImgID,sizeof(rawImgID));
    return photoStream_callback(rqst,token,rawImgID);
  }

  if (_GETexists(rqst,"rev"))
  {
    unsigned int rev=peekCartRev(token);
    snprintf(rqst->content,rqst->MAXcontentSize,"{\"rev\":%u}",rev);
    rqst->contentSize=strlen(rqst->content);
    return 0;
  }

  if (_POSTexists(rqst,"a"))
  {
    char action[16]={0};
    _POSTcpy(rqst,"a",action,sizeof(action));

    if (strcmp(action,"img")==0) { return photoUpload_callback(rqst,token); }

    struct cartActionParams params={0};

    char rawItemID[64]={0};
    _POSTcpy(rqst,"id",rawItemID,sizeof(rawItemID));
    sanitizeItemID(rawItemID,params.itemID,sizeof(params.itemID));

    if (strcmp(action,"add")==0)
    {
      params.action=ACTION_ADD;
      _POSTcpy(rqst,"n",params.text,sizeof(params.text));
    } else
    if (strcmp(action,"qty")==0)
    {
      params.action=ACTION_QTY;
      char dStr[16]={0};
      if ( _POSTcpy(rqst,"d",dStr,sizeof(dStr)) && (dStr[0]!=0) )
      {
        params.hasDelta=1; params.delta=atoi(dStr);
      } else
      {
        char vStr[16]={0};
        _POSTcpy(rqst,"v",vStr,sizeof(vStr));
        params.hasValue=1; params.value=atoi(vStr);
      }
    } else
    if (strcmp(action,"toggle")==0) { params.action=ACTION_TOGGLE; } else
    if (strcmp(action,"del")==0)    { params.action=ACTION_DEL; } else
    if (strcmp(action,"imgdel")==0) { params.action=ACTION_IMGDEL; } else
    if (strcmp(action,"name")==0)
    {
      params.action=ACTION_NAME;
      _POSTcpy(rqst,"n",params.text,sizeof(params.text));
    } else
    {
      params.action=ACTION_NOOP; //unrecognised action : still bumps rev , exactly like go.php's switch() falling through with no case matched
    }

    struct cart cart;
    withCart(token,&cart,&params);
    buildCartJSON(token,&cart,rqst->content,rqst->MAXcontentSize);
    rqst->contentSize=strlen(rqst->content);
    return 0;
  }

  //Plain GET : materialize/seed the cart ( go.php's with_cart() always writes back , even for a pure read )
  struct cart cart;
  withCart(token,&cart,0);

  if (_GETexists(rqst,"api"))
  {
    buildCartJSON(token,&cart,rqst->content,rqst->MAXcontentSize);
    rqst->contentSize=strlen(rqst->content);
    return 0;
  }

  return renderCartPage(rqst,token,&cart);
}


void init_dynamic_content()
{
  ensureStorageDirectories();

  AmmServer_SetIntSettingValue(default_server,AMMSET_MAX_POST_TRANSACTION_SIZE,MAX_UPLOAD_TRANSACTION_SIZE);

  AmmServer_AddResourceHandler(default_server,&goView,"/go.php",GO_PAGE_BUFFER_CAPACITY,0,&go_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);
  AmmServer_DoNOTCacheResourceHandler(default_server,&goView);
}

void close_dynamic_content()
{
  AmmServer_RemoveResourceHandler(default_server,&goView,1);
}


int main(int argc, char *argv[])
{
    printf("\nAmmar Server %s starting up (SuperMarket)..\n",AmmServer_Version());

    AmmServer_CheckIfHeaderBinaryAreTheSame(AMMAR_SERVER_HTTP_HEADER_SPEC);

    char bindIP[MAX_IP_STRING_SIZE];
    strncpy(bindIP,"0.0.0.0",MAX_IP_STRING_SIZE);

    unsigned int port=DEFAULT_BINDING_PORT;

    default_server = AmmServer_StartWithArgs(
                                             "supermarket",
                                              argc,argv,
                                              bindIP,
                                              port,
                                              0,
                                              webserver_root,
                                              templates_root
                                              );

    if (!default_server) { AmmServer_Error("Could not start server , shutting down everything.."); exit(1); }

    init_dynamic_content();

         while ( (AmmServer_Running(default_server))  )
           {
             sleep(1);
           }

    close_dynamic_content();

    AmmServer_Stop(default_server);
    AmmServer_Warning("Ammar Server stopped\n");

    return 0;
}
