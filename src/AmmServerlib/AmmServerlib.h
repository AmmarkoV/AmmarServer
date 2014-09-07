/** @file AmmServerlib.h
* @brief The Main Header for AmmarServer
*
* Any application that may want to interface with AmmarServer will probably want to link to libAmmarServer.a
* and include this header. It provides the entry point for setting up a web share and access to
* sub-modules on runtime.
*
* @author Ammar Qammaz (AmmarkoV)
* @bug AmmarServer is not properly pentested yet
*/

#ifndef AMMSERVERLIB_H_INCLUDED
#define AMMSERVERLIB_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

/**
* @brief An enumerator that lists the types of requests , per HTTP spec , see http://www.w3.org/Protocols/rfc2616/rfc2616-sec9.html
*        Of course not all of them are supported/used internally but they are listed in the same order to maintain spec compatibility
* @bug   A potential bug might arise if the specs of the header file are changed and someone is linking with an older version libAmmServer.a thats why this value exists
*/
#define AMMAR_SERVER_HTTP_HEADER_SPEC 129



/**
* @brief An enumerator that lists the types of requests , per HTTP spec , see http://www.w3.org/Protocols/rfc2616/rfc2616-sec9.html
*        Of course not all of them are supported/used internally but they are listed in the same order to maintain spec compatibility
*/
enum TypesOfRequests
{
    NONE=0,
    HEAD,
    //Asks for the response identical to the one that would correspond to a GET request, but without the response body. This is useful for retrieving meta-information written in response headers, without having to transport the entire content.
    GET,
    //Requests a representation of the specified resource. Requests using GET should only retrieve data and should have no other effect. (This is also true of some other HTTP methods.)[1] The W3C has published guidance principles on this distinction, saying, "Web application design should be informed by the above principles, but also by the relevant limitations."[11] See safe methods below.
    POST,
    //Submits data to be processed (e.g., from an HTML form) to the identified resource. The data is included in the body of the request. This may result in the creation of a new resource or the updates of existing resources or both.
    PUT,
    //Uploads a representation of the specified resource.
    DELETE,
    //Deletes the specified resource.
    TRACE,
    //Echoes back the received request, so that a client can see what (if any) changes or additions have been made by intermediate servers.
    OPTIONS,
    //Returns the HTTP methods that the server supports for specified URL. This can be used to check the functionality of a web server by requesting '*' instead of a specific resource.
    CONNECT,
    //Converts the request connection to a transparent TCP/IP tunnel, usually to facilitate SSL-encrypted communication (HTTPS) through an unencrypted HTTP proxy.[12]
    PATCH,
    //Is used to apply partial modifications to a resource.[13]
    BAD
};


#define MAX_IP_STRING_SIZE 32
#define MAX_QUERY 2048
#define MAX_RESOURCE 2048
#define MAX_FILE_PATH 1024

/** @brief Size for popen replies */
#define POPEN_BUFFER_SIZE 256

#define MAX_INSTANCE_NAME_STRING 128




/**
* @brief Each HTTP Request has a header , this is the internal structure that carries the information about the header of an HTTP request parsed and ready for easy
*        for consumption by the various consumers of HTTP requests
*/
struct HTTPHeader
{
   char * headerRAW;
   unsigned int headerRAWSize;

   int  requestType; //See enum TypesOfRequests
   char resource[MAX_RESOURCE+1];
   char verified_local_resource[MAX_FILE_PATH+1];
   char GETquery[MAX_QUERY+1];

   char * POSTrequest;
   unsigned long POSTrequestSize;

   unsigned char authorized;
   unsigned char keepalive;
   unsigned char supports_compression;

   //RANGE DATA
   unsigned long range_start;
   unsigned long range_end;


   unsigned long ContentLength; //<- for POST requests

   //The next strings point directly on the header to keep memory usage on a minimum
   //and performance on the maximum :P
   char * cookie; //<-   *THIS POINTS SOMEWHERE INSIDE headerRAW , or is 0 *
   unsigned int cookieLength;

   char * host; //<-     *THIS POINTS SOMEWHERE INSIDE headerRAW , or is 0 *
   unsigned int hostLength;

   char * referer; //<-  *THIS POINTS SOMEWHERE INSIDE headerRAW , or is 0 *
   unsigned int refererLength;

   char * eTag; //<-    *THIS POINTS SOMEWHERE INSIDE headerRAW , or is 0 *
   unsigned int eTagLength;

