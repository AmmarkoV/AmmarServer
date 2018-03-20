
/** @file person.h
* @brief A tool that generates code that can span messages across computers
* @author Ammar Qammaz (AmmarkoV)
*/

/*                  
This file was automatically generated @ 20-03-2018 20:07:40 using AmmMessages                  
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
// #Confidence is a float ranging 0.0 to 1.0  
// float32 confidence  
//   
//    
// int32 source  
// int32 inFieldOfView  
//   
//   
// #pose  
// float32 x  
// float32 y  
// float32 z  
// float32 theta  
//    
// #Timestamps are actually the number of the frame  
// int32 timestamp  
// time stamp  
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
int read_person(struct bridgeContext * nbc,struct personMessage* msg)
{
  if (readBridge(nbc, (void*) msg , sizeof(struct personMessage) ) )
  {
    nbc->lastMsgTimestamp = msg->timestampInit;
    return 1;
  }
 return 0;
}

#endif
