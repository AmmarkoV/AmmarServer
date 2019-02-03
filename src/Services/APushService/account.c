#include "account.h"
#include <stdio.h>

#include "../../CSVParser/csvparse.h"
#include "../../InputParser/InputParser_C.h"

#define USE_STATIC_ACCOUNT_SETTINGS 1

int readAccountList(struct accountList * acc,const char * filename)
{
   acc->numberOfAccounts=0; // Initially null
   unsigned int accNumber=0;
   struct accountObject * account = &acc->account[accNumber];


 #if USE_STATIC_ACCOUNT_SETTINGS
   //-------------------------------------------------------------------
   ++accNumber;
   account->lastLogin=0;
   snprintf(account->accountUsername,32,"admin");
   snprintf(account->accountPassword,32,"admin");
   snprintf(account->email,512,"local");
   account->numberOfAssociatedDevices=1;
   snprintf(account->device[0].serialNumber,8,"0001");
   //-------------------------------------------------------------------
   acc->numberOfAccounts=accNumber;
   return 1;
#else
  //---------------------------------------------------------------------------------------------------
  //---------------------------------------------------------------------------------------------------
  //---------------------------------------------------------------------------------------------------
  unsigned int lineCounter=0;
  unsigned int i;

  struct CSVParser *  csvParser = csvParserCreate("|",1);
  if (csvParser!=0)
  {
    if (csvParser_StartParsingFile(csvParser,"db/pushService.accounts"))
    {
       unsigned int numberOfLines = csvParser_CountNumberOfLines(csvParser,"db/pushService.accounts");
       fprintf(stderr,"Number of Lines %u \n",numberOfLines);

       while (csvParser_ParseNextLine(csvParser))
       {
         if (lineCounter==0)
         {
           //Labels..
         } else
         if (i<MAXIMUM_NUMBER_OF_ACCOUNTS)
         {
          accNumber = lineCounter;
          account = &acc->account[accNumber];

          for (i=0; i<csvParser_GetNumberOfFields(csvParser); i++)
          {
           if (csvParser_GetField(csvParser,i))
           {
             switch (i)
             {
               case 0: snprintf(account->accountUsername,32,"%s",csvParser_GetField(csvParser,i)); break;
               case 1: snprintf(account->accountPassword,32,"%s",csvParser_GetField(csvParser,i)); break;
               case 2: snprintf(account->email,512,"%s",csvParser_GetField(csvParser,i));          break;
               case 3: fprintf(stderr,"NoSerial %u/%u : %s\n",lineCounter,i,csvParser_GetField(csvParser,i));   break;
             }
             fprintf(stderr,"!!LINE %u/FIELD %u : %s\n",lineCounter,i,csvParser_GetField(csvParser,i));
           }
          }

         }

         ++lineCounter;
       }
      csvParser_StopParsingFile(csvParser);
      fprintf(stderr,"Successfully finished reading accounts ..\n");
      acc->numberOfAccounts=accNumber;
      csvParserDestroy(csvParser);
      return 1;
    } else
    { fprintf(stderr,"Could not open accounts csv file..\n"); }
    csvParserDestroy(csvParser);
  } else
  { fprintf(stderr,"Could not allocate accounts csvParser object..\n"); }
  //---------------------------------------------------------------------------------------------------
  //---------------------------------------------------------------------------------------------------
  //---------------------------------------------------------------------------------------------------
#endif

 return 0;
}
