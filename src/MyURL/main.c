/*
AmmarServer , main executable

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
#include "../AmmServerlib/AmmServerlib.h"

#define MAX_BINDING_PORT 65534
#define MAX_INPUT_IP 256


#define DEFAULT_BINDING_PORT 8080  // <--- Change this to 80 if you want to bind to the default http port..!
char webserver_root[MAX_FILE_PATH]="public_html/"; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
//char webserver_root[MAX_FILE_PATH]="ammar.gr/"; //<- This is my dev dir.. itshould be commented or removed in stable release..
char templates_root[MAX_FILE_PATH]="public_html/templates/";

char service_root[128]="http://ammar.gr:8080/go";
char db_file[128]="myurl.db";


#define MAX_TO_SIZE 32
#define MAX_LONG_URL_SIZE 1024
#define MAX_LINKS 20000


struct URLDB
{
  char * long_url;
  unsigned long short_url;
};


struct AmmServer_Instance * myurl_server=0;

struct AmmServer_RH_Context create_url={0};
struct AmmServer_RH_Context goto_url={0};


char * default_failed = (char*)"http://ammar.gr/myloader/vfile.php?i=f2166b56f919fa75345991e73448febc-notyet_new.ogg";


unsigned int loaded_links=0;
struct URLDB links[MAX_LINKS]={{0}};


int is_an_unsafe_str(char * input,unsigned int input_length)
{
  int i=0;
  while (i<input_length)
  {
     if (input[i]<=14) { return 1;}
     ++i;
  }
  return 0;
}


/*
  -----------------------------------------------------------
                   This is the backbone of MyURL
   -----------------------------------------------------------
*/

int Append2MyURLDBFile(char * filename,char * LongURL,char * ShortURL);

unsigned long hashURL(char *str)
    {
        if (str==0) return 0;
        if (str[0]==0) return 0;

        unsigned long hash = 5381; //<- magic
        int c=1;

        while (c != 0)
        {
            c = *str++;
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        }

        return hash;
    }

unsigned long Add_MyURL(char * LongURL,char * ShortURL,int saveit)
{
  if (loaded_links>=MAX_LINKS) { return 0; }

  unsigned int long_url_length = strlen(LongURL);
  if (long_url_length>=MAX_LONG_URL_SIZE) { return 0; }

  unsigned int our_index=loaded_links++;
  unsigned long our_hash = hashURL(ShortURL);

  links[our_index].short_url=our_hash;
  links[our_index].long_url = ( char * ) malloc (sizeof(char) * (long_url_length+1) );
  if (links[our_index].long_url != 0 )
   {
     strncpy(links[our_index].long_url,LongURL,long_url_length);
     links[our_index].long_url[long_url_length]=0;  // null terminator :P

    if (saveit) { Append2MyURLDBFile(db_file,LongURL,ShortURL); }
   } else
   { fprintf(stderr,"Could not allocate space for a new string \n "); /*To prevent race conditions.. --loaded_links;*/ return 0; }

  return 1;
}

int Append2MyURLDBFile(char * filename,char * LongURL,char * ShortURL)
{
    if  ((ShortURL==0)||(LongURL==0)) { return 0; }
    FILE * pFile;
    pFile = fopen (filename,"a");
    if (pFile!=0)
    {
     //LongURL , ShortURL have stripped the newline character so there is no danger in just plain adding them with a \n seperator..!
     fprintf(pFile,"%s\n%s\n",LongURL,ShortURL);
     fclose (pFile);
     return 1;
    }

    return 0;
}

int LoadMyURLDBFile(char * filename)
{
    FILE * pFile;
    pFile = fopen (filename,"r");
    if (pFile!=0)
    {
       char LongURL[MAX_LONG_URL_SIZE]={0};
       char ShortURL[MAX_TO_SIZE]={0};

       unsigned int i=0;
       while (!feof(pFile))
        {
           memset(LongURL,0,MAX_LONG_URL_SIZE);
           memset(ShortURL,0,MAX_TO_SIZE);

           fscanf (pFile, "%s\n", LongURL);
           fscanf (pFile, "%s\n", ShortURL);
           Add_MyURL(LongURL,ShortURL,0 /*We dont want to reappend it :P*/);
           ++i;
           fprintf(stderr,"%u - Loaded Keyword %s \n",i,ShortURL);
        }
     fclose (pFile);
     return 1;
    }

    return 0;
}


char * Get_LongURL(char * ShortURL)
{
  unsigned long our_hash = hashURL(ShortURL);
  int i=0;
  while ( i < loaded_links )
   {
      if (our_hash==links[i].short_url) { return links[i].long_url; }
      ++i;
   }

  return default_failed;
}


/*
   -----------------------------------------------------------
   -----------------------------------------------------------
*/




/*
  -----------------------------------------------------------
            This is the HTML page output of MyURL
   -----------------------------------------------------------
*/

//This function prepares the content of  the url creator context
void * serve_create_url_page(char * content)
{
  memset(content,0,4096);

  strcpy(content,"<html><head><title>Welcome to MyURL</title></head><body><br><br><br><br><br><br><br><br><br><br><center><table border=5><tr><td><center><br><h2>Welcome to MyURL(<blink>Alpha</blink>)</h2><br>");

  strcat(content,"&nbsp;&nbsp;&nbsp;<form name=\"input\" action=\"go\" method=\"get\"> Long URL : <input type=\"text\" name=\"url\" /> Name: <input type=\"text\" name=\"to\"/>&nbsp;<input type=\"submit\" value=\"Submit\" /></form>&nbsp;&nbsp;&nbsp;");

  strcat(content,"</center><br><br></td></tr></table></center></body></html>");
  create_url.content_size=strlen(content);
  return 0;
}