   char * userAgent; //<-    *THIS POINTS SOMEWHERE INSIDE headerRAW , or is 0 *
   unsigned int userAgentLength;

   char * contentType; //<-    *THIS POINTS SOMEWHERE INSIDE headerRAW , or is 0 *
   unsigned int contentTypeLength;

   char * contentDisposition;  //<-    *THIS POINTS SOMEWHERE INSIDE headerRAW , or is 0 *
   unsigned int contentDispositionLength;

   char * boundary;  //<-    *THIS POINTS SOMEWHERE INSIDE headerRAW , or is 0 *
   unsigned int boundaryLength;

};


/**
* @brief Each Dynamic Resource Handler can have multiple profiles for optimizing performance/memory usage etc.
*        For now there are 2 profiles/scenarios. The first one is where there is a global state that all clients should share
*        The second one is where there is a different page for each client , which is more memory intensive since there are separate buffers etc for each request.
*/
enum RHScenarios
{
   SAME_PAGE_FOR_ALL_CLIENTS = 0 ,
   DIFFERENT_PAGE_FOR_EACH_CLIENT
};

/**
* @brief We can override/intercept connections before the very fundamental HTTP stage using a request override context and AmmServer_AddRequestHandler
*        This is the structure that holds the information and what to be called back to populate the response
*/
struct AmmServer_RequestOverride_Context
{
   char requestHeader[64]; //Initial request ( GET , HEAD , CONNECT )
   struct HTTPHeader * request;
   void * request_override_callback;
};

/**
* @brief When a call to a function that is a dynamic request is done this is the structure that holds the information
*/
struct AmmServer_DynamicRequest
{
   unsigned int headerResponse;

   char * content;
   unsigned long contentSize;
   unsigned long MAXcontentSize;

   char * compressedContent;
   unsigned long compressedContentSize;
   unsigned long MAXcompressedContentSize;

   char * GET_request;
   unsigned int GET_request_length;

   char * POST_request;
   unsigned int POST_request_length;


   unsigned int clientID;
};

/**
* @brief We can override resources to respond with our own C function code , to do so a AmmServer_DynamicRequest must be populated using a AmmServer_AddResourceHandler
*/
struct AmmServer_RH_Context
{
   unsigned int RH_Scenario;

   unsigned int last_callback;
   unsigned int callback_every_x_msec;
   char callback_cooldown;

   void * dynamicRequestCallbackFunction;

   char web_root_path[MAX_FILE_PATH];
   char resource_name[MAX_RESOURCE];

   struct AmmServer_DynamicRequest requestContext;
};

/**
* @brief Each Instance of AmmarServer has some basic settings , which are stored in AmmServer_Instance_Settings
*/
struct AmmServer_Instance_Settings
{
    //Configuration of instance..
    //------------------------------------------------
    //A ) Password protection..
    int PASSWORD_PROTECTION;
    char * USERNAME;
    char * PASSWORD;
    char * BASE64PASSWORD;
    //------------------------------------------------

    int BINDING_PORT;
};

/** @brief This holds all the information about an Ammar Server Instance , sockets , thread pools , cache , memory , settings etc , this is the central structure for holding context */
struct AmmServer_Instance
{
    char instanceName[MAX_INSTANCE_NAME_STRING];
    struct AmmServer_Instance_Settings settings;

    unsigned int prespawn_turn_to_serve;
    unsigned int prespawn_jobs_started;
    unsigned int prespawn_jobs_finished;
    int files_open;

    //Server state
    int serversock;
    int server_running;
    int pause_server;
    int stop_server;

    //Cache Items..
    unsigned int cacheVersionETag; //This should be set to a number ( typically 0 ) and gets prepended to the e-tag , in order to make clients refresh on different launches of AmmarServer
    unsigned long loaded_cache_items_Kbytes;
    unsigned int loaded_cache_items;
    void * cache; /*Actually struct cache_item * but declared as a void pointer here */
    void * cacheHashMap;

    void * clientList;
    //Thread holders..
    unsigned int CLIENT_THREADS_STARTED;
    unsigned int CLIENT_THREADS_STOPPED;

    pthread_t server_thread_id;
    pthread_t * threads_pool;
    //pthread_attr_t attr;

    void * prespawned_pool; //Actually struct PreSpawnedThread * but declared as a void pointer here

    struct AmmServer_RequestOverride_Context * clientRequestHandlerOverrideContext;

    char webserver_root[MAX_FILE_PATH];
    char templates_root[MAX_FILE_PATH];
};

