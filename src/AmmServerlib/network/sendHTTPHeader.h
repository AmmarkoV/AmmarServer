#ifndef SENDHTTPHEADER_H_INCLUDED
#define SENDHTTPHEADER_H_INCLUDED

unsigned long SendErrorCodeHeader(int clientsock,unsigned int error_code,char * verified_filename,char * templates_root);
unsigned long SendSuccessCodeHeader(int clientsock,int success_code,char * verified_filename);
unsigned long SendNotModifiedHeader(int clientsock);
unsigned long SendAuthorizationHeader(int clientsock,char * message,char * verified_filename);

#endif // SENDHTTPHEADER_H_INCLUDED
