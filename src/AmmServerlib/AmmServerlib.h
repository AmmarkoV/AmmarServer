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
#include <libgen.h>

#if USE_OPENSSL
 #include <openssl/ssl.h>
 #include <openssl/err.h>
#endif // USE_OPENSSL

/**
* @brief An ID that is used to distinguish between different versions..
* @bug   A potential bug might arise if the specs of the header file are changed and someone is linking with an older version libAmmServer.a thats why this value exists
*/
#define AMMAR_SERVER_HTTP_HEADER_SPEC 136



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




/**
* @brief An enumerator that lists the types of requests fields availiable for a POST / GET / COOKIE or FILE request
*/
enum TypesOfRequestFields
{
   NAME        = 1 ,
   VALUE           ,
   FILENAME        ,
   TEMPNAME        ,
   TYPE            ,
   SIZE
};




#define MAX_IP_STRING_SIZE 48 // This should be more than INET6_ADDRSTRLEN
#define MAX_QUERY 2048
#define MAX_RESOURCE 2048
#define MAX_FILE_PATH 1024

/** @brief Size for popen replies */
#define POPEN_BUFFER_SIZE 256

#define MAX_INSTANCE_NAME_STRING 128


/** @brief Maximum number of boundaries to be searched for in a GET request */
#define MAX_HTTP_GET_VARIABLE_COUNT 128

/** @brief Maximum number of boundaries to be searched for in a POST request */
#define MAX_HTTP_POST_BOUNDARY_COUNT 32

/**
* @brief Each POST Request has a payload that consists of a series of boundaries that contain data. This structure holds this data as a series of pointers in the initial header buffer
         in order to facilitate easy and fast parsing of the data
*/
struct POSTRequestBoundaryContent
{
   ///very important to add any fields here to the recalculateHeaderFieldsBasedOnANewBaseAddress  code..
   char * pointerStart;
   char * pointerEnd;
   unsigned int contentSize;

   //All the following are 0 if not used or point to a place inside an HTTP Header
   char *       name;
   unsigned int nameSize;

   char *       value;
   unsigned int valueSize;

   char *       filename;
   unsigned int filenameSize;

   char *       contentDisposition;
   unsigned int contentDispositionSize;

   char *       contentType;
   unsigned int contentTypeSize;

   int reallocateOnHeaderRAWResize;
};


/**
* @brief Each GET Request has a payload that consists of a series of boundaries that contain data. This structure holds this data as a series of pointers in the initial header buffer
         in order to facilitate easy and fast parsing of the data
*/
struct GETRequestContent
{
   ///very important to add any fields here to the recalculateHeaderFieldsBasedOnANewBaseAddress  code..
   //All the following are 0 if not used or point to a place inside an HTTP Header
   char *       name;
   unsigned int nameSize;

   char *       value;
   unsigned int valueSize;

   int reallocateOnHeaderRAWResize;
};


/**
* @brief Each HTTP Request has a header , this is the internal structure that carries the information about the header of an HTTP request parsed and ready for easy
*        for consumption by the various consumers of HTTP requests
*/
struct HTTPHeader
{
   unsigned int failed;
   unsigned int dumpedToFile; //This is dummy

   unsigned int parsingStartOffset;
   unsigned int parsingCurrentLine;


   char * headerRAW; //This is the place where everything rests at..!
   unsigned int headerRAWHeadSize;
   unsigned int headerRAWSize;
   unsigned int headerRAWRequestedSize; // The size that the client requests ( we have our own limits and agenda though )
   unsigned int MAXheaderRAWSize;

   int  requestType; //See enum TypesOfRequests
   char resource[MAX_RESOURCE+1];
   char verified_local_resource[MAX_FILE_PATH+1];

   //char GETquery[MAX_QUERY+1];
   unsigned int sizeOfExtraDataThatWillNeedToBeDeallocated;
   char * extraDataThatWillNeedToBeDeallocated;

   char * GETrequest;
   unsigned long GETrequestSize;

   char * POSTrequest;
   unsigned long POSTrequestSize;
   char * POSTrequestBody;
   unsigned long POSTrequestBodySize;


   unsigned char authorized;
   unsigned char keepalive;
   unsigned char supports_compression;


   //RANGE DATA
   unsigned long range_start;
   unsigned long range_end;


   unsigned long ContentLength; //<- for POST requests

   //The next strings point directly on the header to keep memory usage on a minimum
   //and performance on the maximum , they have to be refreshed if memory gets reallocated:P

   ///very important to add any fields here to the recalculateHeaderFieldsBasedOnANewBaseAddress  code..
   unsigned int cookieLength;
   char * cookie; //<-   *THIS POINTS SOMEWHERE INSIDE headerRAW , or is 0 *

   unsigned int hostLength;
   char * host; //<-     *THIS POINTS SOMEWHERE INSIDE headerRAW , or is 0 *

   unsigned int refererLength;
   char * referer; //<-  *THIS POINTS SOMEWHERE INSIDE headerRAW , or is 0 *

   unsigned int eTagLength;
   char * eTag; //<-    *THIS POINTS SOMEWHERE INSIDE headerRAW , or is 0 *

   unsigned int userAgentLength;
   char * userAgent; //<-    *THIS POINTS SOMEWHERE INSIDE headerRAW , or is 0 *

   unsigned int contentTypeLength;
   char * contentType; //<-    *THIS POINTS SOMEWHERE INSIDE headerRAW , or is 0 *

   unsigned int contentDispositionLength;
   char * contentDisposition;  //<-    *THIS POINTS SOMEWHERE INSIDE headerRAW , or is 0 *

   unsigned int boundaryLength;
   char * boundary;  //<-    *THIS POINTS SOMEWHERE INSIDE headerRAW , or is 0 *

   unsigned int GETItemNumber;
   struct GETRequestContent GETItem[MAX_HTTP_GET_VARIABLE_COUNT+1]; //<-    *THIS POINTS SOMEWHERE INSIDE headerRAW , or is 0 *

   unsigned int POSTItemNumber;
   struct POSTRequestBoundaryContent POSTItem[MAX_HTTP_POST_BOUNDARY_COUNT+1]; //<-    *THIS POINTS SOMEWHERE INSIDE headerRAW , or is 0 *

   unsigned int COOKIEItemNumber;
   struct GETRequestContent COOKIEItem[MAX_HTTP_GET_VARIABLE_COUNT+1]; //<-    *THIS POINTS SOMEWHERE INSIDE headerRAW , or is 0 *
};



