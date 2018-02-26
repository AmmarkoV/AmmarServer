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
#include <pthread.h>
#include "../../AmmServerlib/AmmServerlib.h"


#define ENABLE_CAPTCHA_SYSTEM 1
#define USE_BINARY_SEARCH 1
#define MAX_BINDING_PORT 65534
#define MAX_CAPTCHA_JPG_SIZE 10 * 1024 //10KB more than enough
#define DEFAULT_BINDING_PORT 8080  // <--- Change this to 80 if you want to bind to the default http port..!


#if ENABLE_CAPTCHA_SYSTEM
#include "../../AmmCaptcha/AmmCaptcha.h"
#endif

char webserver_root[MAX_FILE_PATH]="public_html/"; // <- change this to the directory that contains your content if you dont want to use the default public_html dir..
//char webserver_root[MAX_FILE_PATH]="ammar.gr/"; //<- This is my dev dir.. itshould be commented or removed in stable release..
char templates_root[MAX_FILE_PATH]="public_html/templates/";

//It is retarded to have all these different strings for the same thing
//should really clear the code below though first and then keep only one of a kind
char service_filename_noslash[5]="go";
char service_filename[5]="/go";
char service_root[128]="http://myurl.ammar.gr/go";
char service_root_withoutfilename[128]="http://myurl.ammar.gr/";
char * default_failed = (char*)"http://myurl.ammar.gr/error.html";
//---------------------------------------------------------------

int enableMonitor=0;

char db_file[128]="db/myurl.db";
pthread_mutex_t db_fileLock;
pthread_mutex_t db_addIDLock;

char indexPagePath[128]="src/Services/MyURL/myurl.html";
char * indexPage=0;
unsigned int indexPageLength=0;

#define DYNAMIC_PAGES_MEMORY_COMMITED 4096
#define MAX_TO_SIZE 32
#define MAX_LONG_URL_SIZE 2048
#define MAX_LINKS 200000
#define LINK_ALLOCATION_STEP 5000
#define REGROUP_AFTER_X_UNSORTED_LINKS 1000


struct AmmServer_Instance * myurl_server=0;
struct AmmServer_RequestOverride_Context requestResolver={{0}};

struct AmmServer_RH_Context error_url={0};
struct AmmServer_RH_Context create_url={0};
struct AmmServer_RH_Context goto_url={0};
struct AmmServer_RH_Context captcha_url={0};

struct URLDB
{
  char * longURL;
  char * shortURL;
  unsigned long shortURLHash;
};


unsigned int loaded_links=0;
unsigned int sorted_links=0;
unsigned int allocated_links=0;
struct URLDB * links=0;


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

int Append2MyURLDBFile(char * filename,char * longURL,char * shortURL);

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


unsigned int allocateLinksIfNeeded()
{
  unsigned int makeAnAllocation=0;
  if (allocated_links>=MAX_LINKS)  { AmmServer_Warning("Reached constraint on links , will not commit more memory\n"); return 0;  }
  if (loaded_links>=allocated_links)  { makeAnAllocation=1; }

  if (makeAnAllocation)
  {
      fprintf(stderr,"Reallocating -> %u records \n",allocated_links+LINK_ALLOCATION_STEP);
      struct URLDB * newlinks = realloc(links, sizeof(struct URLDB) * (allocated_links+LINK_ALLOCATION_STEP) );
      if (newlinks!=0)
         {
            links=newlinks;
            memset(links+allocated_links,0,LINK_ALLOCATION_STEP);
            allocated_links+=LINK_ALLOCATION_STEP;
            return 1;
         }
  }

  return 0;
}

//This returns true if URLDB is sorted and false if not!
int isURLDBSorted()
{
  if (loaded_links==0) { return 1; }
  if (loaded_links==1) { return 1; }
  int i=1;
  sorted_links=0;
    while ( i < loaded_links )
    {
      if (links[i-1].shortURLHash>links[i].shortURLHash) { sorted_links=i-1; return 0; /*We got ourself a non sorted entry!*/ }
      ++i;
    }

  sorted_links=loaded_links-1;
  return 1;
}

