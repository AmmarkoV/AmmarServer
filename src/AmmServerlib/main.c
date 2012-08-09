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
#include <stdio.h>
#include <stdlib.h>
#include "version.h"
#include "AmmServerlib.h"
#include "server_threads.h"
#include "file_caching.h"



int AmmServer_Start(char * ip,unsigned int port,char * web_root_path,char * templates_root_path)
{
  fprintf(stderr,"Binding AmmarServer v%s to %s:%u\n",FULLVERSION_STRING,ip,port);
  fprintf(stderr,"\n\nDISCLAIMER : \n");
  fprintf(stderr,"Please note that this server version is not thoroughly\n");
  fprintf(stderr," pen-tested so it is not meant for production deployment..\n");

  fprintf(stderr,"Bug reports and feedback are very welcome.. \n");
  fprintf(stderr,"via https://github.com/AmmarkoV/AmmarServer/issues\n\n");

  fprintf(stderr,"TODO: Add configuration file ammServ.conf parsing..\n");
  fprintf(stderr,"TODO: Implement resume capabilities..\n");
  fprintf(stderr,"TODO: Add detailed input header parsing\n");
  fprintf(stderr,"TODO: Improve directory listings\n");
  fprintf(stderr,"TODO: Improve implemented file caching mechanism\n");
  fprintf(stderr,"TODO: Improve dynamic content handling ( coming from programs statically linked to the webserver ) ..\n");
  fprintf(stderr,"TODO: Add apache like logging capabilities\n");
  fprintf(stderr,"TODO: Implement gzip file compression\n");

  InitializeCache(2000/*Seperate items*/,64/*MB*/);
  return StartHTTPServer(ip,port,web_root_path,templates_root_path);
}

int AmmServer_Stop()
{
  DestroyCache();
  return StopHTTPServer();
}

int AmmServer_Running()
{
  return server_running;
}

int AmmServer_AddResourceHandler(char * web_root_path,char * resource_name,char * content_memory,unsigned long * content_memory_size,void * prepare_content_callback)
{
  return AddDirectResourceToCache(web_root_path,resource_name,content_memory,content_memory_size,prepare_content_callback);
}

