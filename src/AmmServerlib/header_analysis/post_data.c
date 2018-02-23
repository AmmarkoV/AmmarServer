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

   //Get rid of New line..
   if (*ptr==13) { ++ptr; }
   if (*ptr==10) { ++ptr; }
   if (*ptr==13) { ++ptr; }
   if (*ptr==10) { ++ptr; }

   output->POSTItem[n].pointerStart=ptr;
   output->POSTItem[n].pointerEnd=ptr;
   output->POSTItem[n].contentSize=0;
   output->POSTItem[n].contentType=0;

   //------------------------------------------
   output->POSTItem[n].name=0;
   output->POSTItem[n].nameSize=0;

   output->POSTItem[n].value=0;
   output->POSTItem[n].valueSize=0;


   output->POSTItem[n].filename=0;
   output->POSTItem[n].filenameSize=0;

   output->POSTItem[n].contentDisposition=0;
   output->POSTItem[n].contentDispositionSize=0;

   output->POSTItem[n].contentType=0;
   output->POSTItem[n].contentTypeSize=0;


   output->POSTItem[n].populated=1;

  return 1;
}

char * reachNextBlock(char * request,unsigned int requestLength,unsigned int * endOfLine)
{
  char * ptrA=request;
  char * ptrB=request+1;
  char * ptrC=request+2;
  char * ptrD=request+3;

  char * ptrEnd = request + requestLength;

  //fprintf(stderr,"\nreachNextBlock for 13 10 13 10 on a buffer with %u bytes of data : ",requestLength);
   while (ptrD<ptrEnd)
    {
      if ( ( (*ptrA==13) && (*ptrB==10) && (*ptrC==13) && (*ptrD==10) ) || (*ptrA==0) )
        {
         ++ptrD;

         *ptrA=0; //Also make null terminated string..
         *endOfLine = ptrA-request;

         //fprintf(stderr,"done\n");
         return ptrD;
        }
      //fprintf(stderr,"%c(%u) ",*ptrA,*ptrA);
      ++ptrA;   ++ptrB;   ++ptrC;   ++ptrD;
    }

 //fprintf(stderr,"not found\n");
 return request;
}

char * reachNextLine(char * request,unsigned int requestLength,unsigned int * endOfLine)
{
  char * ptrA=request;

  char * ptrEnd = request + requestLength;

  //fprintf(stderr,"\nreachNextLine for 13 10 13 10 on a buffer with %u bytes of data : ",requestLength);
   while (ptrA<ptrEnd)
    {
      if ( (*ptrA==13) || (*ptrA==10) || (*ptrA==0)/*If we encounter a null terminator this is a violent end*/ )
        {
         *ptrA=0; //Also make null terminated string..
         *endOfLine = ptrA-request;

         ++ptrA;
         if ( (*ptrA==13) || (*ptrA==10) ) { ++ptrA; }
         if ( (*ptrA==13) || (*ptrA==10) ) { ++ptrA; }
         if ( (*ptrA==13) || (*ptrA==10) ) { ++ptrA; }

         //fprintf(stderr,"done\n");
         return ptrA;
        }
       //fprintf(stderr,"%c(%u) ",*ptrA,*ptrA);
      ++ptrA;
    }

 //fprintf(stderr,"not found\n");
 * endOfLine = requestLength;
 return request;
}


unsigned int countStringUntilQuotesOrNewLine(char * request,unsigned int requestLength)
{
  unsigned int endOfLine=0;
  char * ptrA=request;

  char * ptrEnd = request + requestLength;

  //fprintf(stderr,"\countStringUntilQuotesOrNewLine for 13 or 10 at %u bytes of data : ",requestLength);
   while (ptrA<ptrEnd)
    {
      if ( (*ptrA==13) || (*ptrA==10) || (*ptrA=='"') || (*ptrA==0)/*If we encounter a null terminator this is a violent end*/  )
        {
         *ptrA=0; //Also make null terminated string..
         endOfLine = (unsigned int) (ptrA-request);

         //fprintf(stderr,"done\n");
         return endOfLine;
        }

       //fprintf(stderr,"%c(%u) ",*ptrA,*ptrA);

      ++ptrA;
    }

 //fprintf(stderr,"not found\n");

 return endOfLine;
}




