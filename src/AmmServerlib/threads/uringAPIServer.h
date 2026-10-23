/*
AmmarServer , HTTP Server Library

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

#ifndef URINGAPISERVER_H_INCLUDED
#define URINGAPISERVER_H_INCLUDED

#include "../AmmServerlib.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*
   API mode on io_uring instead of the epoll layer : one thread , one or more rings , and every connection
   living as a *direct descriptor* - an index into a ring's own registered file table rather than an entry
   in the process file descriptor table. That is the whole point of this file. A direct descriptor is not
   charged against RLIMIT_NOFILE , so what limits how many connections one process can hold stops being
   `ulimit -n` and becomes the size of that table , which only has to be declared within RLIMIT_NOFILE once ,
   at registration. Measured on 6.8 : with RLIMIT_NOFILE lowered to 64 , a ring with a 64 slot table served
   60 connections while the process held 6 real descriptors , where epoll would have run out at 58.

   And that check is per table , not per process , so a second ring is a second budget : N rings hold N times
   RLIMIT_NOFILE connections in one process , from one thread , with one SQLite writer behind them - which is
   the whole reason to want more than one. ( Measured on 6.8 : 8 rings of 64 slots held ~300 connections at
   once with RLIMIT_NOFILE at 64. ) They are not for parallelism ; there is still exactly one thread.

   Everything else follows from that choice : the accept is a multishot accept that installs each new socket
   straight into the table , receives are multishot over a ring of provided buffers so an idle connection
   holds no receive buffer at all , and sends go out against the direct descriptor. One connection costs a
   32 byte struct here and nothing else until it has bytes in flight.

   This backend is optional in every sense : it needs Linux 5.19 or newer to compile and a working
   io_uring_setup to run , and UringAPI_Start says so rather than failing , leaving the caller to fall back
   to the epoll layer ( which is what threadedServer.c does ).
*/

/**
* @brief Take over API mode for this instance : set up the rings and run them on one thread. The listening
*        socket becomes theirs , and must not be accepted from anywhere else afterwards.
* @param How many rings to spread the connections over , which multiplies the connection ceiling by that
*        many ; 1 is one ring and the ordinary case. Each costs two descriptors and its share of the receive
*        buffers , and a ring beyond the first that cannot be set up is reported and left out rather than
*        failing the server.
* @retval 1=the rings are up and serving , 0=they are not , use the epoll layer instead */
int UringAPI_Start(struct AmmServer_Instance * instance,int serverSocket,unsigned int rings);

/**
* @brief Wait for the ring thread to finish , which it does when the instance is told to stop. Returns at
*        once if the rings never started , and costs nothing while they serve. */
void UringAPI_Wait(struct AmmServer_Instance * instance);

/**
* @brief Ask the ring thread to stop , and wait for it. Safe to call when it never started. */
void UringAPI_Stop(struct AmmServer_Instance * instance);

#ifdef __cplusplus
}
#endif

#endif // URINGAPISERVER_H_INCLUDED
