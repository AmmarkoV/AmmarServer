#include "post_data.h"

#include <stdio.h>
#include <string.h>

int wipePOSTData(struct HTTPHeader * output)
{
  output->POSTItemNumber=0;
  memset(output->POSTItem,0,MAX_HTTP_POST_BOUNDARY_COUNT*sizeof(struct POSTRequestBoundaryContent));
  return 1;
}

int createPOSTData(struct HTTPHeader * output)
{
  return (wipePOSTData(output));
}

int addPOSTDataBoundary(struct HTTPHeader * output,char * ptr)
{
  unsigned int n=0;
  if (output->POSTItemNumber>=MAX_HTTP_POST_BOUNDARY_COUNT)
  {
   AmmServer_Warning("Surpassed the maximum number of POST boundaries acceptable ( MAX_HTTP_POST_BOUNDARY_COUNT = %u ), this is a compile-time setting \n",MAX_HTTP_POST_BOUNDARY_COUNT);
   return 0;
  } else
  {
    n=output->POSTItemNumber;
    ++output->POSTItemNumber;
  }

   output->POSTItem[n].pointerStart=ptr;
   output->POSTItem[n].pointerEnd=ptr;
   output->POSTItem[n].contentSize=0;
   output->POSTItem[n].contentType=0;

   //------------------------------------------
   output->POSTItem[n].name=0;
   output->POSTItem[n].nameSize=0;

   output->POSTItem[n].filename=0;
   output->POSTItem[n].filenameSize=0;

   output->POSTItem[n].contentDisposition=0;
   output->POSTItem[n].contentDispositionSize=0;

   output->POSTItem[n].contentType=0;
   output->POSTItem[n].contentTypeSize=0;


   output->POSTItem[n].populated=1;

  return 1;
}


char * reachNextLine(char * request,unsigned int requestLength)
{
  char * ptrA=request;
  char * ptrB=request+1;
  char * ptrC=request+2;
  char * ptrD=request+3;

  char * ptrEnd = request + requestLength;

   while (ptrD<ptrEnd)
    {
      if ( (*ptrA==13) && (*ptrB==10) && (*ptrC==13) && (*ptrD==10) )
        {
          ++ptrD;
         return ptrD;
        }

      ++ptrA;   ++ptrB;   ++ptrC;   ++ptrD;
    }


}





int finalizePOSTData(struct HTTPHeader * output)
{
 return 1;

 unsigned int success=0;
 unsigned int i=0;
 unsigned int PNum=output->POSTItemNumber;
 if (PNum>MAX_HTTP_POST_BOUNDARY_COUNT) { PNum=MAX_HTTP_POST_BOUNDARY_COUNT; }

 for (i=0; i<PNum; i++)
 {
  AmmServer_Success("finalizePOSTData(%u)=%s\n",i,output->POSTItem[i].pointerStart);
  char * configuration = reachNextLine(output->POSTItem[i].pointerStart ,  output->POSTrequestSize);

  AmmServer_Warning("configuration(%u)=%s\n",i,configuration);


 }

 return (success!=PNum);
}




/*
----------------------------------------------
              ACESS POST DATA
----------------------------------------------
*/
char * getPointerToPOSTItem(struct AmmServer_DynamicRequest * rqst,char * nameToLookFor,unsigned int * pointerLength)
{
 unsigned int sizeOfNameToLookFor = strlen(nameToLookFor);

 unsigned int i=0;
 unsigned int PNum=rqst->POSTItemNumber;
 if (PNum>MAX_HTTP_POST_BOUNDARY_COUNT) { PNum=MAX_HTTP_POST_BOUNDARY_COUNT; }

 AmmServer_Success("getPointerToPOSTItem(%u)\n",PNum);
 for (i=0; i<PNum; i++)
 {
    struct POSTRequestBoundaryContent * p = &rqst->POSTItem[i];
    if (p->name!=0)
    {
     if (strncmp (p->name,nameToLookFor,sizeOfNameToLookFor) == 0)
     {
      if (p->pointerEnd > p->pointerStart)
       {
        AmmServer_Success("getPointerToPOSTItem success \n");
        *pointerLength = p->pointerEnd - p->pointerStart;
        return p->pointerStart;
       }
     }
    }
 }

 AmmServer_Warning("getPointerToPOSTItem called but could not find name=`%s` \n",nameToLookFor);
 *pointerLength=0;
 return 0;
}

int getNumberOfPOSTItems(struct AmmServer_DynamicRequest * rqst)
{
 return rqst->POSTItemNumber;
}
