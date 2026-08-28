#include "cookie_data.h"
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

/**
* @brief Cookie headers look like "name1=value1; name2=value2" , i.e. GET-query-string syntax
*        but with "; " between pairs instead of "&" , so finalizeGenericGETField() (which splits
*        on '&' and '=') can't be reused directly. This is otherwise the same left-to-right scan as
*        finalizeGenericGETField() , with ';' as the pair separator and leading spaces skipped.
*
*        Unlike the GET/POST query strings ( which finalizeGETData/finalizePOSTData strnlen() out of
*        an already fully isolated , private copy before ever touching it ) , "value" here still points
*        straight into the shared headerRAW buffer WHILE the outer per-line header tokenizer is still
*        walking forward through it — cookieLength stops right before this line's trailing CRLF , so the
*        scan below always runs off the counted length for the last pair on the line without finding an
*        in-bounds separator. Writing a NUL past that boundary ( onto the CRLF itself ) corrupts the byte
*        the outer tokenizer still needs to find where this line ends , breaking the rest of the request
*        ( confirmed : it turns the whole request into a 400 ). So this function never writes past
*        `value+valueLength` ; name/value lengths are always computed from pointer arithmetic instead of
*        strlen(), and the last field on a line is therefore NOT NUL-terminated in place — _COOKIEcpy()
*        and _COOKIEcmp() are written length-bounded accordingly , never assuming a trailing NUL.
*/
static int finalizeGenericCookieField(
                                       struct GETRequestContent * target ,
                                       unsigned int * targetNumber ,
                                       char * value,
                                       unsigned int valueLength
                                      )
{
  *targetNumber=0;
  char * ptr = value;
  char * end = value + valueLength;

  while (ptr<end)
  {
    while ( (ptr<end) && ( (*ptr==' ') || (*ptr==';') ) ) { ++ptr; }
    if ( (ptr>=end) || (*ptr==10) || (*ptr==13) || (*ptr==0) ) { break; }
    if (*targetNumber>=MAX_HTTP_GET_VARIABLE_COUNT) { break; }

    char * nameStart = ptr;
    while ( (ptr<end) && (*ptr!='=') && (*ptr!=';') && (*ptr!=10) && (*ptr!=13) && (*ptr!=0) ) { ++ptr; }

    if ( (ptr>=end) || (*ptr!='=') )
    {
      //A bare name with no value ( rare , but treat it like finalizeGenericGETField does )
      target[*targetNumber].name=nameStart;
      target[*targetNumber].nameSize=(unsigned int)(ptr-nameStart);
      target[*targetNumber].value=0;
      target[*targetNumber].valueSize=0;
      int hitLineTerminator = ( (ptr>=end) || (*ptr==10) || (*ptr==13) || (*ptr==0) );
      if (ptr<end) { *ptr=0; } //only safe to write while still strictly inside the counted region
      *targetNumber+=1;
      if (hitLineTerminator) { break; }
      ++ptr; //skip the ';'
      continue;
    }

    target[*targetNumber].name=nameStart;
    target[*targetNumber].nameSize=(unsigned int)(ptr-nameStart);
    *ptr=0; //the '=' is always strictly inside the counted region , safe to NUL
    ++ptr;

    char * valueStart = ptr;
    while ( (ptr<end) && (*ptr!=';') && (*ptr!=10) && (*ptr!=13) && (*ptr!=0) ) { ++ptr; }
    target[*targetNumber].value=valueStart;
    target[*targetNumber].valueSize=(unsigned int)(ptr-valueStart);
    int hitLineTerminator = ( (ptr>=end) || (*ptr==10) || (*ptr==13) || (*ptr==0) );
    if (ptr<end) { *ptr=0; } //ditto : only when still strictly inside the counted region
    *targetNumber+=1;

    if (hitLineTerminator) { break; }
    ++ptr; //skip the ';'
  }

  unsigned int i=0;
  for (i=0; i<*targetNumber; i++) { target[i].reallocateOnHeaderRAWResize=1; }

  return 1;
}

int finalizeCOOKIEData(struct HTTPHeader * output,char * value,unsigned int valueLength)
{
  createCOOKIEData(output);

  if ( (value==0) || (valueLength==0) ) { return 0; }

  return finalizeGenericCookieField(
                                     output->COOKIEItem ,
                                     &output->COOKIEItemNumber ,
                                     value,
                                     valueLength
                                    );
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
