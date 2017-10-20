/** @file networkAbstraction.h
* @brief The way to switch from SSL to Regular connections
* @author Ammar Qammaz (AmmarkoV)
*/

#ifndef NETWORKABSTRACTION_H_INCLUDED
#define NETWORKABSTRACTION_H_INCLUDED

#include "../AmmServerlib.h"
#include <stdlib.h>


int ASRV_Send(
              struct AmmServer_Instance * instance,
              struct HTTPTransaction * transaction,
              const void *buf,
              size_t len,
              int flags
              );

ssize_t ASRV_Recv(
                  struct AmmServer_Instance * instance,
                  struct HTTPTransaction * transaction,
                  void *buf,
                  size_t len,
                  int flags
                 );

#endif
