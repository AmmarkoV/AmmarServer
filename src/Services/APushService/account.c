#include "account.h"
#include <stdio.h>

#include "../../CSVParser/csvparse.h"
#include "../../InputParser/InputParser_C.h"

int readAccountList(struct accountList * acc,const char * filename)
{
   unsigned int accNumber=0;
   //-------------------------------------------------------------------
   struct accountObject * account = &acc->account[accNumber];
   ++accNumber;
   account->lastLogin=0;
   snprintf(account->accountUsername,32,"admin");
   snprintf(account->accountPassword,32,"admin");
   snprintf(account->email,512,"local");
   account->numberOfAssociatedDevices=1;
   snprintf(account->device[0].serialNumber,8,"0001");
   //-------------------------------------------------------------------

   acc->numberOfAccounts=accNumber;




  //---------------------------------------------------------------------------------------------------
  unsigned int lineCounter=0;
  struct CSVParser *  csvParser = csvParserCreate(";",1);
  if (csvParser!=0)
  {
    if (csvParser_StartParsingFile(csvParser,"db/pushService.accounts"))
    {
       unsigned int numberOfLines = csvParser_CountNumberOfLines(csvParser,"db/pushService.accounts");
       fprintf(stderr,"Number of Lines %u \n",numberOfLines);

       while (csvParser_ParseNextLine(csvParser))
       {


         unsigned int i;
         for (i=0; i<csvParser_GetNumberOfFields(csvParser); i++)
         {
           if (csvParser_GetField(csvParser,i))
           {
             fprintf(stderr,"!!LINE %u/FIELD %u : %s\n",lineCounter,i,csvParser_GetField(csvParser,i));
           } else
           { fprintf(stderr,"Unable to get field %u for line %u\n",i,lineCounter); }
         }
         fprintf(stderr,"\n");
         ++lineCounter;
       }
      csvParser_StopParsingFile(csvParser);
    } else
    { fprintf(stderr,"Could not open accounts csv file..\n"); }
   csvParserDestroy(csvParser);
  } else
  { fprintf(stderr,"Could not allocate accounts csvParser object..\n"); }
  //---------------------------------------------------------------------------------------------------

 fprintf(stderr,"Finished reading accounts ..\n");


  return 1;
}
