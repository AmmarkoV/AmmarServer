
/** @file person.h
* @brief A tool that generates code that can span messages across computers
* @author Ammar Qammaz (AmmarkoV)
*/

/*                  
This file was automatically generated @ 20-03-2018 19:12:08 using AmmMessages                  
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
