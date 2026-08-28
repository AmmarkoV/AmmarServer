/*
ShareTex , a tiny multiuser LaTeX collaboration Service built on AmmarServer

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "../../AmmServerlib/AmmServerlib.h"

#include "state.h"
#include "auth.h"
#include "project.h"
#include "editor.h"
#include "cursor.h"
#include "compile.h"

#define DEFAULT_BINDING_PORT 8090
#define MAX_BINDING_PORT 65534

#define WEBSERVERROOT "data/"
char webserver_root[MAX_FILE_PATH]=WEBSERVERROOT;
char templates_root[MAX_FILE_PATH]=WEBSERVERROOT "/templates/";

struct AmmServer_RH_Context doSignupView={0};
struct AmmServer_RH_Context doLoginView={0};
struct AmmServer_RH_Context dashboardView={0};
struct AmmServer_RH_Context createProjectView={0};
struct AmmServer_RH_Context shareView={0};
struct AmmServer_RH_Context editorView={0};
struct AmmServer_RH_Context getFileContentView={0};
struct AmmServer_RH_Context saveFileContentView={0};
struct AmmServer_RH_Context newFileView={0};
struct AmmServer_RH_Context postCursorView={0};
struct AmmServer_RH_Context pollCursorsView={0};
struct AmmServer_RH_Context compileView={0};


void init_dynamic_content()
{
  if ( ! initializeLoginSystem() ) { AmmServer_Error("Could not initialize user accounts\n"); }
  if ( ! loadAllProjects() )       { AmmServer_Error("Could not load projects\n"); }

  AmmServer_AddResourceHandler(default_server,&doSignupView,"/doSignup.html",4096,0,&signup_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);
  AmmServer_AddResourceHandler(default_server,&doLoginView,"/doLogin.html",4096,0,&login_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);

  AmmServer_AddResourceHandler(default_server,&dashboardView,"/dashboard.html",131072,0,&dashboard_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  AmmServer_AddResourceHandler(default_server,&createProjectView,"/createProject.html",4096,0,&createProject_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);
  AmmServer_AddResourceHandler(default_server,&shareView,"/share.html",4096,0,&share_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);

  AmmServer_AddResourceHandler(default_server,&editorView,"/editor.html",2*1024*1024,0,&editorPage_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  AmmServer_AddResourceHandler(default_server,&getFileContentView,"/getFileContent.html",2*1024*1024,0,&getFileContent_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  AmmServer_AddResourceHandler(default_server,&saveFileContentView,"/saveFileContent.html",4096,0,&saveFileContent_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);
  AmmServer_AddResourceHandler(default_server,&newFileView,"/newFile.html",4096,0,&newFile_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);

  AmmServer_AddResourceHandler(default_server,&postCursorView,"/postCursor.html",1024,0,&postCursor_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);
  AmmServer_AddResourceHandler(default_server,&pollCursorsView,"/pollCursors.html",8192,0,&pollCursors_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  AmmServer_DoNOTCacheResourceHandler(default_server,&pollCursorsView);
  AmmServer_DoNOTCacheResourceHandler(default_server,&getFileContentView);

  AmmServer_AddResourceHandler(default_server,&compileView,"/compile.html",16384,0,&compile_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);
}

void close_dynamic_content()
{
  unloadAllProjects();
  stopLoginSystem();

  AmmServer_RemoveResourceHandler(default_server,&doSignupView,1);
  AmmServer_RemoveResourceHandler(default_server,&doLoginView,1);
  AmmServer_RemoveResourceHandler(default_server,&dashboardView,1);
  AmmServer_RemoveResourceHandler(default_server,&createProjectView,1);
  AmmServer_RemoveResourceHandler(default_server,&shareView,1);
  AmmServer_RemoveResourceHandler(default_server,&editorView,1);
  AmmServer_RemoveResourceHandler(default_server,&getFileContentView,1);
  AmmServer_RemoveResourceHandler(default_server,&saveFileContentView,1);
  AmmServer_RemoveResourceHandler(default_server,&newFileView,1);
  AmmServer_RemoveResourceHandler(default_server,&postCursorView,1);
  AmmServer_RemoveResourceHandler(default_server,&pollCursorsView,1);
  AmmServer_RemoveResourceHandler(default_server,&compileView,1);
}


int main(int argc, char *argv[])
{
    printf("\nAmmar Server %s starting up (ShareTex)..\n",AmmServer_Version());

    AmmServer_CheckIfHeaderBinaryAreTheSame(AMMAR_SERVER_HTTP_HEADER_SPEC);

    char bindIP[MAX_IP_STRING_SIZE];
    strncpy(bindIP,"0.0.0.0",MAX_IP_STRING_SIZE);

    unsigned int port=DEFAULT_BINDING_PORT;

    default_server = AmmServer_StartWithArgs(
                                             "sharetex",
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
