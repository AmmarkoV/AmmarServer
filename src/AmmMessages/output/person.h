
/** @file person.h
* @brief A tool that generates code that can span messages across computers
* @author Ammar Qammaz (AmmarkoV)
*/

/*
This file was automatically generated @ 22-03-2018 18:46:28 using AmmMessages
https://github.com/AmmarkoV/AmmarServer/tree/master/src/AmmMessages
Please note that changes you make here may be automatically overwritten
if the AmmMessages generator runs again..!
 */

#ifndef PERSON_H_INCLUDED
#define PERSON_H_INCLUDED


#include <stdio.h>

#include <string.h>

#include "mmapBridge.h"

const char * pathToMMAPperson="/home/ammar/mmap/person.mmap";
static struct bridgeContext personBridge= {0};

struct personMessage
{
    unsigned long timestampInit;
    void * callbackOnNewData;
//#Confidence is a float ranging 0.0 to 1.0
    float confidence;
//
//
    unsigned int source;
    unsigned int inFieldOfView;
//
//
//#pose
    float x;
    float y;
    float z;
    float theta;
//
//#Timestamps are actually the number of the frame
    unsigned int timestamp;
//time stamp
//
};



/** @brief This is the static memory location where we receive stuff so we don't even have to declare this..*/
static struct personMessage personStatic= {0};



/** @brief Register a callback that will get called when person is updated*/
static int registerCallbackOnNewData_person(void * callback)
{
    personStatic.callbackOnNewData = callback;
    return 1;
}



/** @brief Send a person message through the bridge
* @ingroup stringParsing
* @param The bridge context
* @param The message to send
* @retval See above enumerator*/
static int write_person(struct bridgeContext * nbc,struct personMessage * msg)
{
    printf("updating person message.. \n ");
    return writeBridge(nbc, (void*) msg , sizeof(struct personMessage) );
}



/** @brief Receive a person message through the bridge
* @ingroup stringParsing
* @param The bridge context
* @param The message to receive
* @retval See above enumerator*/
static int read_person(struct bridgeContext * nbc,struct personMessage* msg)
{
    if (readBridge(nbc, (void*) msg , sizeof(struct personMessage) ) )
    {
        if (personStatic.callbackOnNewData!=0)
        {
            void ( *DoCallback) ( struct personMessage * ) = 0 ;
            DoCallback=personStatic.callbackOnNewData;
            DoCallback(&personStatic);
        }
        nbc->lastMsgTimestamp = msg->timestampInit;
        return 1;
    }
    return 0;
}



/** @brief Empty bridge  state for person */
static int empty_person()
{
    memset(&personStatic,0,sizeof(struct personMessage));
    return 1;
}



/** @brief This call crafts a request to person.html and serializes our message*/
static int packToHTTPGETRequest_person(char * buffer,unsigned int bufferSize,struct personMessage * msg)
{
    return snprintf(buffer,bufferSize,"/person.html?confidence=%0.5f&source=%u&inFieldOfView=%u&x=%0.5f&y=%0.5f&z=%0.5f&theta=%0.5f&timestamp=%u&stamp=t=89383",msg->confidence,msg->source,msg->inFieldOfView,msg->x,msg->y,msg->z,msg->theta,msg->timestamp);
}



/** @brief Print message for person */
static int print_person(struct personMessage * msg)
{
    char buffer[2049]= {0};
    unsigned int bufferSize=2048;
    packToHTTPGETRequest_person(buffer,bufferSize,msg);
    printf("%s",buffer);
    return 1;
}



/** @brief Initialize a bridge to write values person */
static int initializeForWriting_person()
{
    if ( initializeWritingBridge(&personBridge , pathToMMAPperson , sizeof(struct personMessage)) )    {
        struct personMessage empty= {0};
        empty.timestampInit = rand()%10000;
        if ( write_person(&personBridge,&empty) ) {
            return 1;
        }
        else {
            return 0;
        }
    } else {
        return 0;
    }
}



/** @brief Initialize a bridge to read values person */
static int initializeForReading_person()
{
    if ( initializeReadingBridge(&personBridge , pathToMMAPperson , sizeof(struct personMessage)) )    {
        personStatic.timestampInit = rand()%10000;
        if ( read_person(&personBridge,&personStatic) ) {
            return 1;
        }
        else {
            return 0;
        }
    } else {
        return 0;
    }
}



/** @brief If we don't have AmmarServer included then we don't need the rest of the code*/
#ifdef AMMSERVERLIB_H_INCLUDED


/** @brief This is the Resource handler context that will manage requests to person.html */
static struct AmmServer_RH_Context personRH= {0};
static void * personHTTPServer(struct AmmServer_DynamicRequest  * rqst)
{
    char value[256]= {0};
    if ( _GETcpy(rqst,(char*)"confidence",value,255) )   {
        personStatic.confidence=atof(value);
    }
    if ( _GETcpy(rqst,(char*)"source",value,255) )   {
        personStatic.source=atoi(value);
    }
    if ( _GETcpy(rqst,(char*)"inFieldOfView",value,255) )   {
        personStatic.inFieldOfView=atoi(value);
    }
    if ( _GETcpy(rqst,(char*)"x",value,255) )   {
        personStatic.x=atof(value);
    }
    if ( _GETcpy(rqst,(char*)"y",value,255) )   {
        personStatic.y=atof(value);
    }
    if ( _GETcpy(rqst,(char*)"z",value,255) )   {
        personStatic.z=atof(value);
    }
    if ( _GETcpy(rqst,(char*)"theta",value,255) )   {
        personStatic.theta=atof(value);
    }
    if ( _GETcpy(rqst,(char*)"timestamp",value,255) )   {
        personStatic.timestamp=atoi(value);
    }
#if DEBUG_PRINT
    fprintf(stderr,"HTTPServer: Received a person message \n ");
    print_person(&personStatic);
#endif
    write_person(&personBridge,&personStatic);
    snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>OK</body></html>");
    rqst->contentSize=strlen(rqst->content);

    return 0;

}



/** @brief This function adds person.html to the webserver and makes it listen for GET commands and forward them to the mmaped structure personStatic */
static int personAddToHTTPServer(struct AmmServer_Instance * instance)
{
    if ( initializeForWriting_person() )    {
        AmmServer_Success("Successfully initialized mmaped bridge");
    }
    else    {
        AmmServer_Error("Could not initialize mmaped bridge ");
    }
    return AmmServer_AddResourceHandler(instance,&personRH,"/person.html",2048+sizeof(struct personMessage),0,&personHTTPServer,SAME_PAGE_FOR_ALL_CLIENTS);
}

static int personRemoveFromHTTPServer(struct AmmServer_Instance * instance)
{
    AmmServer_RemoveResourceHandler(instance,&personRH,1);
    closeWritingBridge(&personBridge);
}
#endif



/** @brief If we don't have AmmarClient included then we won't use it and we don't need the rest of the code*/
#ifdef AMMCLIENT_H_INCLUDED


/** @brief If this instance of your code is running on a machine that is connected through TCP/IP and you want to send your data you can do it using this call person.html */
static int sendToServer_person(struct AmmClient_Instance * instance,struct personMessage * msg)
{
    char buffer[2049]= {0};
    unsigned int bufferSize=2048;
    packToHTTPGETRequest_person(buffer,bufferSize,msg);
    char http[2049]= {0};
    unsigned int httpSize=2048;
    snprintf(http,httpSize,"GET %s HTTP/1.1\nConnection: keep-alive\n\n",buffer);
    return AmmClient_Send(instance,http,strlen(http),1);
}
#endif

#endif