int printURLDB()
{
  int i=0;
    while ( i < loaded_links )
    {
      fprintf(stderr,"%lu - %s - %s\n",links[i].shortURLHash,links[i].shortURL,links[i].longURL);
      ++i;
    }
  return 1;
}


/* qsort struct comparision function (price float field) ( See qsort call in main ) */
int struct_cmp_urldb_items(const void *a, const void *b)
{
    struct URLDB *ia = (struct URLDB *)a;
    struct URLDB *ib = (struct URLDB *)b;

    return(ia->shortURLHash > ib->shortURLHash);
}



 unsigned int Find_longURLSerial(char * shortURL,int * found)
{
  *found = 0;
  if (loaded_links==0) { return 0; }

  unsigned long our_hash = hashURL(shortURL);
  int i=loaded_links-1;
  while ( i > 0 )
   {
     if (our_hash==links[i].shortURLHash)
          {
            *found = 1;
            return i;
          }
      --i;
   }

   if (our_hash==links[0].shortURLHash)
          {
            *found = 1;
            return 0;
          }

  return 0;
}

//Binary Search
 unsigned int Find_longURL(char * shortURL,int * found)
{
  #if USE_BINARY_SEARCH
  *found = 0;
  if (shortURL==0) { return 0; }
  if (loaded_links==0) { return 0; }

  if (sorted_links!=0)
  {
   unsigned long our_hash = hashURL(shortURL);
   unsigned int binarySearchLastElement = sorted_links;
   unsigned int beg=0,mid=0,fin=binarySearchLastElement-1;
   while ( beg <= fin )
   {
     mid=(unsigned int) beg + ( (fin-beg)/2 );
     if (mid >= binarySearchLastElement)
        { AmmServer_Error("Binary Search overflowed ( beg %u mid %u fin %u ) , binarySearchLastElement %u \n",beg,mid,fin,binarySearchLastElement); break; } else
     if (our_hash<links[mid].shortURLHash) { fin=mid-1; } else
     if (our_hash>links[mid].shortURLHash) { beg=mid+1; } else
                                           {
                                             *found = 1;
                                             AmmServer_Success("Found %s using binary search\n",shortURL);
                                             return mid;
                                           }
   }
  }
  //TODO : Remove this message in the future -------------
  AmmServer_Warning("Binary Search couldn't find result , extending search to unsorted list\n");
  return Find_longURLSerial(shortURL,found);
  //----------------------------------------
  #else // USE_BINARY_SEARCH
   return Find_longURLSerial(shortURL,found);
  #endif

  return 0;
}

char * Get_longURL(char * shortURL)
{
  int found=0;
  //We search for a longURL for this shortURL
  unsigned int i = Find_longURL(shortURL,&found);
  //If we find a longURL for this shortURL , we return it
  if (found) {  return links[i].longURL; }
  //If we didnt find one we return the defaul failure url
  return default_failed;
}


int Append2MyURLDBFile(char * filename,char * longURL,char * shortURL)
{
    if  ((shortURL==0)||(longURL==0)) { return 0; }
    pthread_mutex_lock (&db_fileLock); // LOCK PROTECTED OPERATION -------------------------------------------
     int result = 0;
     FILE * pFile;
     pFile = fopen (filename,"a");
    if (pFile!=0)
    {
     //longURL , shortURL have stripped the newline character so there is no danger in just plain adding them with a \n seperator..!
     fprintf(pFile,"%s\n%s\n",longURL,shortURL);
     fclose (pFile);
     result=1;
    }
    pthread_mutex_unlock (&db_fileLock); // LOCK PROTECTED OPERATION -------------------------------------------
    return result;
}


int ReWriteMyURLDBFile(char * filename,struct URLDB * links,unsigned int loaded_links)
{
     pthread_mutex_lock (&db_fileLock); // LOCK PROTECTED OPERATION -------------------------------------------
     int result = 0;
     FILE * pFile;
     pFile = fopen (filename,"w");
     if (pFile!=0)
      {
        int i=0;
        while (i<loaded_links)
            {
                if (  (links[i].longURL!=0) && (links[i].shortURL!=0) )
                 {
                  fprintf(pFile,"%s\n%s\n",links[i].longURL,links[i].shortURL);
                 }
                ++i;
            }

     fclose (pFile);
     result=1;
    }
    pthread_mutex_unlock (&db_fileLock); // LOCK PROTECTED OPERATION -------------------------------------------
    return result;
}



