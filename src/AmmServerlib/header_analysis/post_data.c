#define _GNU_SOURCE         /* for memmem*/


#include "post_data.h"
#include "generic_header_tools.h"

#include "../tools/http_tools.h"

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

   output->POSTItem[n].reallocateOnHeaderRAWResize=1;
  return 1;
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
  //Tracks whether THIS item parsed cleanly - cleared on any of the "we had to fall back / this part looks
  //malformed" paths below , so the function's return value ( success==PNum ) actually reflects whether every
  //item parsed as expected , instead of unconditionally reporting success whenever PNum>0 regardless of what
  //happened in the loop.
  unsigned int itemOk=1;
  unsigned int length=0;
  char * configuration = output->POSTItem[i].pointerStart;
  char * payload = reachNextBlock(
                                   output->POSTItem[i].pointerStart,
                                   _calculateRemainingDataLength(
                                                                  output->headerRAW ,
                                                                  output->headerRAWSize ,
                                                                  output->POSTItem[i].pointerStart
                                                                )  ,
                                   &length
                                   ,1 //Null termination
                                  );
  reachNextLine( payload+1,
                 _calculateRemainingDataLength(
                                               output->headerRAW ,
                                               output->headerRAWSize ,
                                               payload+1
                                              ),
                &length,
                0//THIS NEEDS TO BE SET TO 0 OTHERWISE THIS BLEEDS IN THE DATA AND ADDS A NULL
               );

  //When reachNextBlock() can't find the "\r\n\r\n" that ends this part's own header block within bounds ( a
  //malformed/truncated multipart body ) it returns pointerStart unchanged , so payload==configuration and
  //configurationLength is correctly 0 here - the memmem() calls below then safely find nothing rather than
  //scanning unbounded into whatever data happens to follow in the shared header buffer ( which is what the
  //old strstr()-based version of this code did ).
  unsigned int configurationLength = payload-configuration;

  //AmmServer_Warning("configuration(%u)=`%s`\n",i,configuration);
  //AmmServer_Success("payload(%u)=`%s`\n",i,payload);

  char * filename = (char*) memmem(configuration,configurationLength,"filename=\"",10);
  if (filename!=0)
  {
    output->POSTItem[i].filename = filename+10; //skip filename="
    //Bound the scan by what's actually left of the configuration block from filename's own position onward ,
    //not the whole block's length measured from its start - passing the full configurationLength here used to
    //let a filename value with no closing quote scan straight past this field's true end and into the
    //payload/file data that follows it, NUL-terminating ( "inserting garbage" into ) partway through someone
    //else's file content and reporting a filenameSize that included part of it.
    unsigned int filenameRemaining = configurationLength - (unsigned int)(output->POSTItem[i].filename-configuration);
    output->POSTItem[i].filenameSize = countStringUntilQuotesOrNewLine(
                                                                        output->POSTItem[i].filename,
                                                                        filenameRemaining ,
                                                                        1 //Null termination
                                                                       );


    char * name = (char*) memmem(configuration,configurationLength,"name=\"",6);
    if (name!=0)
     {
       output->POSTItem[i].name = name+6; //skip name="
       unsigned int nameRemaining = configurationLength - (unsigned int)(output->POSTItem[i].name-configuration);
       output->POSTItem[i].nameSize = countStringUntilQuotesOrNewLine(
                                                                       output->POSTItem[i].name,
                                                                       nameRemaining,
                                                                       1 // Null termination
                                                                       );
     } else
     {
       AmmServer_Warning("File upload boundary part %u/%u has no name=\"...\" field, marking it incomplete..\n",i,PNum);
       itemOk=0;
     }

       output->POSTItem[i].value = payload;

       unsigned int payloadSize = _calculateRemainingDataLength
                                              (
                                               output->headerRAW ,
                                               output->headerRAWSize ,
                                               payload
                                              );

/*
       fprintf(stderr,"Payload (%p) size %u / Boundary(%p) size %u \n",
                                     payload,
                                     payloadSize ,
                                     output->boundary ,
                                     output->boundaryLength);*/

       char * payloadEnd  = (char*) memmem(
                                           payload,
                                           payloadSize ,
                                           output->boundary ,
                                           output->boundaryLength
                                          );

       if (payloadEnd!=0)
       {
        //output->boundary does not include the mandatory "--" that always precedes a boundary occurrence on
        //the wire , and the payload also carries a trailing CRLF right before that "--" which is framing , not
        //data , so both have to be stripped off to get the real file size..!
        unsigned int rawSize = payloadEnd-payload;
        if (rawSize>=2) { rawSize-=2; }
        while ( (rawSize>0) && ( (payload[rawSize-1]==13) || (payload[rawSize-1]==10) ) ) { --rawSize; }
        output->POSTItem[i].valueSize=rawSize;
        //AmmServer_Success("Found boundary in file payload, size of payload is %u ..!",output->POSTItem[i].valueSize);
       } else
       {
        //Boundary never showed up before the buffer ran out ( truncated upload ) - fall back to "everything
        //that's left of the buffer from here" ( payloadSize , already computed above ) rather than the size
        //of the wrong region : this used to measure from output->boundary ( a pointer back in this part's own
        //*header*, nowhere near the file data ) to the end of the buffer , which is a meaningless length for
        //a file payload that actually starts at payload , not at output->boundary.
        AmmServer_Error("Could not detect boundary in file payload, using remaining buffer length as a fallback..!");
        output->POSTItem[i].valueSize = payloadSize;
        itemOk=0;
       }
  } else
  {
    char * name = (char*) memmem(configuration,configurationLength,"name=\"",6);
    if (name!=0)
     {
       output->POSTItem[i].name = name+6; //skip name="
       unsigned int nameRemaining = configurationLength - (unsigned int)(output->POSTItem[i].name-configuration);
       output->POSTItem[i].nameSize = countStringUntilQuotesOrNewLine(
                                                                        output->POSTItem[i].name,
                                                                        nameRemaining,
                                                                        1 // Null termination
                                                                     );
     }

    if (payload!=0)
     {
       output->POSTItem[i].value = payload;

       //A plain ( non file ) part's value is not delimited by a single line , it can be empty , span multiple
       //lines or contain arbitrary bytes , so just like the file case above we have to search for the next
       //boundary occurrence to know where it actually ends , otherwise valueSize ends up wrong and , since
       //callers like _GENERIC_cpy treat this pointer as a NUL terminated C string , the value bleeds into
       //whatever comes after it in the raw request buffer..!
       unsigned int payloadSize = _calculateRemainingDataLength(
                                                                  output->headerRAW ,
                                                                  output->headerRAWSize ,
                                                                  payload
                                                                 );

       char * payloadEnd = (char*) memmem(
                                           payload,
                                           payloadSize ,
                                           output->boundary ,
                                           output->boundaryLength
                                          );

       if (payloadEnd!=0)
       {
        //output->boundary does not include the mandatory "--" that always precedes a boundary occurrence on
        //the wire , and the payload also carries a trailing CRLF right before that "--" which is framing , not
        //data , so both have to be stripped off . Text fields are safe to also NUL terminate , unlike file data..!
        unsigned int rawSize = payloadEnd-payload;
        if (rawSize>=2) { rawSize-=2; }
        while ( (rawSize>0) && ( (payload[rawSize-1]==13) || (payload[rawSize-1]==10) ) ) { --rawSize; }
        output->POSTItem[i].valueSize=rawSize;
        payload[rawSize]=0;
       } else
       {
        output->POSTItem[i].valueSize=length;
       }
     }

    if (name!=0)
     {
      //fprintf(stderr,"%s=%p\n",output->POSTItem[i].name,output->POSTItem[i].value);
     } else
     {
       AmmServer_Warning("Incorrect name for boundary part %u/%u marking it as empty..\n",i,PNum);
       output->POSTItem[i].value = 0;
       output->POSTItem[i].valueSize=0;
       itemOk=0;
     }
  }

  if (itemOk) { ++success; }
 }

 //True only if every boundary part parsed cleanly ( no missing name= , no "could not detect boundary" fallback
 //) - PNum==0 ( a POST with no multipart parts at all , e.g. an empty body ) also reports success here since
 //there was nothing to fail at parsing, not a real error.
 return (success==PNum);
}




