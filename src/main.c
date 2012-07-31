#include <stdio.h>
#include <stdlib.h>
#include "AmmServerlib/AmmServerlib.h"

#include <unistd.h>

int main()
{
    printf("Ammar Server v0.0 starting up\n");

    AmmServer_Start("0.0.0.0",8081);


         while (AmmServer_Running())
           {
             usleep(10000);
           }


    AmmServer_Stop();

    return 0;
}
