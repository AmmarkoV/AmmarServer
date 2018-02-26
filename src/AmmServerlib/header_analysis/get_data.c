#include "get_data.h"
#include "generic_header_tools.h"

#include "../tools/http_tools.h"

#include <stdio.h>
#include <string.h>


int wipeGETData(struct HTTPHeader * output)
{
  output->GETItemNumber=0;
  memset(output->GETItem,0,MAX_HTTP_GET_VARIABLE_COUNT*sizeof(struct GETRequestContent));
  return 1;
}


int createGETData(struct HTTPHeader * output)
{
  return (wipeGETData(output));
}


int finalizeGETData(struct HTTPHeader * output)
{
  createGETData(output);

  output->GETItemNumber=0;
  char * GETPtr = output->GETRequest;

  if (GETPtr==0)
  {
   return 0;
  }

  unsigned int GETPtrLength = strnlen(GETPtr,output->headerRAWSize);
  char * GETPtrEnd = GETPtr + GETPtrLength;

  //AmmServer_Warning("GET Request %s has %u bytes of stuff..\n",GETPtr ,GETPtrLength);
  if (GETPtrLength==0) { return 0; }

  char * startOfPTR=GETPtr;

  #define FOUND_NOTHING  0
  #define SEEKING_NAME   1
  #define SEEKING_VALUE  2

  unsigned int state = FOUND_NOTHING;
  while (GETPtr<GETPtrEnd)
  {
    if ( (*GETPtr==10) || (*GETPtr==13) || (*GETPtr==0) )
    {
      if (state==FOUND_NOTHING)
        {
          //We found no payload..
          break;
        } else
      if (state==SEEKING_NAME)
        {
          //Last Item was a name with no value
          output->GETItem[output->GETItemNumber].name=startOfPTR;
          output->GETItem[output->GETItemNumber].value=0;
          *GETPtr=0; //Null Terminate
          ++output->GETItemNumber;
          break;
        } else
      if (state==SEEKING_VALUE)
        {
          //We reached the end having found a name(which is already set) and a value..!
          output->GETItem[output->GETItemNumber].value=startOfPTR;
          *GETPtr=0; //Null Terminate

          //fprintf(stderr,"We finished string searching for a value so it is found value %s\n",startOfPTR);
          ++output->GETItemNumber;
          break;
        }
    } else
    {
      //fprintf(stderr,"GET -> %c \n",*GETPtr);
      if (state == FOUND_NOTHING) { state=SEEKING_NAME; }

      if (*GETPtr=='=')
      {

       //We found a value
       if (state==SEEKING_NAME)
        {
          output->GETItem[output->GETItemNumber].name=startOfPTR;
          output->GETItem[output->GETItemNumber].value=0;
          *GETPtr=0;
          //fprintf(stderr,"We finished a name %s\n",startOfPTR);

          state=SEEKING_VALUE;
          startOfPTR=GETPtr+1;
        }
      } else
      if (*GETPtr=='&')
      {
       //We found a value
       if (state==SEEKING_NAME)
        {
          output->GETItem[output->GETItemNumber].name=startOfPTR;
          output->GETItem[output->GETItemNumber].value=0;
          ++output->GETItemNumber;

          *GETPtr=0; //Null Terminate
          startOfPTR=GETPtr+1;
        } else
       if (state==SEEKING_VALUE)
        {
          output->GETItem[output->GETItemNumber].value=startOfPTR;
          ++output->GETItemNumber;

          *GETPtr=0; //Null Terminate
          state=SEEKING_NAME;
          startOfPTR=GETPtr+1;
        }
      }

    }


    ++GETPtr;
  }

  //Final ( or only value )
  if (state==SEEKING_VALUE)
        {
          //We reached the end having found a name(which is already set) and a value..!
          output->GETItem[output->GETItemNumber].value=startOfPTR;
          *GETPtr=0; //Null Terminate

          //fprintf(stderr,"We finished string searching for a value so it is found value %s\n",startOfPTR);
          ++output->GETItemNumber;
        }

  //Mark that all of the get items here point on RAW and need update on realloc
  unsigned int i=0;
  for (i=0; i<output->GETItemNumber; i++)
  {
     output->GETItem[i].reallocateOnHeaderRAWResize=1;
     //TODO strip HTML characters here..
     //StripHTMLCharacters_Inplace(output->GETquery,0 /* 0 = Disregard dangerous bytes , Safety OFF*/); // <- This call converts char sequences like %20 to " " and %00 to \0 disregarding any form of safety , ( since it is a raw var )
  }


/*
  AmmServer_Success("A total of %u GET Items \n",output->GETItemNumber);
  for (i=0; i<output->GETItemNumber; i++)
  {
     fprintf(stderr,"GET Item %u ------------------ \n",i);
     fprintf(stderr,"name = %s \n",output->GETItem[i].name);
     fprintf(stderr,"value = %s \n",output->GETItem[i].value);
     fprintf(stderr,"-------------------------------- \n",i);
  }
*/

 return 1;
}





/*
----------------------------------------------
              ACCESS POST DATA
----------------------------------------------
*/
const struct GETRequestContent * getGETItemFromName(struct AmmServer_DynamicRequest * rqst,const char * nameToLookFor)
{
 unsigned int sizeOfNameToLookFor = strlen(nameToLookFor);

 unsigned int i=0;
 unsigned int PNum=rqst->GETItemNumber;
 if (PNum>MAX_HTTP_GET_VARIABLE_COUNT) { PNum=MAX_HTTP_GET_VARIABLE_COUNT; }

 if (rqst->GETItem!=0)
 {
  for (i=0; i<PNum; i++)
  {
    struct GETRequestContent * p = &rqst->GETItem[i];
    //AmmServer_Info("POSTItem[%u].name = %s and we have %s \n",i,p->name,nameToLookFor);
    if (p->name!=0)
    {
     if (strncmp(p->name,nameToLookFor,sizeOfNameToLookFor) == 0)
     {
       return p;
     }
    }
  }
 }
 return 0;
}


char * getPointerToGETItemValue(struct AmmServer_DynamicRequest * rqst,const char * nameToLookFor,unsigned int * pointerLength)
{
 const struct GETRequestContent * p = getGETItemFromName(rqst,nameToLookFor);

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



int getNumberOfGETItems(struct AmmServer_DynamicRequest * rqst)
{
 return rqst->GETItemNumber;
}