int ResortDB(char * db_file,struct URLDB * links,unsigned int loaded_links)
{
  if ( !isURLDBSorted() )
        {
           AmmServer_Warning("URLDB is not sorted Sorting it now..!\n");
           qsort(links, loaded_links , sizeof(struct URLDB), struct_cmp_urldb_items);

           if ( !isURLDBSorted() ) { AmmServer_Warning("Could not sort URLDB ..! :( , exiting \n");
                                     return 0;
                                   } else
                                   {
                                     AmmServer_Success("Sorted URLDB \n");
                                     if ( ReWriteMyURLDBFile(db_file,links,loaded_links) )
                                     {
                                        AmmServer_Success("Saved db file \n");
                                     }
                                   }
        } else
        {
          AmmServer_Success("Presorted URLDB \n");
        }
   return 1;
}


unsigned long Add_MyURL(char * longURL,char * shortURL,int saveit)
{
  int shortURL_AlreadyExists=0;
  Find_longURL(shortURL,&shortURL_AlreadyExists);
  if (shortURL_AlreadyExists) { return 0; }

  if (loaded_links>=MAX_LINKS) { return 0; }
  allocateLinksIfNeeded();

  unsigned int long_url_length = strlen(longURL);
  if (long_url_length>=MAX_LONG_URL_SIZE) { return 0; }
  unsigned int sort_url_length = strlen(shortURL);
  if (sort_url_length>=MAX_TO_SIZE) { return 0; }

  pthread_mutex_lock (&db_addIDLock); // LOCK PROTECTED OPERATION -------------------------------------------
  //it might not seem like it but here we are doing two seperate operations
  //first we give our_index the loaded_links value , and then we increment loaded_links
  //of course this is a potential race condition where two threads assign themselves the same link
  //and we have an empty record after that , solved using a lock protection
  unsigned int our_index=loaded_links++;
  pthread_mutex_unlock (&db_addIDLock); // LOCK PROTECTED OPERATION -------------------------------------------

  links[our_index].longURL = ( char * ) malloc (sizeof(char) * (long_url_length+1) );  //+1 for null termination
  if ( links[our_index].longURL == 0 ) { AmmServer_Warning("Could not allocate space for a new string \n "); return 0; }
  links[our_index].shortURL = ( char * ) malloc (sizeof(char) * (sort_url_length+1) ); //+1 for null termination
  if ( links[our_index].shortURL == 0 ) { AmmServer_Warning("Could not allocate space for a new string \n "); return 0; }

  links[our_index].shortURLHash=hashURL(shortURL);

  strncpy(links[our_index].longURL,longURL,long_url_length);
  links[our_index].longURL[long_url_length]=0;  // null terminator :P

  strncpy(links[our_index].shortURL,shortURL,sort_url_length);
  links[our_index].shortURL[sort_url_length]=0;  // null terminator :P

  if (saveit) { Append2MyURLDBFile(db_file,longURL,shortURL); }

  if ( REGROUP_AFTER_X_UNSORTED_LINKS <= loaded_links-sorted_links )
  {
      ResortDB(db_file,links,loaded_links);
  }

  return 1;
}


