
/** @file person.h
* @brief A tool that generates code that can span messages across computers
* @author Ammar Qammaz (AmmarkoV)
*/

/*
This file was automatically generated @ 20-03-2018 23:53:40 using AmmMessages
https://github.com/AmmarkoV/AmmarServer/tree/master/src/AmmMessages
Please note that changes you make here may be automatically overwritten
if the AmmMessages generator runs again..!
 */

#ifndef PERSON_H_INCLUDED
#define PERSON_H_INCLUDED


#include "mmapBridge.h"

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

#ifdef AMMSERVERLIB_H_INCLUDED
static struct personMessage personStatic;
static void * personHTTPServer(struct AmmServer_DynamicRequest  * rqst)
{
    char value[256];
    if ( _GETcpy(rqst,(char*)"confidence",value,255) )   {
        personStatic.confidence=confidence;
    }
    else if ( _GETcpy(rqst,(char*)"source",value,255) )   {
        personStatic.source=source;
    }
    else if ( _GETcpy(rqst,(char*)"inFieldOfView",value,255) )   {
        personStatic.inFieldOfView=inFieldOfView;
    }
    else if ( _GETcpy(rqst,(char*)"x",value,255) )   {
        personStatic.x=x;
    }
    else if ( _GETcpy(rqst,(char*)"y",value,255) )   {
        personStatic.y=y;
    }
    else if ( _GETcpy(rqst,(char*)"z",value,255) )   {
        personStatic.z=z;
    }
    else if ( _GETcpy(rqst,(char*)"theta",value,255) )   {
        personStatic.theta=theta;
    }
    else if ( _GETcpy(rqst,(char*)"timestamp",value,255) )   {
        personStatic.timestamp=timestamp;
    }
    else if ( _GETcpy(rqst,(char*)"stamp",value,255) )   {
        personStatic.stamp=stamp;
    }
    else
    {  }
    snprintf(rqst->content,rqst->MAXcontentSize,"<html><body>OK</body></html>");
    unsigned int commandLength=strlen(rqst->content);

    return 0;

}

static int personAddToHTTPServer(struct AmmServer_Instance * instance,struct AmmServer_RH_Context * context)
{
    return AmmServer_AddResourceHandler(instance,context,"/person.html",2048+sizeof(struct personMessage),0,&personHTTPServer,SAME_PAGE_FOR_ALL_CLIENTS);
}

#endif

#endif
