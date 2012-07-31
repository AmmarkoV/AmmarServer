#ifndef HTTPPROTOCOL_H_INCLUDED
#define HTTPPROTOCOL_H_INCLUDED


struct HTTPRequest
{
   int requestType;
   char resource[512];
   //Languages etc here..!
};


int HTTPRequestComplete(char * request,unsigned int request_length);

int AnalyzeHTTPLineRequest(struct HTTPRequest * output,char * request,unsigned int request_length,unsigned int lines_gathered);

#endif // HTTPPROTOCOL_H_INCLUDED
