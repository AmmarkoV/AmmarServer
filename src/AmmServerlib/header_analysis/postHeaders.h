#ifndef POSTHEADERS_H_INCLUDED
#define POSTHEADERS_H_INCLUDED


#include "../AmmServerlib.h"

int AnalyzePOSTLineRequest(
                            struct AmmServer_Instance * instance,
                            struct HTTPRequest * output,
                            char * request,
                            unsigned int request_length,
                            unsigned int lines_gathered,
                            char * webserver_root
                          );

#endif // POSTHEADERS_H_INCLUDED
