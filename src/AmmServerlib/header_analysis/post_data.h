/** @file post_data.h
* @brief Functions to facilitate POST data parsing
* @author Ammar Qammaz (AmmarkoV)
* @bug POST header analysis is not fully implemented yet
*/

#ifndef POSTDATA_H_INCLUDED
#define POSTDATA_H_INCLUDED

#include "../AmmServerlib.h"


int wipePOSTData(struct HTTPHeader * output);

int createPOSTData(struct HTTPHeader * output);

int addPOSTDataBoundary(struct HTTPHeader * output,char * ptr);


int finalizePOSTData(struct HTTPHeader * output);


// ----------------------------------
char * getPointerToPOSTItem(struct AmmServer_DynamicRequest * rqst,const char * nameToLookFor,unsigned int * pointerLength);
int getNumberOfPOSTItems(struct AmmServer_DynamicRequest * rqst);


#endif // POSTHEADERS_H_INCLUDED