/*
----------------------------------------------
              ACCESS POST DATA
----------------------------------------------
*/
const struct POSTRequestBoundaryContent * getPOSTItemFromName(struct AmmServer_DynamicRequest * rqst,const char * nameToLookFor)
{
 unsigned int i=0;
 unsigned int PNum=rqst->POSTItemNumber;
 if (PNum>MAX_HTTP_POST_BOUNDARY_COUNT) { PNum=MAX_HTTP_POST_BOUNDARY_COUNT; }

 if (rqst->POSTItem!=0)
 {
  for (i=0; i<PNum; i++)
  {
    struct POSTRequestBoundaryContent * p = &rqst->POSTItem[i];
    //AmmServer_Info("POSTItem[%u].name = %s and we have %s \n",i,p->name,nameToLookFor);
    if (p->name!=0)
    {
     //This used to be a strncmp bounded by nameToLookFor's length , which means looking up "captcha" would
     //also match a field actually named "captchaID" ( or any field whose name merely starts with "captcha" ) ,
     //silently returning the wrong POST item. Field names are NUL terminated ( see finalizePOSTData() ) so a
     //plain exact strcmp is both correct and safe here.
     if (strcmp(p->name,nameToLookFor) == 0)
     {
       return p;
     }
    }
  }
 }
 return 0;
}


char * getPointerToPOSTItemValue(struct AmmServer_DynamicRequest * rqst,const char * nameToLookFor,unsigned int * pointerLength)
{
 const struct POSTRequestBoundaryContent * p = getPOSTItemFromName(rqst,nameToLookFor);

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


char * getPointerToPOSTItemFilename(struct AmmServer_DynamicRequest * rqst,const char * nameToLookFor,unsigned int * pointerLength)
{
 const struct POSTRequestBoundaryContent * p = getPOSTItemFromName(rqst,nameToLookFor);

 if (p!=0)
 {
       *pointerLength = p->filenameSize;
       return p->filename;
 }

 *pointerLength=0;
 return 0;
}



char * getPointerToPOSTItemType(struct AmmServer_DynamicRequest * rqst,const char * nameToLookFor,unsigned int * pointerLength)
{
 const struct POSTRequestBoundaryContent * p = getPOSTItemFromName(rqst,nameToLookFor);

 if (p!=0)
 {
       *pointerLength = p->contentTypeSize;
       return p->contentType;
 }

 *pointerLength=0;
 return 0;
}


int getNumberOfPOSTItems(struct AmmServer_DynamicRequest * rqst)
{
 return rqst->POSTItemNumber;
}

