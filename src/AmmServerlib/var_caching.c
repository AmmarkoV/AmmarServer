#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "var_caching.h"


unsigned int VAR_CACHING_ENABLED=0;
unsigned int MAX_VAR_TOTAL_ALLOCATION_IN_MB=10;
unsigned int MAX_VAR_CACHE_SIZE=100;

struct submitted_variables
{
   char * GETvar;
   unsigned int GETvar_length;
   char * POSTvar;
   unsigned int POSTvar_length;
   unsigned char erase;
};

struct submitted_variables * var_cache=0;
unsigned int loaded_var_cache_items=0;

int InitializeVariableCache(unsigned int max_seperate_variables , unsigned int max_total_var_allocation_MB,unsigned int max_var_allocation_per_entry_MB)
{
  return 1;

  MAX_VAR_TOTAL_ALLOCATION_IN_MB=max_total_var_allocation_MB;
  MAX_VAR_CACHE_SIZE=max_seperate_variables;
  //TODO : Take into account max_var_allocation_per_entry_MB
  if (var_cache==0)
   {
     var_cache = (struct submitted_variables *) malloc(sizeof(struct submitted_variables) * (MAX_VAR_CACHE_SIZE+1));
     if (var_cache == 0) { fprintf(stderr,"Unable to allocate initial cache for POST/GET requests \n"); return 0; }
   }


   //TODO: Change implementation here
   //unsigned int i=0;
   //for (i=0; i<MAX_VAR_CACHE_SIZE; i++) { cache[i].mem=0; cache[i].filesize=0; cache[i].prepare_mem_callback=0; }
   return 1;
}

int DestroyVariableCache()
{
    //free var_cache etc..!
   return 1;
}


int AddVariablesFromClient(unsigned int client_id,char * variables,unsigned int variables_length,unsigned int timestamp,unsigned int GETorPOST)
{
  fprintf(stderr,"AddVariablesFromClient(%u,%s,%u,%u,%u) not implemented\n",client_id,variables,variables_length,timestamp,GETorPOST);
  /* TODO , implementation here..*/
  return 0;
}


int RemoveVariablesFromClient(unsigned int var_id)
{
  fprintf(stderr,"RemoveVariablesFromClient(%u) not implemented\n",var_id);
  /* TODO , implementation here..*/
  return 0;
}


int AddGETVariablesFromClient(unsigned int client_id,char * variables,unsigned int variables_length,unsigned int timestamp)
{
  return AddVariablesFromClient(client_id,variables,variables_length,timestamp,1);
}

int AddPOSTVariablesFromClient(unsigned int client_id,char * variables,unsigned int variables_length,unsigned int timestamp)
{
  return AddVariablesFromClient(client_id,variables,variables_length,timestamp,2);
}