/** @brief Structure to keep data for an HTTP Transaction */
struct HTTPTransaction
{
  struct AmmServer_Instance * instance;

  struct HTTPHeader incomingHeader;

  //struct HTTPOutHeader outgoingHeader;
  char * outgoingBody;
  unsigned int outgoingBodySize;

  unsigned int resourceCacheID;

  int clientSock;
  unsigned int clientListID;
  unsigned int threadID;
  int prespawnedThreadFlag;
};

/** @brief Enumerator for calls AmmServer_GetInfo */
enum AmmServInfos
{
    AMMINF_ACTIVE_CLIENTS=0,
    AMMINF_ACTIVE_THREADS
};

/** @brief Enumerator for calls AmmServer_GetIntSettingValue and AmmServer_SetIntSettingValue */
enum AmmServSettings
{
    AMMSET_PASSWORD_PROTECTION=0,
    AMMSET_RANDOMIZE_ETAG_BEGINNING,
    AMMSET_TEST
};

/** @brief Enumerator for calls AmmServer_GetStrSettingValue and AmmServer_SetStrSettingValue */
enum AmmServStrSettings
{
    AMMSET_USERNAME_STR=0,
    AMMSET_PASSWORD_STR,
    AMMSET_TESTSTR
};

/** @brief Returns a string with the version of AmmarServer , in case it returns NULL it means that we are linked to AmmarServerNULL which means a fake binary */
char * AmmServer_Version();

/**
* @brief Internal Check to compare against changes of the header files
* @ingroup tools
* @param Header ( should be AMMAR_SERVER_HTTP_HEADER_SPEC )
* @retval 1=Success,0=Failure
*/
int AmmServer_CheckIfHeaderBinaryAreTheSame(int headerSpec);

/**
* @brief Writes the C string pointed by format to stderr , as a warning ( Yellow ) and logs it to the appropriate log
         If format includes format specifiers (subsequences beginning with %), the additional arguments following format are formatted and inserted in the resulting
         string replacing their respective specifiers.
* @ingroup tools
* @param format , see printf ( http://www.cplusplus.com/reference/cstdio/printf/ )
* @param Arbitrary number of other parameters that where defined in format
*/
void AmmServer_Warning( const char *format , ... );

/**
* @brief Writes the C string pointed by format to stderr , as an error ( Red ) and logs it to the appropriate log
         If format includes format specifiers (subsequences beginning with %), the additional arguments following format are formatted and inserted in the resulting
         string replacing their respective specifiers.
* @ingroup tools
* @param format , see printf ( http://www.cplusplus.com/reference/cstdio/printf/ )
* @param Arbitrary number of other parameters that where defined in format
*/
void AmmServer_Error( const char *format , ... );

/**
* @brief Writes the C string pointed by format to stderr , as a success ( Green ) and logs it to the appropriate log
         If format includes format specifiers (subsequences beginning with %), the additional arguments following format are formatted and inserted in the resulting
         string replacing their respective specifiers.
* @ingroup tools
* @param format , see printf ( http://www.cplusplus.com/reference/cstdio/printf/ )
* @param Arbitrary number of other parameters that where defined in format
*/
void AmmServer_Success( const char *format , ... );

/**
* @brief Start a Web Server , allocate memory , bind ports and return its instance..
* @ingroup core
* @param String containing the name of this Server
* @param String containing the IP to be binded ( 0.0.0.0 , for all interfaces )
* @param Port to use , ports under 1000 require superuser privileges
* @param String with the filename of a configuration file
* @param String with the root public_html directory , all directories that are childs of this dir could be visible
* @param String with the root directory for templates ( custom 404 pages etc )
* @retval An Ammar Server instance or 0=Failure
*/
struct AmmServer_Instance * AmmServer_Start(const char * name ,const char * ip,unsigned int port,const char * conf_file,const char * web_root_path,const char * templates_root_path);

/**
* @brief Start a Web Server , allocate memory , bind ports and return its instance , also process arguments ( argc and argv from int main(int argc, char *argv[]) ) ..
* @ingroup core
* @param String containing the name of this Server
* @param argc , number of arguments
* @param argv , array of strings
* @param String containing the IP to be binded ( 0.0.0.0 , for all interfaces )
* @param Port to use , ports under 1000 require superuser privileges
* @param String with the filename of a configuration file
* @param String with the root public_html directory , all directories that are childs of this dir could be visible
* @param String with the root directory for templates ( custom 404 pages etc )
* @retval An Ammar Server instance or 0=Failure
*/
struct AmmServer_Instance * AmmServer_StartWithArgs(const char * name , int argc, char ** argv , const char * ip,unsigned int port,const char * conf_file,const char * web_root_path,const char * templates_root_path);

