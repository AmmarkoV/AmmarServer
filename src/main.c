/*
AmmarServer , main executable

URLs: http://ammar.gr
Written by Ammar Qammaz a.k.a. AmmarkoV 2012

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "AmmServerlib/AmmServerlib.h"

#define MAX_BINDING_PORT 65534
#define DEFAULT_BINDING_PORT 1234
#define MAX_INPUT_IP 256

int main(int argc, char *argv[])
{
    printf("Ammar Server starting up\n");

    char bindIP[MAX_INPUT_IP];
    strcpy(bindIP,"0.0.0.0");

    unsigned int port=DEFAULT_BINDING_PORT;

    if ( argc <1 ) { fprintf(stderr,"Something weird is happening , argument zero should be executable path :S \n"); return 1; } else
    if ( argc <= 2 ) { /*fprintf(stderr,"Please note that you may choose a different binding IP/port as command line parameters.. \"ammarserver 0.0.0.0 8080\"\n");*/ } else
     {
        if (strlen(argv[1])>=MAX_INPUT_IP) { fprintf(stderr,"Console argument for binding IP is too long..!\n"); } else
                                           { strncpy(bindIP,argv[1],MAX_INPUT_IP); }
        port=atoi(argv[2]);
        if (port>=MAX_BINDING_PORT) { port=DEFAULT_BINDING_PORT; }
     }

    AmmServer_Start(bindIP,port);

         while (AmmServer_Running())
           {
             usleep(10000);
           }

    AmmServer_Stop();

    return 0;
}
