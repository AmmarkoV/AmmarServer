#ifndef HTTP_HEADER_ANALYSIS_H_INCLUDED
#define HTTP_HEADER_ANALYSIS_H_INCLUDED

#include "configuration.h"

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


struct HTTPRequest
{
   int  requestType;
   char resource[MAX_RESOURCE+1];
   char verified_local_resource[MAX_FILE_PATH+1];
   char GETquery[MAX_QUERY+1];
   char POSTquery[4*MAX_QUERY+1];
   unsigned char authorized;
   unsigned char keepalive;
   unsigned char supports_gzip;

   //RANGE DATA
   unsigned long range_start;
   unsigned long range_end;

   /*! IMPORTANT update FIELDS_TO_CLEAR_FROM_HTTP_REQUEST when I add something here.. */
   char * Cookie; //<-   *THIS SHOULD BE CLEARED AFTER USAGE*
   char * Host; //<-     *THIS SHOULD BE CLEARED AFTER USAGE*
   char * Referer; //<-  *THIS SHOULD BE CLEARED AFTER USAGE*
   char * UserAgent;//<- *THIS SHOULD BE CLEARED AFTER USAGE*
   char * ContentType; //<- for POST requests *THIS SHOULD BE CLEARED AFTER USAGE*
   unsigned long ContentLength; //<- for POST requests
   //Languages etc here..!
};
/*! IMPORTANT @@@ */
#define FIELDS_TO_CLEAR_FROM_HTTP_REQUEST 5
/*! IMPORTANT @@@*/

int FreeHTTPRequest(struct HTTPRequest * output);
int HTTPRequestComplete(char * request,unsigned int request_length);
int AnalyzeHTTPRequest(struct HTTPRequest * output,char * request,unsigned int request_length, char * webserver_root);

#endif // HTTP_HEADER_ANALYSIS_H_INCLUDED
