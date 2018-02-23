#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "framebuffer.h"




int storeImage(struct imageStorage * is , int id , char * data,unsigned int dataSize)
{
 return 1;
 //pthread_mutex_init(&prespawned_data->operation_mutex,0);
 //pthread_cond_init(&prespawned_data->condition_var,0);
 //pthread_mutex_lock(&prespawned_data->operation_mutex);

 if (is[id].data!=0)
  {
    free(is[id].data);
    is[id].data=0;
  }

  is[id].data = (char*) malloc(sizeof(char) * (dataSize+1));
  if (is[id].data!=0)
  {
    memcpy(is[id].data,data,dataSize);
    is[id].dataSize=dataSize;
  }

  return  ( (is[id].data!=0) && (is[id].dataSize!=0) );
}
