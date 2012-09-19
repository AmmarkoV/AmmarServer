#ifndef VAR_CACHING_H_INCLUDED
#define VAR_CACHING_H_INCLUDED

int InitializeVariableCache(unsigned int max_seperate_variables , unsigned int max_total_var_allocation_MB,unsigned int max_var_allocation_per_entry_MB);
int DestroyVariableCache();


int AddGETVariablesFromClient(unsigned int client_id,char * variables,unsigned int variables_length,unsigned int timestamp);
int AddPOSTVariablesFromClient(unsigned int client_id,char * variables,unsigned int variables_length,unsigned int timestamp);

int RemoveVariablesFromClient(unsigned int var_id);

#endif // VAR_CACHING_H_INCLUDED