//This function prepares the content of  stats context , ( stats.content )
void * serve_goto_url_page(char * content)
{
  //The url , to Long , Short eetc conventions are shit.. :P I should really make them better :p
  memset(content,0,4096);

  if  ( goto_url.GET_request != 0 )
    {
        char url[MAX_LONG_URL_SIZE]={0};
        char to[MAX_TO_SIZE]={0};
        //If both URL and NAME is set we want to assign a (short)to to a (long)url
        if ( _GET(myurl_server,&goto_url,"url",url,MAX_LONG_URL_SIZE) )
             {
               if ( _GET(myurl_server,&goto_url,"to",to,MAX_TO_SIZE) )
                {
                  //Assigning a (short)to to a (long)url
                  if ( (is_an_unsafe_str(to,strlen(to))) || (is_an_unsafe_str(url,strlen(url)) ) ) //There should be an internal length of the get argument instead of strlen!
                    {
                      sprintf(content,"<html><body>Bad Strings provided..</body></html>");
                    } else
                    {
                      Add_MyURL(url,to,1 /*We want to save it to disk..!*/);
                      sprintf(content,"<html><head><title>MyURL has shortened your URL</title></head><body><br><br><center>Your link is ready <a target=\"_new\" href=\"%s?to=%s\">%s?to=%s</a><br>Go on , make <a href=\"index.html\">another one</a></center></body></html>",service_root,to,service_root,to);
                    }
                  } else
                {
                 //No Point in a url without a to , here we could probably generate a random to !
                 strcpy(content,"<html><head><meta http-equiv=\"refresh\" content=\"2;URL='index.html'\"></head><body><h2>Error creating a new url</h2></body></html>");
                }
             } else
         //If only to is set it means we have ourselves somewhere to go to!
         if ( _GET(myurl_server,&goto_url,"to",to,MAX_TO_SIZE) )
             {
                sprintf(content,"<html><head><meta http-equiv=\"refresh\" content=\"0;URL='%s'\"></head><body></body></html>",Get_LongURL(to));
             }
    } else
    {
      strcpy(content,"<html><head><meta http-equiv=\"refresh\" content=\"0;URL='index.html'\"></head><body>Could not find a name to go to .. </body></html>");
    }

  goto_url.content_size=strlen(content);
  return 0;
}

/*
   -----------------------------------------------------------
   -----------------------------------------------------------
*/





//This function adds a Resource Handler for the pages and their callback functions
void init_dynamic_content()
{
  if (! AmmServer_AddResourceHandler(myurl_server,&create_url,"/index.html",webserver_root,4096,0,&serve_create_url_page,SAME_PAGE_FOR_ALL_CLIENTS) ) { fprintf(stderr,"Failed adding create page\n"); }
  AmmServer_DoNOTCacheResourceHandler(myurl_server,&create_url);

  if (! AmmServer_AddResourceHandler(myurl_server,&goto_url,"/go",webserver_root,4096,0,&serve_goto_url_page,DIFFERENT_PAGE_FOR_EACH_CLIENT) ) { fprintf(stderr,"Failed adding form testing page\n"); }
  AmmServer_DoNOTCacheResourceHandler(myurl_server,&goto_url);
}

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(myurl_server,&create_url,1);
    AmmServer_RemoveResourceHandler(myurl_server,&goto_url,1);
}



int main(int argc, char *argv[])
{
    printf("Ammar Server starting up\n");
    AmmServer_RegisterTerminationSignal();

    char bindIP[MAX_INPUT_IP];
    strcpy(bindIP,"0.0.0.0");

    unsigned int port=DEFAULT_BINDING_PORT;


    if ( argc <1 ) { fprintf(stderr,"Something weird is happening , argument zero should be executable path :S \n"); return 1; } else
    if ( argc <= 2 ) {  } else
     {
        if (strlen(argv[1])>=MAX_INPUT_IP) { fprintf(stderr,"Console argument for binding IP is too long..!\n"); } else
                                           { strncpy(bindIP,argv[1],MAX_INPUT_IP); }
        port=atoi(argv[2]);
        if (port>=MAX_BINDING_PORT) { port=DEFAULT_BINDING_PORT; }
     }
   if (argc>=3) { strncpy(webserver_root,argv[3],MAX_FILE_PATH); }
   if (argc>=4) { strncpy(templates_root,argv[4],MAX_FILE_PATH); }

    //Kick start AmmarServer , bind the ports , create the threads and get things going..!
    myurl_server = AmmServer_Start(bindIP,port,0,webserver_root,templates_root);
    if (!myurl_server) { fprintf(stderr,"Could not start myurl server\n"); exit(1); }

    if (LoadMyURLDBFile(db_file))
    {
      //Create dynamic content allocations and associate context to the correct files
      init_dynamic_content();

         while (AmmServer_Running(myurl_server))
           {
             //Main thread should just sleep and let the background threads do the hard work..!
             //In other applications the programmer could use the main thread to do anything he likes..
             //The only caveat is that he would takeup more CPU time from the server and that he would have to poll
             //the AmmServer_Running() call once in a while to make sure everything is in order
             //usleep(10000);
             sleep(1);
           }

      //Delete dynamic content allocations and remove stats.html and formtest.html from the server
      close_dynamic_content();
    } else
    {
      fprintf(stderr,"Could not load the database file , so exiting..!\n");
      fprintf(stderr,"!!!!!! If this is the first installation/run of the program please consider issuing `touch %s` to create an empty db file ..!!!!!!\n",db_file);
    }
    //Stop the server and clean state
    AmmServer_Stop(myurl_server);

    return 0;
}

