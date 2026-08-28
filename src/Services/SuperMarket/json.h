#ifndef JSON_H_INCLUDED
#define JSON_H_INCLUDED

#include "state.h"

//Append a JSON-escaped ( quotes/backslashes/control chars ) copy of str to out , growing at strlen(out) , never overflowing outSize
void jsonAppendEscaped(char * out,unsigned int outSize,const char * str);

//Build the full JSON representation of a cart , exactly the shape supermarket.py / the web UI expect :
//{"items":[{"id":"..","n":"..","q":N,"c":0|1,"img":MTIME}],"rev":N,"name":"..","cimg":MTIME}
//"name" is only emitted when the list has a custom title ( matches go.php unsetting the key for the default title )
void buildCartJSON(const char * token,struct cart * cart,char * out,unsigned int outSize);

#endif // JSON_H_INCLUDED
