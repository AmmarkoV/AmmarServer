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

const struct POSTRequestBoundaryContent * getPOSTItemFromName(struct AmmServer_DynamicRequest * rqst,const char * nameToLookFor);

char * getPointerToPOSTItemValue(struct AmmServer_DynamicRequest * rqst,const char * nameToLookFor,unsigned int * pointerLength);
char * getPointerToPOSTItemFilename(struct AmmServer_DynamicRequest * rqst,const char * nameToLookFor,unsigned int * pointerLength);
char * getPointerToPOSTItemType(struct AmmServer_DynamicRequest * rqst,const char * nameToLookFor,unsigned int * pointerLength);

int getNumberOfPOSTItems(struct AmmServer_DynamicRequest * rqst);


#endif // POSTHEADERS_H_INCLUDED