/**
* @brief Each Dynamic Resource Handler can have multiple profiles for optimizing performance/memory usage etc.
*        For now there are 2 profiles/scenarios. The first one is where there is a global state that all clients should share
*        The second one is where there is a different page for each client , which is more memory intensive since there are separate buffers etc for each request.
*/
enum RHScenarios
{
   SAME_PAGE_FOR_ALL_CLIENTS        = 1 ,
   DIFFERENT_PAGE_FOR_EACH_CLIENT   = 2 ,
   ENABLE_RECEIVING_FILES           = 4
                                  //= 8
                                  //= 16
                                  //= 32
                                  //= 64
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
* @brief A Wrapper around a memory buffer that enables house keeping for reallocations etc
*/
struct AmmServer_MemoryHandler
{
  unsigned int contentSize;
  unsigned int contentCurrentLength;
  unsigned int contentGrowthStep;
  char * content;
};




/**
* @brief When a call to a function that is a dynamic request is done this is the structure that holds the information
*/
struct AmmServer_DynamicRequest
{
   struct AmmServer_Instance * instance;
   unsigned int clientID;
   unsigned int sessionID;
   unsigned int useSessionLifecycle;
   char sessionToken[64]; //<- the real, unguessable session identifier ; sessionID above is legacy/unused - see
                          //   cache/session_list.h. Sized to match SESSION_TOKEN_STRING_SIZE ( server_configuration.h ) ;
                          //   not using that macro directly here to avoid a circular include with server_configuration.h.

   //Set by AmmServer_SetCookie() ( or internally, e.g. when a brand new session cookie needs to be issued ) -
   //raw Set-Cookie header line(s), appended verbatim into the response by SendSuccessCodeHeader() and cleared
   //immediately after. Not a general "add any header" facility ( yet ) - just cookies, since that's the one
   //thing dynamic content has never had a way to influence in the response header.
   char pendingResponseHeaders[512];

   unsigned int headerResponse;

   char * content;
   unsigned long contentSize;
   unsigned long MAXcontentSize;
   unsigned int contentContainsPathToFileToBeStreamed;

   char * compressedContent;
   unsigned long compressedContentSize;
   unsigned long MAXcompressedContentSize;

   unsigned int sizeOfExtraDataThatWillNeedToBeDeallocated;
   char * extraDataThatWillNeedToBeDeallocated;

   unsigned int POSTItemNumber;
   struct POSTRequestBoundaryContent * POSTItem; //<-    *THIS POINTS SOMEWHERE INSIDE THE STACK SO NEVER FREE IT ..!

   unsigned int GETItemNumber;
   struct GETRequestContent * GETItem; //<-    *THIS POINTS SOMEWHERE INSIDE headerRAW , or is 0 *

   unsigned int COOKIEItemNumber;
   struct GETRequestContent * COOKIEItem; //<-    *THIS POINTS SOMEWHERE INSIDE headerRAW , or is 0 *
};



/**
* @brief We can override resources to respond with our own C function code , to do so a AmmServer_DynamicRequest must be populated using a AmmServer_AddResourceHandler
*/
struct AmmServer_RH_Context
{
   unsigned int RH_Scenario;

   unsigned int enablePOST;
   unsigned int allowCrossRequests;
   unsigned int needsDifferentPageForEachClient;
   unsigned int needsSamePageForAllClients;
   unsigned int executedNow;
   unsigned int last_callback;
   unsigned int callback_every_x_msec;
   char callback_cooldown;

   //Serializes callback invocation for SAME_PAGE_FOR_ALL_CLIENTS resources , whose requestContext.content buffer
   //is shared by every concurrent request ; relies on zero-init ( ={0} ) being equivalent to PTHREAD_MUTEX_INITIALIZER
   pthread_mutex_t content_mutex;

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

    unsigned int MAX_POST_TRANSACTION_SIZE;
    int ENABLE_POST;

    //HTTPS / SSL configuration
    int    ENABLE_HTTPS;
    int    HTTPS_PORT;
    char * SSL_CERT_PATH;
    char * SSL_KEY_PATH;
};




struct AmmServer_ResponseCategory_Statistics
{
    unsigned long uploadedKB;
    unsigned long downloadedKB;
    unsigned int  requestNumber;
};


/** @brief This holds all the statistics of an Ammar Server Instance
  *        this is the central structure for holding all of these things to keep the rest of the instance from getting cluttered
  * @bug AmmServer_Instance_Statistics are not yet used , they are a stub..!
*/
struct AmmServer_Instance_Statistics
{
    unsigned long filesCurrentlyOpen;
    unsigned long filesTotalOpen;

    unsigned long clientsServed;

    unsigned long totalUploadKB;
    unsigned long totalDownloadKB;


    unsigned long totalUploadBytes;
    unsigned long totalDownloadBytes;


    unsigned long recvOperationsStarted;
    unsigned long recvOperationsFinished;


    struct AmmServer_ResponseCategory_Statistics notModifiedPages;
    struct AmmServer_ResponseCategory_Statistics notFonudPages;
    struct AmmServer_ResponseCategory_Statistics otherCategoriesHere;

    /*etc*/
};


/** @brief This holds all the information about an Ammar Server Instance , sockets , thread pools , cache , memory , settings etc , this is the central structure for holding context */
struct AmmServer_Instance
{
    char instanceName[MAX_INSTANCE_NAME_STRING];
    struct AmmServer_Instance_Settings settings;

    unsigned int prespawn_turn_to_serve;
    unsigned int prespawn_jobs_started;
    unsigned int prespawn_jobs_finished;

    struct AmmServer_Instance_Statistics statistics;

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
    void * sessionList;
    //Thread holders..
    unsigned int CLIENT_THREADS_STARTED;
    unsigned int CLIENT_THREADS_STOPPED;

    pthread_t server_thread_id;
    pthread_t * threads_pool;
    //pthread_attr_t attr;

    void * prespawned_pool; //Actually struct PreSpawnedThread * but declared as a void pointer here

    struct AmmServer_RequestOverride_Context * clientRequestHandlerOverrideContext;
    struct AmmServer_RH_Context webserverMonitorPage;
    int webserverMonitorEnabled;

    char webserver_root[MAX_FILE_PATH];
    char templates_root[MAX_FILE_PATH];

    //SSL Stuff
    int sslAvailable;
    int sslserversock;  /* listening socket for HTTPS port, -1 if disabled */
    #if USE_OPENSSL
     SSL_CTX *sslctx;
    #endif // USE_OPENSSL

    //Epoll accept layer : newly accepted connections sit here ( costing no thread ) until they have their first
    //byte ready to read , only then do they get handed to a prespawned/fresh worker thread like today
    int accept_epoll_fd;      /* -1 if not running */
    pthread_t accept_epoll_thread_id;