/**
* @brief Stop a Web Server , deallocate memory , free ports and free the server instance..
* @ingroup core
* @param An AmmarServer Instance
* @retval 1=Success,0=Failure
*/
int AmmServer_Stop(struct AmmServer_Instance * instance);

/**
* @brief Query if an instance of AmmarServer is initialized and running
* @ingroup core
* @param An AmmarServer Instance
* @retval 1=Running,0=Stopped
*/
int AmmServer_Running(struct AmmServer_Instance * instance);

/**
* @brief Add a request handler to handle requests , before they get processed internally
*        Calling this will bind a C function that will be called and produce output when someone asks for any resource using the specified method
         TODO : Improve this documenatation
* @ingroup core
* @param An AmmarServer Instance
* @param A AmmServer_RequestOverride_Context to be populated
* @param Request Type
* @param Pointer to function callback
* @retval 1=Success,0=Fail
*/
int AmmServer_AddRequestHandler(struct AmmServer_Instance * instance,struct AmmServer_RequestOverride_Context * RequestOverrideContext,const char * request_type,void * callback);

/**
* @brief Add a request handler to handle dynamic requests , the core mechanic of AmmarServer
*        Calling this will bind a C function that will be called and produce output when someone asks for a resource
         TODO : Improve this documenatation
* @ingroup core
* @param An AmmarServer Instance
* @param An AmmServer_RH_Context to be populated
* @param Name of resource that should get dynamic responses ( i.e. "index.html" )
* @param Root Path for the specific resource
* @param Memory chunk to allocate for responses , ( this is the max response size )
* @param Minimum time between two calls of the function ( 0 = no minimum time)
* @param Function to be called and provides output when someone asks for resource
* @param Scenario/Profile of this resource ( see RHScenarios )
* @retval 1=Success,0=Fail
*/
int AmmServer_AddResourceHandler
     ( struct AmmServer_Instance * instance,
       struct AmmServer_RH_Context * context,
       const char * resource_name ,
       const char * web_root,
       unsigned int allocate_mem_bytes,
       unsigned int callback_every_x_msec,
       void * callback,
       unsigned int scenario
    );




/**
* @brief Remove a request handler that hanles dynamic requests
* @ingroup core
* @param An AmmarServer Instance
* @param An AmmServer_RH_Context to be freed
* @param Switch to control freeing memory or not for this context ( typically should be set to 1 except one knows what he is trying to do )
* @retval 1=Success,0=Failure
*/
int AmmServer_RemoveResourceHandler(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context,unsigned char free_mem);

/**
* @brief Get an Integer out of the state of an instance , of course one can dive into the instance structure but this is a much more clean way to do this
* @ingroup core
* @param An AmmarServer Instance
* @param An ID about which info we want , see (  AmmServInfos )
* @retval Value of the integer we asked about
*/
int AmmServer_GetInfo(struct AmmServer_Instance * instance,unsigned int info_type);

/**
* @brief Get an Integer out of the state of an instance , of course one can dive into the instance structure but this is a much more clean way to do this
* @ingroup core
* @param An AmmarServer Instance
* @param An ID about which integer info we want , see (  AmmServSettings )
* @retval Value of the integer we asked about
*/
int AmmServer_GetIntSettingValue(struct AmmServer_Instance * instance,unsigned int set_type);

/**
* @brief Set an Integer inside the state of an instance , of course one can dive into the instance structure but this is a much more clean way to do this
* @ingroup core
* @param An AmmarServer Instance
* @param An ID about which integer info we want , see (  AmmServSettings )
* @param New value to set
* @retval Value of the integer we asked about
*/
int AmmServer_SetIntSettingValue(struct AmmServer_Instance * instance,unsigned int set_type,int set_value);



/**
* @brief Get a String out of the state of an instance , of course one can dive into the instance structure but this is a much more clean way to do this
* @ingroup core
* @param An AmmarServer Instance
* @param An ID about which string info we want , see (  AmmServStrSettings )
* @retval Value of the string we asked about
*/
char * AmmServer_GetStrSettingValue(struct AmmServer_Instance * instance,unsigned int set_type);

