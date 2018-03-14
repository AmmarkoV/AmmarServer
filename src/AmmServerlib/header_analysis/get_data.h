/** @file get_data.h
* @brief Functions to facilitate GET data parsing
* @author Ammar Qammaz (AmmarkoV)
* @bug GET header analysis is not fully implemented yet
*/

#ifndef GETDATA_H_INCLUDED
#define GETDATA_H_INCLUDED

#include "../AmmServerlib.h"


int wipeGETData(struct HTTPHeader * output);

int createGETData(struct HTTPHeader * output);


int finalizeGenericGETField(
                             struct HTTPHeader * output,
                             struct GETRequestContent * target ,
                             unsigned int * targetNumber ,
                             char * value,
                             unsigned int valueLength
                           );

int finalizeGETData(struct HTTPHeader * output);


// ----------------------------------

const struct GETRequestContent * getGETItemFromName(struct AmmServer_DynamicRequest * rqst,const char * nameToLookFor);

char * getPointerToGETItemValue(struct AmmServer_DynamicRequest * rqst,const char * nameToLookFor,unsigned int * pointerLength);

int getNumberOfGETItems(struct AmmServer_DynamicRequest * rqst);


#endif // GETHEADERS_H_INCLUDED
