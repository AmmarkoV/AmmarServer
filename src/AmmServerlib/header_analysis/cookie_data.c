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
  createCOOKIEData(output);
  return finalizeGenericGETField(
                                 output,
                                 output->COOKIEItem ,
                                 &output->COOKIEItemNumber ,
                                 value,
                                 valueLength
                                );

 return 1;
}