int LoadMyURLDBFile(char * filename)
{
    fprintf(stderr,"Trying to load DB from file %s \n",filename);
    /*
    char curdir[1024]={0};
    getcwd(curdir,1024);
    fprintf(stderr,"Current Directory is %s \n",curdir);
    */

    FILE * pFile;
    pFile = fopen (filename,"r");
    if (pFile!=0)
    {
       char longURL[MAX_LONG_URL_SIZE]={0};
       char shortURL[MAX_TO_SIZE]={0};

       unsigned int i=0;
       while (!feof(pFile))
        {
           memset(longURL,0,MAX_LONG_URL_SIZE);
           memset(shortURL,0,MAX_TO_SIZE);

           int res = fscanf (pFile, "%s\n", longURL);
           if (res!=1) { AmmServer_Warning("error (?) reading longURL (%s) item line %u of file %s ",longURL,i,filename); }
               res = fscanf (pFile, "%s\n", shortURL);
           if (res!=1) { AmmServer_Warning("error (?) reading shortURL (%s) item line %u of file %s ",shortURL,i,filename); }

           Add_MyURL(longURL,shortURL,0 /*We dont want to reappend it :P*/);
           ++i;
        }
     fclose (pFile);
     return 1;
    }

    return 0;
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
void * serve_error_url_page(struct AmmServer_DynamicRequest  * rqst)
{
  memset(rqst->content,0,DYNAMIC_PAGES_MEMORY_COMMITED);
  snprintf(rqst->content,rqst->MAXcontentSize,"<html><head><body><center><br><br><br><br><br><h2>Could not find your URL , <a href=\"javascript:history.go(-1)\">go back</a> , <a href=\"%s\">MyURL home page</a></h2></center></body></html>",service_root_withoutfilename);
  rqst->contentSize=strlen(rqst->content);
  rqst->content[rqst->contentSize]=0;
  return 0;
}

//This overrides serves back the captcha using AmmCaptch!
void * serve_captcha_page(struct AmmServer_DynamicRequest  * rqst)
{
  #if ENABLE_CAPTCHA_SYSTEM
  char captchaIDStr[MAX_LONG_URL_SIZE]={0};
  if ( _GETcpy(rqst,"id",captchaIDStr,MAX_LONG_URL_SIZE) ) { fprintf(stderr,"Captcha ID for image requested %s \n",captchaIDStr); }
  unsigned int captchaID = atoi(captchaIDStr);

  rqst->contentSize=rqst->MAXcontentSize;
  AmmServer_Warning("doing captcha callback..");
  AmmCaptcha_getCaptchaFrame(captchaID,rqst->content,&rqst->contentSize);
  #else
   snprintf(rqst->content,rqst->MAXcontentSize,"ENABLE_CAPTCHA_SYSTEM");
   rqst->contentSize=strlen(rqst->content);
  #endif
  return 0;
}


//This function prepares the content of  the url creator context
void * serve_create_url_page(struct AmmServer_DynamicRequest  * rqst)
{
  strncpy(rqst->content,indexPage,indexPageLength);
  rqst->content[indexPageLength]=0;
  rqst->contentSize=indexPageLength;

  char val[132+1]={0};
  snprintf(val ,132, "%u",loaded_links);

  //Cast our output page to a memory handler structure
  struct AmmServer_MemoryHandler create_url_page_mh={0};
  create_url_page_mh.content=rqst->content;
  create_url_page_mh.contentSize=rqst->contentSize;
  create_url_page_mh.contentCurrentLength=rqst->contentSize;

  //Replace the number of served links..
  AmmServer_ReplaceVariableInMemoryHandler(&create_url_page_mh,"$NUMBER_OF_LINKS$",val);

  //Update the new content size ( our page became a little smaller )..!
  rqst->contentSize=strlen(rqst->content);

  return 0;
}


//This function prepares the content of  stats context , ( stats.content )
void * serve_goto_url_page(struct AmmServer_DynamicRequest  * rqst)
{
  //The url , to Long , Short eetc conventions are shit.. :P I should really make them better :p
  memset(rqst->content,0,DYNAMIC_PAGES_MEMORY_COMMITED);

  if  (
        ( _GETexists(rqst,"url") ) ||
        ( _GETexists(rqst,"to") )
      )
    {
        char url[MAX_LONG_URL_SIZE]={0};
        char to[MAX_TO_SIZE]={0};
        char captchaReply[MAX_LONG_URL_SIZE]={0};
        char captchaIDStr[MAX_LONG_URL_SIZE]={0};
        //If both URL and NAME is set we want to assign a (short)to to a (long)url
        if ( _GETcpy(rqst,"url",url,MAX_LONG_URL_SIZE) )
             {
               #if ENABLE_CAPTCHA_SYSTEM
               if ( _GETcpy(rqst,"captchaID",captchaIDStr,MAX_LONG_URL_SIZE) )
                { fprintf(stderr,"Captcha ID submited %s \n",captchaIDStr); }
               if ( _GETcpy(rqst,"captcha",captchaReply,MAX_LONG_URL_SIZE) )
                { fprintf(stderr,"Captcha submited %s \n",captchaReply); }

               unsigned int captchaID = atoi(captchaIDStr);
               if ( ! AmmCaptcha_isReplyCorrect(captchaID , captchaReply) )
                {
                 strncpy(rqst->content,"<html><head><meta http-equiv=\"refresh\" content=\"2;URL='index.html'\"></head><body><h2>Please solve the captcha and try again</h2></body></html>",rqst->MAXcontentSize);
                 AmmServer_SignalCountAsBadClientBehaviour(rqst);
                } else
               #endif

               if ( _GETcpy(rqst,"to",to,MAX_TO_SIZE) )
                {
                  //Assigning a (short)to to a (long)url
                  if ( (is_an_unsafe_str(to,strlen(to))) || (is_an_unsafe_str(url,strlen(url)) ) ) //There should be an internal length of the get argument instead of strlen!
                    {
                      snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>Bad Strings provided..</body></html>");
                    } else
                    {
                      Add_MyURL(url,to,1 /*We want to save it to disk..!*/);
                      snprintf(rqst->content,rqst->MAXcontentSize,"<html><head><title>MyURL has shortened your URL</title></head><body><br><br><center>Your link is ready \
                              <a target=\"_new\" href=\"%s%s\">%s%s</a> \
                              <br>Go on , make <a href=\"index.html\">another one</a></center></body></html>",service_root_withoutfilename,to,service_root_withoutfilename, to);
                    }
                } else
                {
                 //No Point in a url without a to , here we could probably generate a random to !
                 strncpy(rqst->content,"<html><head><meta http-equiv=\"refresh\" content=\"2;URL='index.html'\"></head><body><h2>Error creating a new url</h2></body></html>",rqst->MAXcontentSize);
                }
             } else
         //If only to is set it means we have ourselves somewhere to go to!
         if ( _GETcpy(rqst,"to",to,MAX_TO_SIZE) )
             {
                snprintf(rqst->content,rqst->MAXcontentSize,"<html><head><meta http-equiv=\"refresh\" content=\"0;URL='%s'\"></head><body></body></html>",Get_longURL(to));
             }
    } else
    {
      strncpy(rqst->content,"<html><head><meta http-equiv=\"refresh\" content=\"0;URL='index.html'\"></head><body>Could not find a name to go to .. </body></html>",rqst->MAXcontentSize);
    }

  rqst->contentSize=strlen(rqst->content);
  return 0;
}

/*
   -----------------------------------------------------------
   -----------------------------------------------------------
*/


/**
* @brief  This is a custom resolver for requests
*          When a new message is received this gets called and depending on the resource requested
*          we camouflage the request in a way that we want to make urls more user friendly
*          so a request for /whatever is converted to /go?to=whatever in ourcase :) ( since we have a url shortner )
*/
void resolveRequest(void * request)
{
  struct AmmServer_RequestOverride_Context * rqstContext = (struct AmmServer_RequestOverride_Context *) request;
  struct HTTPHeader * rqst = rqstContext->request;
  //AmmServer_Warning("With URI : %s \n Filtered URI : %s \n GET Request : %s \n",rqst->resource,rqst->verified_local_resource, rqst->GETquery);

  if (strcmp("/favicon.ico",rqst->resource)==0 ) { return; /*Client requested favicon.ico , no resolving to do */ } else
  if (strcmp("/index.html",rqst->resource)==0 )  { return; /*Client requested index.html , no resolving to do */  } else
  if (strcmp("/error.html",rqst->resource)==0 )  { return; /*Client requested error.html , no resolving to do */  } else
  if (strcmp("/myurl.png",rqst->resource)==0 )   { return; /*Client requested myurl.png , no resolving to do */  } else
  if (strcmp("/captcha.jpg",rqst->resource)==0 ) { return; /*Client requested captcha.jpg , no resolving to do */  } else
  if (strcmp("/",rqst->resource)==0 ) {  return; /*Client requested index.html , no resolving to do */  } else
  if ( (strncmp(service_filename,rqst->resource,3)==0) && (strlen(rqst->resource)==3) ) { return; /*Client requested go , no resolving to do */  } else
         {
             if (rqst->resource==0) { AmmServer_Warning("Request doesnt have a valid resource "); return;  }

             unsigned int newlength = strlen(rqst->resource) + 10;
             if ( (newlength>MAX_TO_SIZE) || (newlength>MAX_QUERY) )
               { AmmServer_Warning("Request (%u) is larger than limit of to fields %u ",newlength,MAX_TO_SIZE); }

             AmmServer_Warning("Detected new kind of page , should make it /go?to=%s",rqst->resource+1);
             //For now they are static if (rqst->GETquery!=0) { free(rqst->GETquery); rqst->GETquery=0; }
             //rqst->GETquery = ( char * ) malloc ( sizeof(char) * newlength);

             rqst->sizeOfExtraDataThatWillNeedToBeDeallocated = strlen(rqst->resource)+32;
             rqst->extraDataThatWillNeedToBeDeallocated       = (char * ) malloc (sizeof(char) * rqst->sizeOfExtraDataThatWillNeedToBeDeallocated);
             if (rqst->extraDataThatWillNeedToBeDeallocated!=0)
             {
              snprintf(rqst->extraDataThatWillNeedToBeDeallocated,rqst->sizeOfExtraDataThatWillNeedToBeDeallocated-1,"to=%s",rqst->resource+1 /* +1 To avoid the slash / */);
              rqst->extraDataThatWillNeedToBeDeallocated[2]=0;

              rqst->GETItemNumber=1;
              rqst->GETItem[0].name=rqst->extraDataThatWillNeedToBeDeallocated;     //<-    *THIS POINTS SOMEWHERE INSIDE headerRAW , or is 0 *
              rqst->GETItem[0].nameSize=2;
              rqst->GETItem[0].value=rqst->extraDataThatWillNeedToBeDeallocated+3;  //<-    *THIS POINTS SOMEWHERE INSIDE headerRAW , or is 0 *
              rqst->GETItem[0].valueSize=strlen(rqst->resource+1);
              rqst->GETItem[0].reallocateOnHeaderRAWResize=0; //We have allocated this on our own so we don't need to realloc it
              AmmServer_Warning("Redirecting %s %s\n",rqst->GETItem[0].name,rqst->GETItem[0].value);

              //if (rqst->resource!=0) { free(rqst->resource); rqst->resource=0; }
              //rqst->resource = ( char * ) malloc (sizeof(char) * 4); //+3 chars + nulltermination
              snprintf(rqst->resource,MAX_RESOURCE,"%s",service_filename);
              snprintf(rqst->verified_local_resource,MAX_FILE_PATH,"%s%s",webserver_root,service_filename_noslash);
              AmmServer_Warning("With URI : %s \n Filtered URI : %s  \n",rqst->resource,rqst->verified_local_resource);
             }
          }
}



//This function adds a Resource Handler for the pages and their callback functions
void init_dynamic_content()
{
  if (enableMonitor)
  {
    AmmServer_Warning("Enabling monitor\n");
    AmmServer_EnableMonitor(myurl_server);
  }


  indexPage=AmmServer_ReadFileToMemory(indexPagePath,&indexPageLength);
  if (indexPage==0) { AmmServer_Error("Could not find Index Page file %s ",indexPagePath); }

  AmmServer_AddResourceHandler(myurl_server,&create_url,"/index.html",indexPageLength,0,&serve_create_url_page,SAME_PAGE_FOR_ALL_CLIENTS);
  AmmServer_DoNOTCacheResourceHandler(myurl_server,&create_url);

  AmmServer_AddResourceHandler(myurl_server,&goto_url,"/go",DYNAMIC_PAGES_MEMORY_COMMITED,0,&serve_goto_url_page,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  AmmServer_DoNOTCacheResourceHandler(myurl_server,&goto_url);

  AmmServer_AddResourceHandler(myurl_server,&error_url,"/error.html",DYNAMIC_PAGES_MEMORY_COMMITED,0,&serve_error_url_page,DIFFERENT_PAGE_FOR_EACH_CLIENT);

  #if ENABLE_CAPTCHA_SYSTEM
  AmmServer_AddResourceHandler(myurl_server,&captcha_url,"/captcha.jpg",MAX_CAPTCHA_JPG_SIZE,0,&serve_captcha_page,DIFFERENT_PAGE_FOR_EACH_CLIENT);
  AmmServer_DoNOTCacheResourceHandler(myurl_server,&captcha_url);
  #endif


  if (!AmmServer_AddRequestHandler(myurl_server,&requestResolver,"GET",&resolveRequest) )
      { AmmServer_Warning("Failed adding request handler for testing page\n"); }

}

//This function destroys all Resource Handlers and free's all allocated memory..!
void close_dynamic_content()
{
    AmmServer_RemoveResourceHandler(myurl_server,&create_url,1);
    AmmServer_RemoveResourceHandler(myurl_server,&goto_url,1);
    AmmServer_RemoveResourceHandler(myurl_server,&error_url,1);
}



int main(int argc, char *argv[])
{
    printf("Ammar Server starting up\n");
    //Check binary and header spec
    AmmServer_CheckIfHeaderBinaryAreTheSame(AMMAR_SERVER_HTTP_HEADER_SPEC);
    //Register termination signal for when we receive SIGKILL etc
    AmmServer_RegisterTerminationSignal(&close_dynamic_content);

    char bindIP[MAX_IP_STRING_SIZE]={0};
    strncpy(bindIP,"0.0.0.0",MAX_IP_STRING_SIZE);

    unsigned int port=DEFAULT_BINDING_PORT;

  #if ENABLE_CAPTCHA_SYSTEM
    AmmServer_Warning("Initializing captcha system\n");
    if (!AmmCaptcha_initialize("src/AmmCaptcha/font.ppm","src/AmmCaptcha/ourDictionaryCaptcha.txt"))
        {
          AmmServer_Error("Could not initialize Captcha System");
          exit(1);
        }
  #endif



    pthread_mutex_init(&db_fileLock,0);
    pthread_mutex_init(&db_addIDLock,0);

    //Kick start AmmarServer , bind the ports , create the threads and get things going..!
    myurl_server =  AmmServer_StartWithArgs(
                                             "myurl",
                                              argc,argv , //The internal server will use the arguments to change settings
                                              //If you don't want this look at the AmmServer_Start call
                                              bindIP,
                                              port,
                                              0, /*This means we don't want a specific configuration file*/
                                              webserver_root,
                                              templates_root
                                              );

    if (!myurl_server) { AmmServer_Error("Could not start myurl server\n"); exit(1); }

    if (LoadMyURLDBFile(db_file))
    {
      //Try to enforce beeing sorted ,if we fail there is no point in going on , since binary search will fail
      if ( !ResortDB(db_file,links,loaded_links) ) { return 1; }

      //Create dynamic content allocations and associate context to the correct files
      init_dynamic_content();

         while (AmmServer_Running(myurl_server))
           {
             //Main thread should just sleep and let the background threads do the hard work..!
             sleep(1);
           }

      //Delete dynamic content allocations and remove stats.html and formtest.html from the server
      close_dynamic_content();
    } else
    {
      AmmServer_Error("Could not load the database file , so exiting..!\n");
      AmmServer_Error("!!!!!! If this is the first installation/run of the program please consider issuing `touch %s` to create an empty db file ..!!!!!!\n",db_file);
    }
    //Stop the server and clean state
    AmmServer_Stop(myurl_server);
    #if ENABLE_CAPTCHA_SYSTEM
     AmmCaptcha_destroy();
    #endif // ENABLE_CAPTCHA_SYSTEM

    pthread_mutex_destroy(&db_fileLock);
    pthread_mutex_destroy(&db_addIDLock);

    return 0;
}

