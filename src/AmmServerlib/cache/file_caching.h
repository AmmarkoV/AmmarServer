#ifndef FILE_CACHING_H_INCLUDED
#define FILE_CACHING_H_INCLUDED

#include "../AmmServerlib.h"

#include "../header_analysis/http_header_analysis.h"
#include "../tools/http_tools.h"

#include <sys/stat.h>
#include <time.h>



struct timestamp
{
   unsigned char hour;
   unsigned char minute;
   unsigned char second;
   unsigned char wday;
   unsigned char day;
   unsigned char month;
   unsigned int  year;
};



struct cache_item
{
   //If this is set it means this is a dynamic request item
   void * dynamicRequestCallbackFunction;
   struct AmmServer_RH_Context * dynamicRequest;

   //The whole cache item might be just a rule like do not cache
   unsigned char doNOTCacheRule;

   //in case we are talking about a regular system file ( dynamicRequest == 0 )
   //content should be in the next field
   char * content;
   unsigned long * contentSize;

   //if we are talking about a regular file that has been compressed content
   //should be stored in the next fields
   char * compressedContent;
   unsigned long * compressedContentSize;

   contentType contentTypeID; /*AS Declared in http_tools int GetContentType(char * filename,char * content_type) */

   //Modification time..!
   struct timestamp modification;
};


int freeMallocIfNeeded(char * mem,unsigned char free_is_needed);
int cache_ChangeRequestIfTemplateRequested(struct AmmServer_Instance * instance,char * request,char * templates_root);

int cache_AddFile(struct AmmServer_Instance * instance,char * filename,unsigned int * index,struct stat * last_modification);
int cache_AddMemoryBlock(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context);
int cache_AddDoNOTCacheRuleForResource(struct AmmServer_Instance * instance,char * filename);
int cache_RemoveContextAndResource(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context,unsigned char free_mem);


unsigned long cache_GetHashOfResource(struct AmmServer_Instance * instance,unsigned int index);

unsigned int cache_FindResource(struct AmmServer_Instance * instance,char * resource,unsigned int * index);
int cache_ResourceExists(struct AmmServer_Instance * instance,char * verified_filename,unsigned int * index);


int cache_Initialize(struct AmmServer_Instance * instance,unsigned int max_seperate_items , unsigned int max_total_allocation_MB , unsigned int max_allocation_per_entry_MB);
int cache_Destroy(struct AmmServer_Instance * instance);

char * cache_GetResource(
                          struct AmmServer_Instance * instance,
                          struct HTTPRequest * request,
                          char * verified_filename,
                          unsigned int * index,
                          unsigned long *filesize,
                          struct stat * last_modification,
                          unsigned char * compressionSupported,
                          unsigned char * freeContentAfterUsingIt
                        );


#endif // FILE_CACHING_H_INCLUDED
