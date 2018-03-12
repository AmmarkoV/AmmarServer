/** @file post_header_analysis.h
* @brief Tools to process POST requests
* @author Ammar Qammaz (AmmarkoV)
* @bug POST header analysis is not fully implemented yet
*/

#ifndef POSTHEADERS_H_INCLUDED
#define POSTHEADERS_H_INCLUDED

#include "../AmmServerlib.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief Analyze a POST request line by line filling in the structures that define it
* @ingroup POSTanalysis
* @param An AmmarServer Instance
* @param Output HTTPHeader with information
* @param Memory block with the incoming request
* @param Length of Memory block of the incoming request
* @param Current line we are at
* @param Filename of web server root directory ( public_html )
* @retval 1=Success,0=Failure */
int AnalyzePOSTLineRequest(
                            struct AmmServer_Instance * instance,
                            struct HTTPHeader * output,
                            char * request,
                            unsigned int request_length,
                            unsigned int lines_gathered
                          );




#ifdef __cplusplus
}
#endif

#endif // POSTHEADERS_H_INCLUDED
