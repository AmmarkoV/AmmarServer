/** @file cookie_data.h
* @brief Functions to facilitate GET Cookie data parsing
* @author Ammar Qammaz (AmmarkoV)
* @bug GET header analysis is not fully implemented yet
*/

#ifndef COOKIEDATA_H_INCLUDED
#define COOKIEDATA_H_INCLUDED

#include "../AmmServerlib.h"


int wipeCOOKIEData(struct HTTPHeader * output);
int createCOOKIEData(struct HTTPHeader * output);
int finalizeCOOKIEData(struct HTTPHeader * output,char * value,unsigned int valueLength);


const struct GETRequestContent * getCOOKIEItemFromName(struct AmmServer_DynamicRequest * rqst,const char * nameToLookFor);
char * getPointerToGETItemValue(struct AmmServer_DynamicRequest * rqst,const char * nameToLookFor,unsigned int * pointerLength);
int getNumberOfCOOKIEItems(struct AmmServer_DynamicRequest * rqst);

#endif //COOKIEDATA_H_INCLUDED