    //API mode ( AmmServer_SetAPIHandler ) : when apiHandler is non-zero the accept epoll layer above keeps
    //every connection for its whole life and answers it from this one callback , instead of handing it to a
    //worker thread and running the file-serving pipeline. 0 = ordinary file/dynamic-content server.
    void * apiHandler;
    unsigned int apiMaxResponseSize;
    //Set by AmmServer_StartAPIOnRings : how many io_uring rings to serve API mode from ( uringAPIServer.h )
    //instead of the epoll layer , when the kernel can. 0 = the epoll layer , 1 = one ring , more than one
    //multiplies the connection ceiling by that many since a ring's file table is a descriptor budget of its
    //own. It is a request , not a promise - if the rings cannot be set up the epoll layer takes the instance
    //as usual.
    unsigned char apiRingCount;
    //Set by AmmServer_StartAPIOnRingsReusingPort : the listening socket gets SO_REUSEPORT before it is bound ,
    //so several processes ( or instances ) can listen on the same ip:port and the kernel spreads new
    //connections over them. 0 = the usual exclusive bind.
    unsigned char reusePort;
    //Set by AmmServer_SetAPIBatchHandler : when non-zero the io_uring transport answers every request that is
    //ready in one pass over its completion queues with a single call to this , instead of one apiHandler call
    //each. Written by the caller's thread and read by the ring thread , so it is only ever touched atomically.
    void * apiBatchHandler;
};

/** @brief The most requests one call of an API batch handler is given ( see AmmServer_SetAPIBatchHandler ) */
#define AMMSERVER_API_MAX_BATCH 32

/** @brief Structure to keep data for an HTTP Transaction */
struct HTTPTransaction
{
  struct AmmServer_Instance * instance;

  struct HTTPHeader incomingHeader;

  //struct HTTPOutHeader outgoingHeader;
  char * outgoingBody;
  unsigned int outgoingBodySize;

  unsigned int resourceCacheID;

  //Raw Set-Cookie header line(s) a dynamic-content callback queued via AmmServer_SetCookie() ( or that
  //getSessionFromHeader() queued to issue a brand new session cookie ) - copied here from the request's
  //AmmServer_DynamicRequest::pendingResponseHeaders by cache_GetResource() right after the callback returns,
  //since the transaction ( not the per-callback rqst, which is freed shortly after ) is what's still alive when
  //SendSuccessCodeHeader() actually builds and sends the response header. Emitted verbatim, then cleared.
  char pendingResponseHeaders[512];

  int clientSock;
  int clientDisconnected;
  unsigned int clientListID;
  unsigned int threadID;
  int prespawnedThreadFlag;

  char ipStr[MAX_IP_STRING_SIZE];
  unsigned int port;

