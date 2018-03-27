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

#include "../../InputParser/InputParser_C.h"
#include "../../AmmClient/AmmClient.h"
#include "allAmmMessages.h"


int main(int argc, char *argv[])
{
    printf("Client starting up I will now emitting bogus move messages..\n");

    if (argc<3)
    {
       fprintf(stderr,"Please call with parameters.. \nLike : \n");
       fprintf(stderr,"./AmmMessagesTestClient 127.0.0.1 8080\n");
       return 1;
    }

    struct AmmClient_Instance * connection =  AmmClient_Initialize(argv[1],atoi(argv[2]),30 /*Seconds of timeout*/);
    if (connection!=0)
    {
         while ( 1 )
           {
             moveStatic.velocityX        = (float) (rand()%1000);
             moveStatic.velocityY        = (float) (rand()%1000);
             moveStatic.orientationTheta = (float) (rand()%1000);

             printf("Sending ");
              print_move(&moveStatic);
             printf("\n");

             sendToServer_move(connection , &moveStatic);
             sleep(1);

             //Lets forget it..!
             moveStatic.velocityX        = (float) (rand()%1000);
             moveStatic.velocityY        = (float) (rand()%1000);
             moveStatic.orientationTheta = (float) (rand()%1000);

             readStateFromServer_move(connection,&moveStatic);


             printf("What we have now");
              print_move(&moveStatic);
             printf("\n");
           }

     AmmClient_Close(connection);
    }

    return 0;
}
