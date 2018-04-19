#include "cookie_data.h"
#include "get_data.h"
#include "generic_header_tools.h"

#include "../tools/http_tools.h"

#include <stdio.h>
#include <string.h>

int wipeCOOKIEData(struct HTTPHeader * output)
{
  output->COOKIEItemNumber=0;
  memset(output->COOKIEItem,0,MAX_HTTP_GET_VARIABLE_COUNT*sizeof(struct GETRequestContent));
  return 1;
}

int createCOOKIEData(struct HTTPHeader * output)
{
  return (wipeCOOKIEData(output));
}

int finalizeCOOKIEData(struct HTTPHeader * output,char * value,unsigned int valueLength)
{
  return 0;
  /*
  createCOOKIEData(output);

  return finalizeGenericGETField(
                                 output,
                                 output->COOKIEItem ,
                                 &output->COOKIEItemNumber ,
                                 value,
                                 valueLength
                                );

 return 1;*/
}
/*
----------------------------------------------
              ACCESS COOKIE DATA
----------------------------------------------
*/
const struct GETRequestContent * getCOOKIEItemFromName(struct AmmServer_DynamicRequest * rqst,const char * nameToLookFor)
{
 unsigned int sizeOfNameToLookFor = strlen(nameToLookFor);

 unsigned int i=0;
 unsigned int PNum=rqst->COOKIEItemNumber;
 if (PNum>MAX_HTTP_GET_VARIABLE_COUNT) { PNum=MAX_HTTP_GET_VARIABLE_COUNT; }

 if (rqst->COOKIEItem!=0)
 {
  for (i=0; i<PNum; i++)
  {
    struct GETRequestContent * p = &rqst->COOKIEItem[i];
    //AmmServer_Info("POSTItem[%u].name = %s and we have %s \n",i,p->name,nameToLookFor);
    if (p->name!=0)
    {
     if (strncmp(p->name,nameToLookFor,sizeOfNameToLookFor) == 0)
     {
       //p->valueSize=sizeOfNameToLookFor;
       return p;
     }
    }
  }
 }
 return 0;
}


char * getPointerToCOOKIEItemValue(struct AmmServer_DynamicRequest * rqst,const char * nameToLookFor,unsigned int * pointerLength)
{
 const struct GETRequestContent * p = getCOOKIEItemFromName(rqst,nameToLookFor);

 if (p!=0)
 {
       //AmmServer_Success("getPointerToPOSTItemValue(%s) success => %p \n",nameToLookFor,p->value);
       *pointerLength = p->valueSize;
       return p->value;
 }

 AmmServer_Warning("getPointerToPOSTItemValue called but could not find name=`%s` \n",nameToLookFor);
 *pointerLength=0;
 return 0;
}



int getNumberOfCOOKIEItems(struct AmmServer_DynamicRequest * rqst)
{
 return rqst->COOKIEItemNumber;
}