/**
* @brief Set an string inside the state of an instance , of course one can dive into the instance structure but this is a much more clean way to do this
* @ingroup core
* @param An AmmarServer Instance
* @param An ID about which integer info we want , see (  AmmServStrSettings )
* @param New string value to set
* @retval 1=Success,0=Failure */
int AmmServer_SetStrSettingValue(struct AmmServer_Instance * instance,unsigned int set_type,const char * set_value);

/**
* @brief Get a POST argument
* @ingroup core
* @param Instance of an AmmarServer
* @param Request that contains the POST argument ( see AmmServer_DynamicRequest )
* @param Input Name of argument we are looking for
* @param Output Pointer that will be copied with the value we were looking for
* @param Maximum Size for output Value
* @retval 1=Success,0=Failure */
int AmmServer_POSTArg (struct AmmServer_Instance * instance,struct AmmServer_DynamicRequest * rqst,const char * var_id_IN,char * var_value_OUT,unsigned int max_var_value_OUT);


/**
* @brief Get a GET argument
* @ingroup core
* @param Instance of an AmmarServer
* @param Request that contains the POST argument ( see AmmServer_DynamicRequest )
* @param Input Name of argument we are looking for
* @param Output Pointer that will be copied with the value we were looking for
* @param Maximum Size for output Value
* @retval 1=Success,0=Failure */
int AmmServer_GETArg  (struct AmmServer_Instance * instance,struct AmmServer_DynamicRequest * rqst,const char * var_id_IN,char * var_value_OUT,unsigned int max_var_value_OUT);


/**
* @brief Access a FILE submitted by a dynamic requested
* @ingroup core
* @param Instance of an AmmarServer
* @param Request that contains the POST argument ( see AmmServer_DynamicRequest )
* @param Input Name of argument we are looking for
* @param Output Pointer that will be copied with the value we were looking for
* @param Maximum Size for output Value
* @retval 1=Success,0=Failure */
int AmmServer_FILES   (struct AmmServer_Instance * instance,struct AmmServer_DynamicRequest * rqst,const char * var_id_IN,char * var_value_OUT,unsigned int max_var_value_OUT);

/**
* @brief Shorthand/Shortcut for AmmServer_POSTArg()
* @ingroup shortcut */
int _POST (struct AmmServer_Instance * instance,struct AmmServer_DynamicRequest * rqst,const char * var_id_IN,char * var_value_OUT,unsigned int max_var_value_OUT);

/**
* @brief Shorthand/Shortcut for AmmServer_GETArg()
* @ingroup shortcut */
int _GET  (struct AmmServer_Instance * instance,struct AmmServer_DynamicRequest * rqst,const char * var_id_IN,char * var_value_OUT,unsigned int max_var_value_OUT);

/**
* @brief Shorthand/Shortcut for AmmServer_FILES()
* @ingroup shortcut */
int _FILES(struct AmmServer_Instance * instance,struct AmmServer_DynamicRequest * rqst,const char * var_id_IN,char * var_value_OUT,unsigned int max_var_value_OUT);

/**
* @brief Staged way to easily handle bad clients etc from the clients , currently a stub..!
* @ingroup shortcut
* @bug Client behaviours etc are not implemented yet */
int AmmServer_SignalCountAsBadClientBehaviour(struct AmmServer_Instance * instance,struct AmmServer_DynamicRequest * rqst);

/**
* @brief Save Dynamic Request to file
* @ingroup tools
* @param Filename to save the dynamic request
* @param Instance of an AmmarServer
* @param Request that we want to save to a file ( see AmmServer_DynamicRequest )
* @retval 1=Success,0=Failure */
int AmmServer_SaveDynamicRequest(const char* filename , struct AmmServer_Instance * instance  , struct AmmServer_DynamicRequest * rqst);



/**
* @brief Set resource handler to no-cache mode , this means whoever asks for it will never get a cached response
* @ingroup tools
* @param Instance of an AmmarServer
* @param Resource context that should always be served fresh ( AmmServer_RH_Context )
* @retval 1=Success,0=Failure */
int AmmServer_DoNOTCacheResourceHandler(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context);


/**
* @brief Set resource to no-cache mode , this means whoever asks for it will never get a cached response
* @ingroup tools
* @param Instance of an AmmarServer
* @param Resource name that we want to always serve fresh
* @retval 1=Success,0=Failure */
int AmmServer_DoNOTCacheResource(struct AmmServer_Instance * instance,const char * resource_name);