  #if USE_OPENSSL
   SSL * ssl;  /* non-NULL when this connection is TLS */
  #endif // USE_OPENSSL
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
    AMMSET_MAX_POST_TRANSACTION_SIZE,
    AMMSET_TIMEOUT_SEC,
    AMMSET_TEST
};

/** @brief Enumerator for calls AmmServer_GetStrSettingValue and AmmServer_SetStrSettingValue */
enum AmmServStrSettings
{
    AMMSET_USERNAME_STR=0,
    AMMSET_PASSWORD_STR,
    AMMSET_SSL_CERT_STR,
    AMMSET_SSL_KEY_STR,
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




int AmmServer_GetDateString(char * output , unsigned int maxOutput);


int AmmServer_AppendToFile(const char * filename,const char * msg);


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
* @brief Writes the C string pointed by format to stderr , as an info ( White ) and does not log it
         If format includes format specifiers (subsequences beginning with %), the additional arguments following format are formatted and inserted in the resulting
         string replacing their respective specifiers.
* @ingroup tools
* @param format , see printf ( http://www.cplusplus.com/reference/cstdio/printf/ )
* @param Arbitrary number of other parameters that where defined in format
*/
void AmmServer_Info( const char *format , ... );



/**
* @brief Writes the C string pointed by format to stderr , as an info ( White ) and does not log it
         If format includes format specifiers (subsequences beginning with %), the additional arguments following format are formatted and inserted in the resulting
         string replacing their respective specifiers.
* @ingroup tools
* @param format , see printf ( http://www.cplusplus.com/reference/cstdio/printf/ )
* @param Arbitrary number of other parameters that where defined in format
*/
void AmmServer_Stub( const char *format , ... );



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
struct AmmServer_Instance * AmmServer_StartSSL(const char * name ,const char * ip,unsigned int port,const char * conf_file,const char * web_root_path,const char * templates_root_path,const char * ssl_cert,const char * ssl_key);

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
* @brief Return a file instead of a Dynamic Request
* @ingroup core
* @param An AmmarServer Request
* @param File to serve
* @retval 1=Running,0=Stopped
*/
int AmmServer_DynamicRequestReturnFile(struct AmmServer_DynamicRequest  * rqst,const char * filename);



void* AmmServer_DynamicRequestReturnMemoryHandler(struct AmmServer_DynamicRequest  * rqst,struct AmmServer_MemoryHandler * content);


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
* @brief One HTTP request handed to an API handler ( see AmmServer_SetAPIHandler ). Every pointer in here points
*        inside the event loop's own receive buffer and is only valid until the callback returns.
*/
struct AmmServer_APIRequest
{
   int method;                   /**< @brief One of enum TypesOfRequests ( GET , HEAD , POST , ... ) */
   const char * path;            /**< @brief Request target with any query string stripped , NUL terminated , percent escapes left as-is */
   unsigned int pathLength;
   const char * query;           /**< @brief Everything after the first '?' , NUL terminated , or 0 if the request had none */
   unsigned int queryLength;
   const char * headers;         /**< @brief The raw header block after the request line , NUL terminated ( use AmmServer_APIRequestHeader to read one ) */
   unsigned int headersLength;
   const char * body;            /**< @brief Request body , NOT NUL terminated , or 0 if the request had none */
   unsigned int bodyLength;
};

/**
* @brief The response an API handler fills in. body points at a buffer the server owns ( maxBodyLength bytes ,
*        fixed at AmmServer_SetAPIHandler time ) ; write into it and set bodyLength.
*/
struct AmmServer_APIResponse
{
   unsigned int statusCode;      /**< @brief 200 , 201 , 400 , 404 , 503 , ... - emitted with its proper HTTP reason phrase */
   const char * contentType;     /**< @brief Value for the Content-Type header , e.g. "application/json" ; 0 means text/plain */
   const char * extraHeaders;    /**< @brief More header lines , each ending in \r\n , e.g. "Content-Encoding: gzip\r\n" ; 0 for none. Sent as given , and the whole header block has to fit in API_RESPONSE_HEADER_RESERVE ( 256 bytes ) or the request is answered 500 */
   char * body;
   unsigned int bodyLength;
   unsigned int maxBodyLength;
   unsigned char overflowed;     /**< @brief Set , and stays set , when a AmmServer_APIResponseWrite* call ran out of body buffer */
};

/**
* @brief Put this instance in API mode and bind the C function that answers every request.
*
*        API mode replaces the thread-per-connection pipeline with one non-blocking epoll event loop that owns
*        every connection for its entire life : an idle keep-alive connection costs an fd and a small struct
*        instead of a whole worker thread , so a single process can hold tens of thousands of them. File
*        serving , the file cache , dynamic resource handlers , the client list and access logging are all
*        bypassed - the callback is the entire server. This is the mode to use for a JSON/REST API , where
*        every response carries its own status code and content type ; use AmmServer_AddResourceHandler
*        instead if you want dynamic pages mixed in with served files.
*
*        The callback runs on the event loop thread , so it must not block : one slow callback delays every
*        other connection. It is also only ever called from that one thread , so it needs no locking of its
*        own state.
*
* @ingroup core
* @param An AmmarServer Instance , before AmmServer_Start has been called on it
* @param Pointer to a void callback(struct AmmServer_APIRequest *,struct AmmServer_APIResponse *)
* @param Size in bytes of the response body buffer handed to the callback ( 0 for a sane default )
* @retval 1=Success,0=Fail
*/
int AmmServer_SetAPIHandler(struct AmmServer_Instance * instance,void * callback,unsigned int maxResponseSize);

/**
* @brief Answer requests in batches on the io_uring transport : every request that is complete after one pass
*        over the rings' completion queues ( up to AMMSERVER_API_MAX_BATCH of them ) is handed to one call of
*        callback(requests,responses,count) , each requests[i] answered by filling responses[i] exactly as
*        a single handler would. Nothing waits for a batch to fill : at low load a batch is one request , and
*        it grows by itself with whatever arrived while the previous one was being answered.
*
*        It is for a handler whose cost is mostly a round trip it can share , such as a database it can send
*        every query of the batch to before reading any answer back ( libpq's pipeline mode ). It blocks the
*        ring thread for the whole batch , exactly as a single handler blocks it for one request.
*
*        The single handler given at start stays in use for the epoll transport , and for any request
*        answered before this is set , so the two have to give the same answers.
*
* @ingroup core
* @param A running API mode instance
* @param Pointer to a void callback(struct AmmServer_APIRequest * requests,struct AmmServer_APIResponse * responses,unsigned int count) ,
*        or 0 to go back to one call per request
* @retval 1=Success,0=Fail
*/
int AmmServer_SetAPIBatchHandler(struct AmmServer_Instance * instance,void * batchCallback);

/**
* @brief Start an instance that is in API mode from the outset : the same as AmmServer_Start followed by
*        AmmServer_SetAPIHandler , except that the handler is in place before the server thread exists , so
*        there is no window in which an early connection is dispatched into the file-serving pipeline. This
*        is the call to use for an API ; AmmServer_SetAPIHandler is there for switching an already running
*        instance over.
*        No files are served in API mode , so there is no web root or template root to pass.
* @ingroup core
* @param Name of this instance ( used for its log filenames )
* @param String with the binding IP , e.g. "0.0.0.0"
* @param Port to bind , ports under 1000 need privileges or CAP_NET_BIND_SERVICE
* @param Configuration file to read , or 0 for the usual automatic lookup
* @param Pointer to a void callback(struct AmmServer_APIRequest *,struct AmmServer_APIResponse *)
* @param Size in bytes of the response body buffer handed to the callback ( 0 for a sane default )
* @retval A running instance , or 0 on failure
*/
struct AmmServer_Instance * AmmServer_StartAPI(const char * name ,const char * ip,unsigned int port,const char * conf_file,void * callback,unsigned int maxResponseSize);

/**
* @brief AmmServer_StartAPI , but serving every connection from io_uring instead of the epoll layer : each
*        connection lives as a direct descriptor in a ring's own file table , so how many of them one process
*        can hold stops being bounded by RLIMIT_NOFILE. Needs Linux 5.19 or newer ; where the rings cannot be
*        set up this falls back to the epoll layer and says so , rather than failing to start.
*        AmmServer_StartAPI's arguments , plus :
* @param How many rings , 1 being the ordinary case. A ring's table may be declared no larger than
*        RLIMIT_NOFILE , but that is checked once per table , so N rings hold N times as many connections -
*        from the same one thread , with the same one SQLite writer. Rings fill in turn , so a server that
*        never exceeds one table's worth of connections never touches the second ring.
* @retval A running instance , or 0 on failure
*/
struct AmmServer_Instance * AmmServer_StartAPIOnRings(const char * name ,const char * ip,unsigned int port,const char * conf_file,void * callback,unsigned int maxResponseSize,unsigned int rings);

/**
* @brief AmmServer_StartAPIOnRings , with SO_REUSEPORT set on the listening socket before it is bound. That
*        lets several processes ( or instances ) of the same user listen on the same ip:port , each with its
*        own socket , and the kernel spreads new connections evenly over them by hashing each connection's
*        addresses. It is how a prefork server gets more than one core onto API mode , since one instance's
*        rings are served by one thread. It also means a second server started on a port by mistake binds
*        happily and takes a share of the connections instead of failing , which is why it is not the
*        default.
* @ingroup core
* @param The same arguments as AmmServer_StartAPIOnRings
* @retval A running instance , or 0 on failure
*/
struct AmmServer_Instance * AmmServer_StartAPIOnRingsReusingPort(const char * name ,const char * ip,unsigned int port,const char * conf_file,void * callback,unsigned int maxResponseSize,unsigned int rings);

/**
* @brief Find one header in an API request , e.g. AmmServer_APIRequestHeader(rqst,"Authorization",&length).
*        Case insensitive on the header name. The value returned points inside the request and is not NUL
*        terminated , so always use the length that comes back with it.
* @ingroup core
* @param An AmmarServer API request
* @param Header name , without the ':'
* @param Output : length of the value found
* @retval Pointer to the header value with leading whitespace skipped , or 0 if the request didn't carry it
*/
const char * AmmServer_APIRequestHeader(const struct AmmServer_APIRequest * rqst,const char * headerName,unsigned int * valueLength);

/**
* @brief The bearer token out of a request's Authorization header , e.g. for
*        AmmServer_APIRequestBearer(rqst,&length) on "Authorization: Bearer eyJhbGci...".
*        The scheme name is matched case insensitively ; the token returned points inside the request , is
*        not NUL terminated , and stops at the first space , so always use the length that comes back.
* @ingroup core
* @param An AmmarServer API request
* @param Output : length of the token found
* @retval Pointer to the token , or 0 if the request carried no Authorization header or its scheme was not
*         Bearer. A Bearer header with nothing after the scheme returns non-zero with length 0 - an empty
*         credential rather than a missing one , which is usually a different answer
*/
const char * AmmServer_APIRequestBearer(const struct AmmServer_APIRequest * rqst,unsigned int * tokenLength);

/**
* @brief One query string parameter , e.g. AmmServer_APIRequestQueryParam(rqst,"limit",&length) for
*        GET /feed?limit=20 . The value points inside the request , is not NUL terminated , and percent
*        escapes and '+' are left exactly as the client sent them , like every other part of the request.
* @ingroup core
* @param An AmmarServer API request
* @param Parameter name , without the '=' , matched exactly
* @param Output : length of the value , which is 0 for a name that appeared without one
* @retval Pointer to the value , or 0 if the request had no such parameter. A parameter present without a
*         value ( "?debug" ) returns a non-zero pointer with length 0 , so it can be told from an absent one
*/
const char * AmmServer_APIRequestQueryParam(const struct AmmServer_APIRequest * rqst,const char * name,unsigned int * valueLength);

/**
* @brief Match a request's path against a template and capture what it holds , in place of hand written
*        memcmp routing : AmmServer_APIRequestMatch(rqst,"/posts/%lld/like",&postId) .
*
*        The template is matched literally except for two conversions , and is anchored at both ends - it
*        is the whole path or it is not a match. Neither conversion ever crosses a '/' , so each one is a
*        single path segment :
*
*        %lld  takes a long long * . Matches a run of decimal digits that is a positive number and cannot
*              overflow , and nothing else : "1.5" , "-1" , "0" , "01x" and an empty segment are all simply
*              not a match , which is what lets a bad id be answered 400 rather than parsed loosely.
*        %s    takes a const char ** and an unsigned int * , and captures one non-empty segment without
*              copying it. What comes back points inside the request and is not NUL terminated.
*
*        The method is not part of this ; test rqst->method yourself.
* @ingroup core
* @param An AmmarServer API request
* @param Path template , e.g. "/health" , "/posts/%lld" , "/users/%s/posts/%lld"
* @retval 1 if the path matched , with every capture filled in ; 0 if it did not , with the captures left
*         untouched
*/
int AmmServer_APIRequestMatch(const struct AmmServer_APIRequest * rqst,const char * pathTemplate,...);

/**
* @brief Start a response's body again from nothing , with a status code and optionally a content type :
*        for a handler that has written part of an answer and then found it has to send an error instead.
*        Clears the overflow flag and any extraHeaders with it , since those describe the body being thrown
*        away ( a Content-Encoding on an error message that was never compressed would be wrong ).
* @ingroup core
* @param An AmmarServer API response
* @param Status code to answer with
* @param Content type , or 0 to leave whatever is already set
*/
void AmmServer_APIResponseReset(struct AmmServer_APIResponse * res,unsigned int statusCode,const char * contentType);

/**
* @brief Append bytes to a response body , bounds checked. Once a write has not fitted , the response is
*        marked overflowed and every later write does nothing - so a handler can write a whole response
*        without checking each step and still never overrun the buffer it was given. Check the end with
*        AmmServer_APIResponseOverflowed , or just the return value of the last write.
* @ingroup core
* @param An AmmarServer API response
* @param Bytes to append , which need not be NUL terminated
* @param How many
* @retval 1 if it was appended , 0 if this response has overflowed
*/
int AmmServer_APIResponseWrite(struct AmmServer_APIResponse * res,const char * text,unsigned int length);

/**
* @brief AmmServer_APIResponseWrite for a NUL terminated string.
* @ingroup core
* @retval 1 if it was appended , 0 if this response has overflowed */
int AmmServer_APIResponseWriteText(struct AmmServer_APIResponse * res,const char * text);

/**
* @brief AmmServer_APIResponseWrite for a number , written out in decimal without going through snprintf.
* @ingroup core
* @retval 1 if it was appended , 0 if this response has overflowed */
int AmmServer_APIResponseWriteInteger(struct AmmServer_APIResponse * res,long long value);

/**
* @brief AmmServer_APIResponseWrite for text that is going inside a JSON string , escaped as JSON requires
*        ( '"' , '\\' and the C0 controls ) straight into the response buffer , with no scratch copy. The
*        surrounding quotes are not written , so this is the contents of the string and not the string.
* @ingroup core
* @param An AmmarServer API response
* @param Text to escape and append , which need not be NUL terminated , and may be arbitrary bytes
* @param How many
* @retval 1 if it was appended , 0 if this response has overflowed */
int AmmServer_APIResponseWriteJSONString(struct AmmServer_APIResponse * res,const char * text,unsigned int length);

/**
* @brief Whether anything written to this response did not fit. A handler that answers through the
*        AmmServer_APIResponseWrite* calls only has to ask this once , at the end.
* @ingroup core
* @retval 1 if a write did not fit ( and the body is therefore incomplete ) , 0 if everything fitted
*/
int AmmServer_APIResponseOverflowed(const struct AmmServer_APIResponse * res);

/**
* @brief What AmmServer_JSONGetStringField found. A request body that is not JSON at all and one that is
*        JSON without the field wanted are different answers , because they are usually different errors.
*/
enum AmmServer_JSONFieldResult
{
   AMMSERVER_JSON_FIELD_OK = 0,
   AMMSERVER_JSON_MALFORMED,          /**< @brief The document is not valid JSON */
   AMMSERVER_JSON_FIELD_NOT_A_STRING  /**< @brief Valid JSON , but the field is absent or is not a string */
};

/**
* @brief Validate a JSON document and copy the unescaped value of one top-level string field out of it ,
*        e.g. the "body" of a POSTed {"body":"..."} . The whole document is validated , not just scanned
*        as far as the field , so a malformed body is reported as malformed however it is malformed.
*        \uXXXX escapes are decoded to UTF-8 , surrogate pairs included.
* @ingroup core
* @param The document , which need not be NUL terminated ( a request body is not )
* @param Its length
* @param Name of the top-level field wanted , NUL terminated. The first one of that name wins
* @param Output buffer for the decoded value , which is NUL terminated on return
* @param Its size
* @param Output : the length the value needs , which may be larger than the buffer given - so a value too
*        long to hold can be told apart from an invalid one , which is a different HTTP status
* @retval AMMSERVER_JSON_FIELD_OK , AMMSERVER_JSON_MALFORMED or AMMSERVER_JSON_FIELD_NOT_A_STRING
*/
enum AmmServer_JSONFieldResult AmmServer_JSONGetStringField(
                                                             const char * doc,
                                                             unsigned int docLength,
                                                             const char * field,
                                                             char * out,
                                                             unsigned int outSize,
                                                             unsigned int * outLength
                                                           );

/**
* @brief Escape text into JSON string content : '"' , '\\' and the C0 controls , and nothing else.
*        UTF-8 is passed through , since JSON makes escaping non-ASCII optional and not escaping it is both
*        fewer bytes and less work. The surrounding quotes are not written.
*        AmmServer_APIResponseWriteJSONString is this , straight into a response body.
* @ingroup core
* @param Text to escape , which need not be NUL terminated
* @param Its length
* @param Output buffer
* @param Its size
* @retval Bytes written , or 0 if all of it would not fit ( in which case nothing should be assumed about
*         what was written )
*/
unsigned int AmmServer_JSONEscape(const char * in,unsigned int inLength,char * out,unsigned int outSize);


/**
* @brief Add a scheduler that fires every X seconds  in order to perform mainenance ( defragment databases , save stuff to disk etc )
* @ingroup core
* @param An AmmarServer Instance
* @param Name of resource that will do the callback
* @param Function to be called and provides output when someone asks for resource
* @param Number of seconds between 2 subsequent calls
* @param Number of repetitions to do ( 0 = infinite )
* @retval 1=Success,0=Fail
*/
int AmmServer_AddScheduler
     ( struct AmmServer_Instance * instance,
       const char * resource_name ,
       void * callback,
       unsigned int seconds,
       unsigned int repetitions
    );


/**
* @brief Add a request handler to handle dynamic requests , the core mechanic of AmmarServer
*        Calling this will bind a C function that will be called and produce output when someone asks for a resource
         TODO : Improve this documenatation
* @ingroup core
* @param An AmmarServer Instance
* @param An AmmServer_RH_Context to be populated
* @param Name of resource that should get dynamic responses ( i.e. "index.html" )
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
       //const char * web_root,
       unsigned int allocate_mem_bytes,
       unsigned int callback_every_x_msec,
       void * callback,
       unsigned int scenario
    );


void * AmmServer_EditorCallback(struct AmmServer_DynamicRequest  * rqst);

void * AmmServer_LoginCallback(struct AmmServer_DynamicRequest  * rqst);

//TODO: Add comment here..
int AmmServer_AddEditorResourceHandler
    (
       struct AmmServer_Instance * instance,
       struct AmmServer_RH_Context * context,
       const char * resource_name ,
       //const char * web_root ,
       void * callback
    );

/**
* @brief monitor.html will give information about the server health internals and load , should only be used for debugging
*        otherwise any client will be able to snoop around and see what is happening inside the server
* @ingroup core
* @param An AmmarServer Instance
* @retval 1=Success,0=Fail
*/
int AmmServer_EnableMonitor( struct AmmServer_Instance * instance);


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




///-------------------------------------------------------------------------------
/**
* @brief The client can use this call to see how many POST items have been submitted in an HTTP Request
* @ingroup POST
* @param The Dynamic Request we want to examine
* @retval Number of POST items,0=Failure/No Items */
int _POSTnum(struct AmmServer_DynamicRequest * rqst);

/**
* @brief The client can use this call to retrieve a string value of a specific POST item by submitting its name
  The pointer returned is guaranteed to be null terminated although in the case of a large file that can contain additional null characters
  you should use the last variable to know where it properly ends.
* @ingroup POST
* @param The Dynamic Request we want to examine
* @param The name of the POST field we want to receive a string for
* @param Output size of the POST field we selected using our name
* @retval Pointer to our requested value, 0=Failure*/
char * _POST (struct AmmServer_DynamicRequest * rqst,const char * name,unsigned int * valueLength);

/**
* @brief The client can use this call to retrieve the numeric value ( internally delivered using atoi ) of a specific POST items by submitting its name
* @ingroup POST
* @param The Dynamic Request we want to examine
* @param The name of the POST field we want to receive a number for
* @retval Integer value of the POST item with a given name , 0=Failure*/
unsigned int _POSTuint(struct AmmServer_DynamicRequest * rqst,const char * name);

/**
* @brief Quickly check if a POST field has been submitted or not in a request
* @ingroup POST
* @param The Dynamic Request we want to examine
* @param The Name of the POST item we are checking
* @retval 1=Exists , 0=Does not exist*/
int _POSTexists(struct AmmServer_DynamicRequest * rqst,const char * name);

/**
* @brief Shortcut to compare a POST field to a value ( internally using strcmp(POSTValue,ourValue) )
* @ingroup POST
* @param The Dynamic Request we want to examine
* @param The Name of the POST item we are checking
* @param The Value that we want to strncmp with our value
* @retval See strncmp*/
int _POSTcmp(struct AmmServer_DynamicRequest * rqst,const char * name,const char * what2CompareTo);

/**
* @brief Copy a POST field value to a given buffer
* @ingroup POST
* @param The Dynamic Request we want to examine
* @param The Name of the POST item we want to copy checking
* @param The Destination pointer of where we want to copy to
* @param The Destination pointer maximum accommodation size
* @retval 1=Success , 0=Failure*/
int _POSTcpy(struct AmmServer_DynamicRequest * rqst,const char * name,char * destination,unsigned int destinationSize);
///-------------------------------------------------------------------------------



/**
* @brief The client can use this call to see how many GET items have been submitted in an HTTP Request
* @ingroup GET
* @param The Dynamic Request we want to examine
* @retval Number of GET items,0=Failure/No Items */
int _GETnum(struct AmmServer_DynamicRequest * rqst);

/**
* @brief The client can use this call to retrieve a string value of a specific GET item by submitting its name
  The pointer returned is guaranteed to be null terminated although in the case of a large string that can contain additional null characters
  you should use the last variable to know where it properly ends.
* @ingroup GET
* @param The Dynamic Request we want to examine
* @param The name of the GET field we want to receive a string for
* @param Output size of the GET field we selected using our name
* @retval Pointer to our requested value, 0=Failure*/
char * _GET(struct AmmServer_DynamicRequest * rqst,const char * name,unsigned int * valueLength);

/**
* @brief The client can use this call to retrieve the numeric value ( internally delivered using atoi ) of a specific POST items by submitting its name
* @ingroup GET
* @param The Dynamic Request we want to examine
* @param The name of the POST field we want to receive a number for
* @retval Integer value of the POST item with a given name , 0=Failure*/
unsigned int _GETuint(struct AmmServer_DynamicRequest * rqst,const char * name);


/**
* @brief Quickly check if a GET field has been submitted or not in a request
* @ingroup GET
* @param The Dynamic Request we want to examine
* @param The Name of the GET item we are checking
* @retval 1=Exists , 0=Does not exist*/
int _GETexists(struct AmmServer_DynamicRequest * rqst,const char * name);


/**
* @brief Shortcut to compare a GET field to a value ( internally using strcmp(POSTValue,ourValue) )
* @ingroup GET
* @param The Dynamic Request we want to examine
* @param The Name of the GET item we are checking
* @param The Value that we want to strncmp with our value
* @retval See strncmp*/
int _GETcmp(struct AmmServer_DynamicRequest * rqst,const char * name,const char * what2CompareTo);

/**
* @brief Copy a GET field value to a given buffer
* @ingroup GET
* @param The Dynamic Request we want to examine
* @param The Name of the GET item we want to copy checking
* @param The Destination pointer of where we want to copy to
* @param The Destination pointer maximum accommodation size
* @retval 1=Success , 0=Failure*/
int _GETcpy  (struct AmmServer_DynamicRequest * rqst,const char * name,char * destination,unsigned int destinationSize);
///-------------------------------------------------------------------------------
/**
* @brief The client can use this call to see how many COOKIE items were submitted in an HTTP Request
* @ingroup COOKIE
* @param The Dynamic Request we want to examine
* @retval Number of COOKIE items,0=Failure/No Items */
int _COOKIEnum(struct AmmServer_DynamicRequest * rqst);

/**
* @brief The client can use this call to retrieve a string value of a specific COOKIE item by submitting its name
  Unlike _GET()/_POST() , the pointer returned here is NOT guaranteed to be null terminated : it points
  straight into the shared incoming header buffer and , for the last cookie on the Cookie header line ,
  there is no NUL within it to find (only the header's own CRLF sits right after it, and writing one in
  would corrupt header parsing) , so always use the returned length rather than assuming a C string.
  _COOKIEcpy() / _COOKIEcmp() already do this correctly if you just need a copy or a comparison.
* @ingroup COOKIE
* @param The Dynamic Request we want to examine
* @param The name of the COOKIE field we want to receive a string for
* @param Output size of the COOKIE field we selected using our name
* @retval Pointer to our requested value, 0=Failure*/
const char * _COOKIE(struct AmmServer_DynamicRequest * rqst,const char * name,unsigned int * valueLength);

/**
* @brief The client can use this call to retrieve the numeric value ( internally delivered using atoi ) of a specific COOKIE item by submitting its name
* @ingroup COOKIE
* @param The Dynamic Request we want to examine
* @param The name of the COOKIE field we want to receive a number for
* @retval Integer value of the COOKIE item with a given name , 0=Failure*/
unsigned int _COOKIEuint(struct AmmServer_DynamicRequest * rqst,const char * name);

/**
* @brief Quickly check if a COOKIE field has been submitted or not in a request
* @ingroup COOKIE
* @param The Dynamic Request we want to examine
* @param The Name of the COOKIE item we are checking
* @retval 1=Exists , 0=Does not exist*/
int _COOKIEexists(struct AmmServer_DynamicRequest * rqst,const char * name);

/**
* @brief Shortcut to compare a COOKIE field to a value ( internally using strcmp(COOKIEValue,ourValue) )
* @ingroup COOKIE
* @param The Dynamic Request we want to examine
* @param The Name of the COOKIE item we are checking
* @param The Value that we want to strcmp with our value
* @retval See strcmp*/
int _COOKIEcmp(struct AmmServer_DynamicRequest * rqst,const char * name,const char * what2CompareTo);

/**
* @brief Copy a COOKIE field value to a given buffer
* @ingroup COOKIE
* @param The Dynamic Request we want to examine
* @param The Name of the COOKIE item we want to copy checking
* @param The Destination pointer of where we want to copy to
* @param The Destination pointer maximum accommodation size
* @retval 1=Success , 0=Failure*/
int _COOKIEcpy(struct AmmServer_DynamicRequest * rqst,const char * name,char * destination,unsigned int destinationSize);

/**
* @brief Queues a Set-Cookie response header - see cache/session_list.h for the session system this exists to
*        support, but it's a general-purpose primitive any resource handler can call for its own cookies too.
*        Actually sent by SendSuccessCodeHeader() when the response header goes out ; has no effect on requests
*        served by the epoll fast path ( see epollFastPathServer.c ), which never invokes a dynamic callback.
* @ingroup COOKIE
* @param The Dynamic Request to queue this cookie on
* @param Cookie name
* @param Cookie value
* @param 0=browser-session cookie ( no Max-Age ) , >0=expires after this many seconds , <0=expire immediately ( clears it )
* @param 1=mark HttpOnly ( not readable from JS ) , 0=don't
* @retval 1=Success , 0=Failure ( bad args, or wouldn't fit the pending-header buffer )*/
int AmmServer_SetCookie(struct AmmServer_DynamicRequest * rqst,const char * name,const char * value,int maxAgeSeconds,int httpOnly);
///-------------------------------------------------------------------------------

/**
* @brief Get a SESSION field value by name - the PHP $_SESSION['name'] equivalent. Only resolves anything for
*        resources registered with useSessionLifecycle set ( see AmmServer_RH_Context ) - see cache/session_list.h
*        for how the underlying session store works.
* @ingroup SESSION
* @param The Dynamic Request we want to examine
* @param The name of the SESSION field we want to receive a string for
* @param Output size of the SESSION field we selected using our name
* @retval Pointer to a thread-local scratch copy of the value ( valid until this thread's next _SESSION*() call -
           copy it out immediately if you need it longer, same caveat _SESSIONcpy() exists to avoid ) , or 0=Not found*/
const char * _SESSION(struct AmmServer_DynamicRequest * rqst,const char * name,unsigned int * valueLength);

/**
* @brief Copy a SESSION field value to a given buffer
* @ingroup SESSION
* @param The Dynamic Request we want to examine
* @param The name of the SESSION field we want to copy
* @param The Destination pointer of where we want to copy to
* @param The Destination pointer maximum accommodation size
* @retval 1=Success , 0=Failure*/
int _SESSIONcpy(struct AmmServer_DynamicRequest * rqst,const char * name,char * destination,unsigned int destinationSize);

/**
* @brief Get the numeric value ( internally delivered using atoi ) of a SESSION field by name
* @ingroup SESSION
* @retval Integer value of the SESSION item with a given name , 0=Failure*/
unsigned int _SESSIONuint(struct AmmServer_DynamicRequest * rqst,const char * name);

/**
* @brief Quickly check if a SESSION field has been set or not
* @ingroup SESSION
* @retval 1=Exists,0=Does not Exist*/
int _SESSIONexists(struct AmmServer_DynamicRequest * rqst,const char * name);

/**
* @brief Set ( or overwrite ) a SESSION field - the PHP $_SESSION['name']=value; equivalent
* @ingroup SESSION
* @retval 1=Success , 0=Failure*/
int _SESSIONset(struct AmmServer_DynamicRequest * rqst,const char * name,const char * value);

/**
* @brief Remove a SESSION field - the PHP unset($_SESSION['name']); equivalent
* @ingroup SESSION
* @retval 1=Removed , 0=Not found*/
int _SESSIONunset(struct AmmServer_DynamicRequest * rqst,const char * name);

/**
* @brief Issues ( or reuses, if this session already has one ) an unpredictable, session-bound CSRF token - the
*        building block for protecting any state-changing GET/POST endpoint from Cross-Site Request Forgery.
*        Requires the calling resource to have useSessionLifecycle=1 set, same as the rest of the _SESSION*
*        family. Embed the returned value in a hidden form field ( or a custom header, for an AJAX/JSON API )
*        and check it back with AmmServer_ValidateCSRFToken() before performing the state-changing action - a
*        cross-site attacker page can trigger the request but can never read this session's own token value,
*        since the browser's same-origin policy blocks it from reading the response containing it, and the
*        token is never accepted from any *other* session ( unlike a plain "did the client fetch a valid-
*        looking token from us recently" check, which an attacker can satisfy from their own session/browser
*        and then replay against a victim ).
* @ingroup SESSION,security
* @retval 1=Success ( outToken holds the token ) , 0=Failure ( bad args, or the CSPRNG couldn't be reached )*/
int AmmServer_GenerateCSRFToken(struct AmmServer_DynamicRequest * rqst,char * outToken,unsigned int outTokenSize);

/**
* @brief Checks a client-submitted token against the one issued to this session via AmmServer_GenerateCSRFToken().
* @ingroup SESSION,security
* @retval 1=Valid , 0=Invalid/missing/no token was ever issued for this session*/
int AmmServer_ValidateCSRFToken(struct AmmServer_DynamicRequest * rqst,const char * submittedToken);

/**
* @brief Shorthand/Shortcut for AmmServer_FILES()
* @ingroup shortcut */
const char * _FILES(struct AmmServer_DynamicRequest * rqst,const char * POSTName,enum TypesOfRequestFields POSTType,unsigned int * outputSize);
///-------------------------------------------------------------------------------

/**
* @brief Staged way to easily handle bad clients etc from the clients , currently a stub..!
* @ingroup shortcut
* @bug Client behaviours etc are not implemented yet */
int AmmServer_SignalCountAsBadClientBehaviour(struct AmmServer_DynamicRequest * rqst);

/**
* @brief Save Dynamic Request to file
* @ingroup tools
* @param Filename to save the dynamic request
* @param Instance of an AmmarServer
* @param Request that we want to save to a file ( see AmmServer_DynamicRequest )
* @retval 1=Success,0=Failure */
int AmmServer_SaveDynamicRequest(const char* filename  , struct AmmServer_DynamicRequest * rqst);



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



int AmmServer_ExecuteCommandLineAndRetreiveAllResults(
                                                       const char *  command ,
                                                       char * what2GetBack ,
                                                       unsigned int what2GetBackMaxSize,
                                                       unsigned long * what2GetBackSize
                                                     );



int AmmServer_ExecuteCommandLineAndRetreiveAllResultsTimed(const char *  command , char * what2GetBack , unsigned int what2GetBackMaxSize, unsigned long * what2GetBackSize,int waitTime);


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
int AmmServer_WriteFileFromMemory(const char * filename,const char * memory , unsigned int memoryLength);


/**
* @brief Read a file and store it to a freshly allocated memory handler context
* @ingroup tools
* @param Input Filename
* @retval Pointer to the new memory handler or 0=Failed
*/
struct AmmServer_MemoryHandler *  AmmServer_ReadFileToMemoryHandler(const char * filename);



/**
* @brief Copy a memory handler
* @ingroup tools
* @param Input memory handle
* @retval Pointer to the new memory handler or 0=Failed
*/
struct AmmServer_MemoryHandler *  AmmServer_CopyMemoryHandler(struct AmmServer_MemoryHandler * inpt);


int filterStringForShellInjection(char * buffer , unsigned int bufferSize);
int filterStringForHtmlInjection(char * buffer , unsigned int bufferSize);

/**
* @brief Search for entryPoint pattern in buffer , and inject data there..!
* @ingroup tools
* @param Memory Handler for Buffer we want to inject to , see struct AmmServer_MemoryHandler
* @param String to find in buffer and replace with new content
* @param Data we want to inject
* @retval 1=Ok,0=Failed
*/
int AmmServer_ReplaceVariableInMemoryHandler(struct AmmServer_MemoryHandler * mh,const char * var,const char * value);



/**
* @brief Hot-Replace a character inside a memory block , typically used to replace characters like '+' with ' '
* @ingroup tools
* @param Pointer to memory that contains the null terminated string
* @param Character to be replaced
* @param What to replace the character with
*/
void AmmServer_ReplaceCharInString(char * input , char findChar , char replaceWith);

/**
* @brief Search for entryPoint pattern in buffer multiple times , and inject data in each one..!
* @ingroup tools
* @param Memory Handler for Buffer we want to inject to , see struct AmmServer_MemoryHandler
* @param Number of instances we want to replace
* @param String to find in buffer and replace with new content
* @param Data we want to inject
* @retval 1=Ok,0=Failed
  @Bug If the length of var is smaller than the length of value there might be problems (?)
*/
int AmmServer_ReplaceAllVarsInMemoryHandler(struct AmmServer_MemoryHandler * mh ,unsigned int instances,const char * var,const char * value);


int AmmServer_ReplaceAllVarsInDynamicRequest(struct AmmServer_DynamicRequest * dr ,unsigned int instances,const char * var,const char * value);

struct AmmServer_MemoryHandler * AmmServer_AllocateMemoryHandler(unsigned int initialBufferLength, unsigned int growStep);
int AmmServer_FreeMemoryHandler(struct AmmServer_MemoryHandler ** mh);



/**
* @brief Register a function to call a function that gracefully terminates a client when a SIGKILL or the time to stop the server comes
* @ingroup tools
* @param Pointer to function
* @retval 1=Exists,0=Does not Exist
*/
int AmmServer_RegisterTerminationSignal(void * callback);



const char * AmmServer_GetDirectoryFromPath(char * path);
const char * AmmServer_GetBasenameFromPath(char * path);
const char * AmmServer_GetExtensionFromPath(char * path);

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



int AmmServer_FileIsText(const char * filename);
int AmmServer_FileIsAudio(const char * filename);
int AmmServer_FileIsImage(const char * filename);

/**
* @brief Check if file is a video
* @ingroup tools
* @param Path to file
* @retval 1=Exists,0=Does not Exist
*/
int AmmServer_FileIsVideo(const char * filename);
int AmmServer_FileIsFlash(const char * filename);

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

/**
* @brief Escapes the characters that are unsafe to place directly into HTML ( & < > " ' ) into their entity
*        equivalents , so arbitrary user supplied text ( post bodies, names, form fields, ... ) can be safely
*        embedded into a page without allowing HTML/script injection or breaking out of a quoted attribute.
*        Unlike AmmServer_StringIsHTMLSafe() this does not reject anything , it makes any input safe to use.
* @ingroup tools
* @param Input string to escape
* @param Output buffer that receives the escaped , always NUL terminated , string
* @param Size of the output buffer , escaping stops before it would overflow it ( no partial entities are ever emitted )
* @retval Number of bytes written to output , not including the terminating NUL
*/
unsigned int AmmServer_HTMLEscape(const char * input , char * output , unsigned int outputMaxSize);

/**
* @brief Check if a string has html elements inside it , so if we append it to a web site we won't have html injected
* @ingroup tools
* @param Directory path
* @param Input String
* @retval 1=Safe,0=Unsafe
*/
unsigned int AmmServer_StringHasSafePath( const char * directory , const char * filenameUNSANITIZEDString);



#ifdef __cplusplus
}
#endif

#endif // AMMSERVERLIB_H_INCLUDED
