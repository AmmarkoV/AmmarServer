#include <stdio.h>
#include <string.h>
#include "postHeader.h"

int scanFor_postHeader(char * str)
{
 switch (str[0]) {
 case 'C' :
     switch (str[1]) {
     case 'O' :
         switch (str[2]) {
         case 'N' :
             if ( strcmp(str,"CONTENT-TYPE:") == 0 ) { return POSTHEADER_CONTENT_TYPE; }
             else  if ( strcmp(str,"CONTENT-DISPOSITION:") == 0 ) { return POSTHEADER_CONTENT_DISPOSITION; }
             else  if ( strcmp(str,"CONTENT-LENGTH:") == 0 ) { return POSTHEADER_CONTENT_LENGTH; }
         break;
        };
     break;
    };
 break;
};
 return 0;
}