int finalizePOSTData(struct HTTPHeader * output)
{
 fprintf(stderr,"finalizePOSTData POSTItems : %p , %u items\n",output->POSTItem , output->POSTItemNumber);

 unsigned int success=0;
 unsigned int i=0;
 unsigned int PNum=output->POSTItemNumber;
 if (PNum>MAX_HTTP_POST_BOUNDARY_COUNT) { PNum=MAX_HTTP_POST_BOUNDARY_COUNT; }

 for (i=0; i<PNum; i++)
 {
  //fprintf(stderr,"output->POSTrequestSize=%u\n",output->POSTrequestSize);
  //fprintf(stderr,"output->POSTrequestBodySize=%u\n",output->POSTrequestBodySize);
  //AmmServer_Success("finalizePOSTData(%u)=`%s`\n",i,output->POSTItem[i].pointerStart);
  unsigned int length=0;
  char * configuration = output->POSTItem[i].pointerStart;
  char * payload = reachNextBlock(output->POSTItem[i].pointerStart,  output->POSTrequestBodySize,&length);
  reachNextLine(payload +1,  output->POSTrequestBodySize,&length);

  unsigned int configurationLength = payload-configuration;

  AmmServer_Warning("configuration(%u)=`%s`\n",i,configuration);
  AmmServer_Success("payload(%u)=`%s`\n",i,payload);

  char * filename = strstr(configuration,"filename=\"");
  if (filename!=0)
  {
    output->POSTItem[i].filename = filename+10; //skip filename="
    output->POSTItem[i].filenameSize = countStringUntilQuotesOrNewLine(output->POSTItem[i].filename,configurationLength);


    char * name = strstr(configuration,"name=\"");
    if (name!=0)
     {
       output->POSTItem[i].name = name+6; //skip name="
       output->POSTItem[i].nameSize = countStringUntilQuotesOrNewLine(output->POSTItem[i].name,configurationLength);
     }


       output->POSTItem[i].value = payload;
       output->POSTItem[i].valueSize=length;
  } else
  {
    char * name = strstr(configuration,"name=\"");
    if (name!=0)
     {
       output->POSTItem[i].name = name+6; //skip name="
       output->POSTItem[i].nameSize = countStringUntilQuotesOrNewLine(output->POSTItem[i].name,configurationLength);
     }

    if (payload!=0)
     {
       output->POSTItem[i].value = payload;
       output->POSTItem[i].valueSize=length;
     }


     fprintf(stderr,"%s=%p\n",output->POSTItem[i].name,output->POSTItem[i].value);

  }


 }

 return (success!=PNum);
}




/*
----------------------------------------------
              ACCESS POST DATA
----------------------------------------------
*/



char * getPointerToPOSTItem(struct AmmServer_DynamicRequest * rqst,const char * nameToLookFor,unsigned int * pointerLength)
{
 unsigned int sizeOfNameToLookFor = strlen(nameToLookFor);

 unsigned int i=0;
 unsigned int PNum=rqst->POSTItemNumber;
 if (PNum>MAX_HTTP_POST_BOUNDARY_COUNT) { PNum=MAX_HTTP_POST_BOUNDARY_COUNT; }

 //AmmServer_Success("getPointerToPOSTItem(%u)\n",PNum);
 for (i=0; i<PNum; i++)
 {
    struct POSTRequestBoundaryContent * p = &rqst->POSTItem[i];

    //AmmServer_Info("POSTItem[%u].name = %s and we have %s \n",i,p->name,nameToLookFor);
    if (p->name!=0)
    {
     if (strncmp (p->name,nameToLookFor,sizeOfNameToLookFor) == 0)
     {
       AmmServer_Success("getPointerToPOSTItem(%s) success => %p \n",nameToLookFor,p->value);
       *pointerLength = p->valueSize;

       return p->value;
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



char * getPointerToPOSTFile(struct AmmServer_DynamicRequest * rqst,const char * nameToLookFor,unsigned int * fileLength)
{
}









