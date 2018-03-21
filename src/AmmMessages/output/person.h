
/** @file person.h
* @brief A tool that generates code that can span messages across computers
* @author Ammar Qammaz (AmmarkoV)
*/

/*
This file was automatically generated @ 21-03-2018 03:08:41 using AmmMessages
https://github.com/AmmarkoV/AmmarServer/tree/master/src/AmmMessages
Please note that changes you make here may be automatically overwritten
if the AmmMessages generator runs again..!
 */

#ifndef PERSON_H_INCLUDED
#define PERSON_H_INCLUDED


#include "mmapBridge.h"

const char * pathToMMAPperson="/home/nao/mmap/person.mmap";
static struct bridgeContext personBridge= {0};

struct personMessage
{
    unsigned long timestampInit;
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



/** @brief Send a person message through the bridge
* @ingroup stringParsing
* @param The bridge context
* @param The message to send
* @retval See above enumerator*/
static int write_person(struct bridgeContext * nbc,struct personMessage * msg)
{
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
        nbc->lastMsgTimestamp = msg->timestampInit;
        return 1;
    }
    return 0;
}



/** @brief If we don't have AmmarServer included then we don't need the rest of the code*/
#ifdef AMMSERVERLIB_H_INCLUDED


/** @brief This is the static memory location where we receive stuff from the scope of the webserver*/
static struct personMessage personStatic= {0};



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
    write_person(&personBridge,&personStatic);
    snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>OK</body></html>");
    rqst->contentSize=strlen(rqst->content);

    return 0;

}



/** @brief This function adds person.html to the webserver and makes it listen for GET commands and forward them to the mmaped structure personStatic */
static int personAddToHTTPServer(struct AmmServer_Instance * instance)
{
    if ( initializeWritingBridge(&personBridge , pathToMMAPperson , sizeof(struct personMessage)) )    {
        AmmServer_Success("Successfully initialized mmaped bridge");
        struct personMessage empty= {0};
        empty.timestampInit = rand()%10000;
        if ( write_person(&personBridge,&empty) )      {
            AmmServer_Success("Successfully flushed mmaped region");
        }
        else      {
            AmmServer_Error("Could not flush mmaped region");
        }
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

#endif