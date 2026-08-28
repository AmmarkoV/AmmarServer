/*
Availability , a tiny group-scheduling poll Service built on AmmarServer ( modeled on whenavailable.com )

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
#include "poll.h"
#include "manage.h"

#define DEFAULT_BINDING_PORT 8091
#define MAX_BINDING_PORT 65534

#define WEBSERVERROOT "data/"
char webserver_root[MAX_FILE_PATH]=WEBSERVERROOT;
char templates_root[MAX_FILE_PATH]=WEBSERVERROOT "/templates/";

struct AmmServer_RH_Context createPollView={0};
struct AmmServer_RH_Context voteView={0};
struct AmmServer_RH_Context submitVoteView={0};
struct AmmServer_RH_Context pollResultsView={0};
struct AmmServer_RH_Context manageView={0};
struct AmmServer_RH_Context closePollView={0};
struct AmmServer_RH_Context finalizePollView={0};


void init_dynamic_content()
{
  if ( ! loadAllPolls() ) { AmmServer_Error("Could not load polls\n"); }

  AmmServer_AddResourceHandler(default_server,&createPollView,"/createPoll.html",4096,0,&createPoll_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);
  AmmServer_AddResourceHandler(default_server,&voteView,"/vote.html",300000,0,&votePage_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  AmmServer_AddResourceHandler(default_server,&submitVoteView,"/submitVote.html",4096,0,&submitVote_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);
  AmmServer_AddResourceHandler(default_server,&pollResultsView,"/pollResults.html",70000,0,&pollResultsFragment_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  AmmServer_DoNOTCacheResourceHandler(default_server,&pollResultsView);

  AmmServer_AddResourceHandler(default_server,&manageView,"/manage.html",300000,0,&managePage_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  AmmServer_AddResourceHandler(default_server,&closePollView,"/closePoll.html",4096,0,&closePoll_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);
  AmmServer_AddResourceHandler(default_server,&finalizePollView,"/finalizePoll.html",4096,0,&finalizePoll_callback,DIFFERENT_PAGE_FOR_EACH_CLIENT|ENABLE_RECEIVING_FILES);
}

void close_dynamic_content()
{
  unloadAllPolls();

  AmmServer_RemoveResourceHandler(default_server,&createPollView,1);
  AmmServer_RemoveResourceHandler(default_server,&voteView,1);
  AmmServer_RemoveResourceHandler(default_server,&submitVoteView,1);
  AmmServer_RemoveResourceHandler(default_server,&pollResultsView,1);
  AmmServer_RemoveResourceHandler(default_server,&manageView,1);
  AmmServer_RemoveResourceHandler(default_server,&closePollView,1);
  AmmServer_RemoveResourceHandler(default_server,&finalizePollView,1);
}


int main(int argc, char *argv[])
{
    printf("\nAmmar Server %s starting up (Availability)..\n",AmmServer_Version());

    AmmServer_CheckIfHeaderBinaryAreTheSame(AMMAR_SERVER_HTTP_HEADER_SPEC);

    char bindIP[MAX_IP_STRING_SIZE];
    strncpy(bindIP,"0.0.0.0",MAX_IP_STRING_SIZE);

    unsigned int port=DEFAULT_BINDING_PORT;

    default_server = AmmServer_StartWithArgs(
                                             "availability",
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
