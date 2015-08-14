/** @file file_caching.h
* @brief Central cache of AmmarServer , it reads/indexes and swaps resources asked by clients for fast performance
* @author Ammar Qammaz (AmmarkoV)
* @bug File caching relies on hashmap for storing data , so it relies on optimizations done there for seek time optimization , other than that there needs to be a clean-up and code quality improvement
*/

#ifndef FILE_CACHING_H_INCLUDED
#define FILE_CACHING_H_INCLUDED

#include "../AmmServerlib.h"

#include "../header_analysis/http_header_analysis.h"
#include "../tools/http_tools.h"

#include <sys/stat.h>
#include <time.h>

/** @brief Timestamp for a cache item entry */
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

/** @brief A cache item and all it's contents */
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



/**
* @brief Tool to count total memory usage after a free operation
* @ingroup cache
* @param An AmmarServer Instance
* @param Size of memory being freed
* @bug cache_CountMemoryUsageFreeOperation should have a mutex lock so that it is well defined on massively parallel operations
* @retval 1=Ok,0=Failed*/
int cache_CountMemoryUsageFreeOperation(struct AmmServer_Instance * instance , unsigned long freedSize);


/**
* @brief Tool to count total memory usage after a memory allocation operation
* @ingroup cache
* @param An AmmarServer Instance
* @param Size of memory being allocated
* @bug cache_CountMemoryUsageAllocateOperation should have a mutex lock so that it is well defined on massively parallel operations
* @retval 1=Ok,0=Failed*/
int cache_CountMemoryUsageAllocateOperation(struct AmmServer_Instance * instance , unsigned long allocatedSize);


/**
* @brief Tool to check if a malloc'ed chunk of memory should be freed
* @ingroup cache
* @param Pointer to memory
* @param Flag that signals if the pointer should be freed or not
* @retval 1=Ok,0=Failed*/
int freeMallocIfNeeded(char * mem,unsigned char free_is_needed);

/**
* @brief The role of request caching is to intercept incoming requests and if they are referring
         to an internal resource using the TemplatesInternalURI URI we want to redirect the request
         to our templates folder ..!
         If the request was indeed a change request returns 1 else 0
* @ingroup cache
* @param An AmmarServer Instance
* @param String of Request
* @param Maximum size of request string
* @param Filename pointing to directory that contains templates
* @retval 1=If request was for a template and it got changed ,0= not changed request*/
int cache_ChangeRequestIfTemplateRequested(struct AmmServer_Instance * instance,char * request,unsigned int maxRequest,char * templates_root);

/**
* @brief Add a filesystem file to cache
* @ingroup cache
* @param An AmmarServer Instance
* @param Filename pointing to the file to be added to cache
* @param Output index number of cache item
* @param Output time of last modification
* @retval 1=Found,0=Failed*/
int cache_AddFile(struct AmmServer_Instance * instance,const char * filename,unsigned int * index,struct stat * last_modification);

/**
* @brief Add a memory block to cache
* @ingroup cache
* @param An AmmarServer Instance
* @param Dynamic Request to be added
* @retval 1=Success,0=Failure*/
int cache_AddMemoryBlock(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context);

/**
* @brief Create a rule for specific resource so that it will always be served fresh and not cached
* @ingroup cache
* @param An AmmarServer Instance
* @param Resource filename
* @retval 1=Success,0=Failure*/
int cache_AddDoNOTCacheRuleForResource(struct AmmServer_Instance * instance,const char * filename);

/**
* @brief Destroy Cache entry and resource context
* @ingroup cache
* @param An AmmarServer Instance
* @param Resource Context to be removed
* @param Flag controlling whether memory should be freed
* @retval 1=Success,0=Failure*/
int cache_RemoveContextAndResource(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context,unsigned char free_mem);

/**
* @brief Get Hash Value of resource ( to be used for E-Tags http://en.wikipedia.org/wiki/HTTP_ETag or whatever other reason )
* @ingroup cache
* @param An AmmarServer Instance
* @param Index to the cache item requested
* @retval Hash Value for Index specified , 0 = failure */
unsigned long cache_GetHashOfResource(struct AmmServer_Instance * instance,unsigned int index);

/**
* @brief Query for a resource , and return its index
* @ingroup cache
* @param An AmmarServer Instance
* @param Resource we are searching for
* @param Output Index number
* @retval 1=Success,0=Failure*/
unsigned int cache_FindResource(struct AmmServer_Instance * instance,const char * resource,unsigned int * index);

/**
* @brief Query for a resource , and return its index
* @ingroup cache
* @param An AmmarServer Instance
* @param Resource we are searching for
* @param Output Index number
* @retval 1=Success,0=Failure*/
int cache_ResourceExists(struct AmmServer_Instance * instance,char * verified_filename,unsigned int * index);


/**
* @brief Randomize Cache-Etag prefix , this causes all subsequent hits to the cache to have a different E-Tag Prefix
* @ingroup cache
* @param An AmmarServer Instance
* @retval 1=Ok,0=Failed*/
int cache_RandomizeETAG(struct AmmServer_Instance * instance);

/**
* @brief Allocate and create a new empty cache
* @ingroup cache
* @param An AmmarServer Instance
* @param Maximum Number of separate items
* @param Maximum memory usage ( Megabytes ) for this entire cache
* @param Maximum memory usage ( Megabytes ) for a specific entry of the cache
* @retval 1=Ok,0=Failed*/
int cache_Initialize(struct AmmServer_Instance * instance,unsigned int max_seperate_items , unsigned int max_total_allocation_MB , unsigned int max_allocation_per_entry_MB);


/**
* @brief Query to remove a resource using its index
* @ingroup cache
* @param An AmmarServer Instance
* @param Index number of resource to be removed
* @retval 1=Success,0=Failure*/
int cache_RemoveResource(struct AmmServer_Instance * instance,unsigned int index);


/**
* @brief Deallocate and destroy the cache of an AmmarServer instance
* @ingroup cache
* @param An AmmarServer Instance
* @retval 1=Ok,0=Failed*/
int cache_Destroy(struct AmmServer_Instance * instance);

/**
* @brief Get a resource to be served to a client . This call will try to find if it is already used , if it exists on disk , if it is a dynamic request etc , and return the specified buffer
* @ingroup cache
* @param An AmmarServer Instance
* @param HTTPHeader of request we are trying to service with the resource
* @param cacheID for resource
* @param Filename of the resource , this should be verified so that it doesn't access the whole filesystem but only subdirectories of the root public_html dir , and we consider this safe
* @param Output Index number of cache item we requested
* @param Output FileSize of cache item we requested
* @param Output last modification time of cache item we requested
* @param Output flag about whether the buffer returned is compressed or not
* @param Output flag about whether it is safe to automatically free the resource after using it , or there is an automatic handling of memory for the specific item
* @bug    This function should check filesizes/dates and refresh memory snapshots
          If verified_filename , is not really verified (i.e. outside of the public_html root directory , this function could pose a security problem , since it will just blindly open and serve the filename given to it)
* @retval 1=Ok,0=Failed*/
char * cache_GetResource(
                          struct AmmServer_Instance * instance,
                          struct HTTPHeader * request,
                          unsigned int resourceCacheID,
                          char * verified_filename,
                          unsigned int * index,
                          unsigned long *filesize,
                          struct stat * last_modification,
                          unsigned char * compressionSupported,
                          unsigned char * freeContentAfterUsingIt,
                          unsigned char * serveAsRegularFile
                        );


#endif // FILE_CACHING_H_INCLUDED
