/*
    Copyright 2012 Ammar Qammaz

    This file is part of AmmarServer.

    AmmarServer is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
    AmmarServer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along with AmmarServer. If not, see http://www.gnu.org/licenses/.
*/

#include <stdio.h>
#include <stdlib.h>
#include "AmmServerlib/AmmServerlib.h"

#include <unistd.h>

int main()
{
    printf("Ammar Server starting up\n");

    AmmServer_Start("0.0.0.0",8081);


         while (AmmServer_Running())
           {
             usleep(10000);
           }


    AmmServer_Stop();

    return 0;
}