/**
* @brief Planned functionality for a default http administrator panel per server per instance , currently not implemented correctly
* @ingroup core
* @param IP to bind the interface at
* @param Port to use
* @retval Value of the integer we asked about
*/
struct AmmServer_Instance *  AmmServer_StartAdminInstance(const char * ip,unsigned int port);

/**
* @brief Perform a sanity check on the instance of AmmarServer , this is mostly a dev debug tool and an entry point for code inside AmmServerlib
* @ingroup tools
* @param Ammar Server Instance
* @retval 1=Ok,0=Failed
* @bug Maybe remove AmmServer_SelfCheck
*/
int AmmServer_SelfCheck(struct AmmServer_Instance * instance);



/**
* @brief Execute a command and copy its output line to the provided buffer
* @ingroup tools
* @param Command to execute
* @param Allocated memory to store the result
* @param Size of Allocated memory
* @param Number of line we want to get back
* @retval 1=Ok,0=Failed
* @bug Executing commands can be dangerous , always check and sanitize input before executing , Also be sure about the max size of output so that you don't lose a part of it , also make something like escapeshellcmd
*/
int AmmServer_ExecuteCommandLineNum(const char *  command , char * what2GetBack , unsigned int what2GetBackMaxSize,unsigned int lineNumber);

/**
* @brief Execute a command and copy its output to the provided buffer
* @ingroup tools
* @param Command to execute
* @param Allocated memory to store the result
* @param Size of Allocated memory
* @retval 1=Ok,0=Failed
* @bug Executing commands can be dangerous , always check and sanitize input before executing , Also be sure about the max size of output so that you don't lose a part of it , also make something like escapeshellcmd
*/
int AmmServer_ExecuteCommandLine(const char *  command , char * what2GetBack , unsigned int what2GetBackMaxSize);



/**
* @brief Hot-Replace a character inside a memory block , typically used to replace characters like '+' with ' '
* @ingroup tools
* @param Pointer to memory that contains the null terminated string
* @param Character to be replaced
* @param What to replace the character with
*/
void AmmServer_ReplaceCharInString(char * input , char findChar , char replaceWith);

/**
* @brief Hot-Replace a variable inside a memory block , typically used to replace placeholders inside text files , like $$$$$$$$NAME$$$$$$$$  , the value should be smaller or equal to the var beeing replaced
* @ingroup tools
* @param Pointer to memory that contains the document
* @param Size of document
* @param Variable to be replaced
* @param What to replace it with
* @retval 1=Ok,0=Failed
* @bug Value should not be bigger than variable otherwise things won't fit in the same memory block
*/
int AmmServer_ReplaceVarInMemoryFile(char * page,unsigned int pageLength,const char * var,const char * value);


/**
* @brief Read a file and store it to a freshly allocated memory block
* @ingroup tools
* @param Input Filename
* @param Output Maximum Size
* @retval Pointer to the new memory or 0=Failed
*/
char * AmmServer_ReadFileToMemory(const char * filename,unsigned int *length );

/**
* @brief Dump a memory block to a file
* @ingroup tools
* @param Output Filename
* @param Input Pointer to memory
* @param Size of memory block
* @retval 1=Ok,0=Failed
*/
int AmmServer_WriteFileFromMemory(const char * filename,char * memory , unsigned int memoryLength);

/**
* @brief Register a function to call a function that gracefully terminates a client when a SIGKILL or the time to stop the server comes
* @ingroup tools
* @param Pointer to function
* @retval 1=Exists,0=Does not Exist
*/
int AmmServer_RegisterTerminationSignal(void * callback);



/**
* @brief Check if directory Exists
* @ingroup tools
* @param Path to directory
* @retval 1=Exists,0=Does not Exist
*/
int AmmServer_DirectoryExists(const char * filename);


/**
* @brief Check if file Exists
* @ingroup tools
* @param Path to file
* @retval 1=Exists,0=Does not Exist
*/
int AmmServer_FileExists(const char * filename);

/**
* @brief Erase a File
* @ingroup tools
* @param Path to file
* @retval 1=Success,0=Failure
*/
int AmmServer_EraseFile(const char * filename);

/**
* @brief Check if a string has html elements inside it , so if we append it to a web site we won't have html injected
* @ingroup tools
* @param Input String
* @retval 1=Safe,0=Unsafe
*/
unsigned int AmmServer_StringIsHTMLSafe(const char * str);

#ifdef __cplusplus
}
#endif

#endif // AMMSERVERLIB_H_INCLUDED
