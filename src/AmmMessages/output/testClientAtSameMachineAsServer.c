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

#include "../../AmmClient/AmmClient.h"
#include "allAmmMessages.h"


void * newMoveMessage(struct moveMessage * pm)
{
    printf("New move message received by final client ( non http )..\n");
    print_move(pm);
}


int main(int argc, char *argv[])
{
    printf("\Client starting up..\n");

    initializeAllMessagesForReading();
    registerCallbackOnNewData_move(newMoveMessage);

         while ( 1 )
           {
             sleep(1);
           }


    return 0;
}
