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

#define _GNU_SOURCE

#include "uringAPIServer.h"

//Everything this file does needs io_uring with registered files , provided buffer rings and multishot
//receives , which is Linux 5.19 and a kernel header to match. Anywhere else it is an honest "no" so the
//caller can use the epoll layer , and not a build failure.
#if defined(__linux__) && defined(__has_include)
  #if __has_include(<linux/io_uring.h>)
    #include <linux/io_uring.h>
    //Tested against the macros only , because the register opcodes next to them ( IORING_REGISTER_FILES2 ,
    //IORING_REGISTER_PBUF_RING ) are enum values that no #if can see. Between them these four say the header
    //is 6.0 or newer , which is where multishot receives arrived.
    #if defined(IORING_FILE_INDEX_ALLOC) && defined(IORING_ACCEPT_MULTISHOT) && defined(IORING_RECV_MULTISHOT) && defined(IORING_RSRC_REGISTER_SPARSE) && defined(IORING_OFF_PBUF_RING)
      #define URING_API_SUPPORTED 1
    #endif
  #endif
#endif

#if !defined(URING_API_SUPPORTED)

int UringAPI_Start(struct AmmServer_Instance * instance,int serverSocket,unsigned int rings) { return 0; }
void UringAPI_Wait(struct AmmServer_Instance * instance) { }
void UringAPI_Stop(struct AmmServer_Instance * instance) { }

#else

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "apiProtocol.h"

#include "../server_configuration.h"
#include "../tools/logs.h"

//Both are a kernel 6.1 flag , asked for where the header is older than that : these values are ABI , and a
//kernel that does not know them refuses the setup , which RingSetup already retries without them.
#ifndef IORING_SETUP_SINGLE_ISSUER
 #define IORING_SETUP_SINGLE_ISSUER (1U << 12)
#endif
#ifndef IORING_SETUP_DEFER_TASKRUN
 #define IORING_SETUP_DEFER_TASKRUN (1U << 13)
#endif

//How many submissions can be in flight at once. Every connection has at most one receive and one send
//outstanding , and both are armed again only from their own completion , so this is never the constraint.
#define URING_SQ_ENTRIES 4096
//The provided buffers multishot receives land in. A request is a few hundred bytes , so 2 KB takes nearly
//every one in a single buffer ; anything longer is appended to the connection's own partial buffer instead.
//These are the only receive memory in the server , and they are a queue depth rather than a per connection
//cost : an idle connection holds none ( which is what 65,000 of them at once rests on ). The count is the
//budget for the whole server , shared out between the rings , so adding rings adds connection slots without
//adding receive memory.
#define URING_RECV_BUFFER_SIZE 2048
#define URING_RECV_BUFFER_COUNT 1024
#define URING_RECV_BUFFER_MINIMUM 128
#define URING_RECV_BUFFER_GROUP 1
//Responses in flight. A send is asynchronous , so its bytes have to stay untouched until its completion
//arrives , which means one buffer per outstanding send rather than one for the server. Sends to a socket
//with room in its send buffer complete immediately , so this is a queue depth , not a connection count ,
//and one pool serves every ring because one thread serves every ring.
#define URING_SEND_TICKETS 32
//Beyond this a table costs more memory than the connections could ever be worth ; RLIMIT_NOFILE normally
//says less anyway , and the smaller of the two wins.
#define URING_MAX_SLOTS (256*1024)
//Rings. Each one is a file table of its own , so this is the multiplier on the connection ceiling , and
//16 of them is 1,048,576 connections against the usual LimitNOFILE=65535 - past anything one vCPU could
//answer. Each costs two real descriptors ( the ring and its eventfd ) and its share of the receive buffers.
#define URING_MAX_RINGS 16
//How many free slots a ring keeps in hand while it is the one accepting. The accept moves to the next ring
//on reaching this , rather than on running out : a table that fills while a multishot accept is still armed
//on it answers the connection that arrives next with EMFILE , and the kernel has by then already accepted
//and must drop it , so the client sees a reset rather than a queue. Moving over early costs these slots only
//while another ring still has room - the last ring to fill uses every one of them.
#define URING_ACCEPT_RESERVE 64

#define URING_OP_ACCEPT  1
#define URING_OP_RECV    2
#define URING_OP_SEND    3
#define URING_OP_CLOSE   4
#define URING_OP_PROVIDE 5
#define URING_OP_CANCEL  6

//user_data carries what completed , for which connection , and which incarnation of it. The generation is
//what makes a late completion safe : a slot freed by a close can be handed to a new accept before the
//canceled receive on the old connection reports back , and without the generation that completion would be
//applied to whoever holds the slot now. Which ring it belongs to is not in here and does not need to be :
//a completion is only ever read from the ring it was submitted to.
#define URING_USER_DATA(op,generation,slot) \
  ( ((unsigned long long)(op)<<56) | (((unsigned long long)(generation)&0xFFFFFFULL)<<32) | (unsigned long long)(slot) )
#define URING_USER_DATA_OP(ud)         ((unsigned int)((ud)>>56))
#define URING_USER_DATA_GENERATION(ud) ((unsigned int)(((ud)>>32)&0xFFFFFFULL))
#define URING_USER_DATA_SLOT(ud)       ((unsigned int)((ud)&0xFFFFFFFFULL))
#define URING_NO_SLOT 0xFFFFFFFFu

static int uring_setup(unsigned int entries,struct io_uring_params * params)
{ return (int) syscall(__NR_io_uring_setup,entries,params); }
static int uring_enter(int ringFd,unsigned int toSubmit,unsigned int minComplete,unsigned int flags,void * arg,size_t argSize)
{ return (int) syscall(__NR_io_uring_enter,ringFd,toSubmit,minComplete,flags,arg,argSize); }
static int uring_register(int ringFd,unsigned int opcode,void * arg,unsigned int nrArgs)
{ return (int) syscall(__NR_io_uring_register,ringFd,opcode,arg,nrArgs); }

//One connection. Nothing here is a buffer : the bytes coming in are in a provided buffer until they are
//consumed , the bytes going out are in a send ticket , and `in` is allocated only for a connection that is
//holding an incomplete request or has had something arrive behind a response still being sent.
struct UringConnection
{
   struct ApiPartialRequest in;
   unsigned int generation;
   unsigned int nextWaiting;          //queue of connections waiting for a send ticket , URING_NO_SLOT ends it
   unsigned int nextRearm;            //queue of connections whose receive could not be armed , same ending
   unsigned char inUse;
   unsigned char recvArmed;
   unsigned char sendInFlight;
   unsigned char waitingForTicket;
   unsigned char waitingForRearm;
};

//One response on its way out. The buffer is the handler's , so it cannot be reused until the send that is
//reading it has completed ; slot and generation say who to carry on serving afterwards , without trusting
//that the connection is still there.
struct UringSendTicket
{
   struct ApiResponseBuffer buffer;
   char * data;
   unsigned int total;
   unsigned int sent;
   unsigned int slot;
   unsigned int generation;
   unsigned char busy;
   unsigned char closeWhenWritten;
};

//One ring , with the table and the buffers that belong to it. Everything that scales with the number of
//connections is in here , because that is what a second ring is for : a registered file table may be
//declared no larger than RLIMIT_NOFILE , so one ring cannot hold more connections than the descriptor limit
//names - but the limit is checked once per table , at registration , and never again. N tables are N times
//the connections from the same limit , in one process , with one SQLite writer.
struct UringRing
{
   int fd;
   unsigned int * sqHead;
   unsigned int * sqTail;
   unsigned int * sqRingMask;
   unsigned int * sqArray;
   struct io_uring_sqe * sqes;
   unsigned int * cqHead;
   unsigned int * cqTail;
   unsigned int * cqRingMask;
   struct io_uring_cqe * cqes;
   unsigned int sqEntries;
   unsigned int sqTailLocal;   //our own copy of the tail , published to the kernel when we submit
   unsigned int toSubmit;

   //Kept so that a ring that could not be finished can be given back in full rather than left mapped.
   char * sqRingMapping;
   char * cqRingMapping;
   size_t sqRingMappingSize;
   size_t cqRingMappingSize;
   size_t sqesMappingSize;

   //How this ring says it has completions waiting , when it is not the only ring and so cannot simply be
   //the thing the thread blocks on. -1 when the thread blocks on this ring itself.
   int eventFd;
   unsigned char needsFlush;   //enter this ring before reading its completion queue

   struct UringConnection * connections;
   unsigned int connectionSlots;
   unsigned int connectionsOpen;

   unsigned int recvBufferCount;
   int recvBuffersAreRing;
   struct io_uring_buf_ring * recvBufferRing;
   size_t recvBufferRingSize;
   char * recvBufferMemory;

   unsigned int waitingHead;   //connections on this ring waiting for a send ticket
   unsigned int waitingTail;

   unsigned int rearmHead;     //connections on this ring whose receive is waiting for a provided buffer
   unsigned int rearmTail;

   unsigned char acceptArmed;
   unsigned int index;
};

//API mode is one instance with one handler ( the epoll layer's buffers are file static for the same reason ) ,
//so the rings , their tables and their buffers are too.
static struct UringRing ringAt[URING_MAX_RINGS];
static unsigned int ringCount = 1;

static struct UringSendTicket sendTicket[URING_SEND_TICKETS];
//Which ring to look at first for a connection waiting on a ticket , moved on each time one is served so that
//a busy ring cannot starve the others of the shared pool.
static unsigned int ticketCursor = 0;

static int listenSocket = -1;
//There is one accept in the whole server , on one ring at a time : whichever ring accepts a connection is
//the ring that will hold it , so accepting from all of them at once would be the kernel choosing which table
//fills , where this way it is us. Rings fill in turn , and a server that never exceeds one table's worth of
//connections never touches the second ring at all.
static unsigned int acceptRing = 0;
static int acceptStalled = 0;
static time_t lastStallReport = 0;
static time_t lastBufferFailureReport = 0;

//Only used when there is more than one ring : an eventfd per ring , waited on together. Measured on 6.8 ,
//because it is not obvious and the obvious alternative is silently wrong : a registered eventfd does wake
//an epoll_wait on a ring set up with DEFER_TASKRUN , while polling the ring's own fd does not ( it never
//becomes readable , because with DEFER_TASKRUN the completion is only posted once the owning task enters
//the ring - which is why a ring that signals has to be entered in the same pass that saw the signal ).
static int waitEpollFd = -1;

static pthread_t ringThread;
static volatile int ringThreadStarted = 0;
static volatile int ringStartupResult = 0;  //0=still trying , 1=serving , -1=failed
static volatile int ringShouldStop = 0;

/* ------------------------------------------------------------------ the rings themselves ------------ */

static int RingSetup(struct UringRing * r)
{
  struct io_uring_params params;
  memset(&params,0,sizeof(params));

  //SINGLE_ISSUER tells the kernel only one task ever submits ( this thread , which is why the ring is set up
  //here rather than by whoever called UringAPI_Start ) and DEFER_TASKRUN keeps completion work from
  //interrupting that thread at random , running it where we ask for completions instead. On one vCPU that
  //is the difference between work happening in our loop and work happening on top of it.
  params.flags = IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN;

  r->fd = uring_setup(URING_SQ_ENTRIES,&params);
  if (r->fd<0)
   {
     //An older kernel knows io_uring but not these two flags ; it can still do everything else here.
     memset(&params,0,sizeof(params));
     r->fd = uring_setup(URING_SQ_ENTRIES,&params);
     if (r->fd<0) { return 0; }
   }

  size_t sqSize = params.sq_off.array + params.sq_entries*sizeof(unsigned int);
  size_t cqSize = params.cq_off.cqes  + params.cq_entries*sizeof(struct io_uring_cqe);

  int singleMapping = ( (params.features & IORING_FEAT_SINGLE_MMAP)!=0 );
  if (singleMapping) { if (cqSize>sqSize) { sqSize=cqSize; } cqSize=sqSize; }

  char * sqRing = (char*) mmap(0,sqSize,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_POPULATE,r->fd,IORING_OFF_SQ_RING);
  if (sqRing==MAP_FAILED) { return 0; }
  r->sqRingMapping = sqRing;
  r->sqRingMappingSize = sqSize;

  char * cqRing = sqRing;
  if (!singleMapping)
   {
     cqRing = (char*) mmap(0,cqSize,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_POPULATE,r->fd,IORING_OFF_CQ_RING);
     if (cqRing==MAP_FAILED) { return 0; }
     r->cqRingMapping = cqRing;
     r->cqRingMappingSize = cqSize;
   }

  r->sqesMappingSize = params.sq_entries*sizeof(struct io_uring_sqe);
  r->sqes = (struct io_uring_sqe *) mmap(0,r->sqesMappingSize,
                                         PROT_READ|PROT_WRITE,MAP_SHARED|MAP_POPULATE,r->fd,IORING_OFF_SQES);
  if (r->sqes==MAP_FAILED) { r->sqes=0; r->sqesMappingSize=0; return 0; }

  r->sqHead     = (unsigned int*)(sqRing + params.sq_off.head);
  r->sqTail     = (unsigned int*)(sqRing + params.sq_off.tail);
  r->sqRingMask = (unsigned int*)(sqRing + params.sq_off.ring_mask);
  r->sqArray    = (unsigned int*)(sqRing + params.sq_off.array);
  r->cqHead     = (unsigned int*)(cqRing + params.cq_off.head);
  r->cqTail     = (unsigned int*)(cqRing + params.cq_off.tail);
  r->cqRingMask = (unsigned int*)(cqRing + params.cq_off.ring_mask);
  r->cqes       = (struct io_uring_cqe *)(cqRing + params.cq_off.cqes);
  r->sqEntries  = params.sq_entries;
  r->sqTailLocal = *r->sqTail;
  r->toSubmit    = 0;

  return 1;
}

//Give a ring back , for a ring that could not be finished. The table and the provided buffers belong to the
//ring fd and go with it ; the mappings and our own arrays are ours to return.
static void RingRelease(struct UringRing * r)
{
  if (r->sqes!=0)           { munmap(r->sqes,r->sqesMappingSize); }
  if (r->cqRingMapping!=0)  { munmap(r->cqRingMapping,r->cqRingMappingSize); }
  if (r->sqRingMapping!=0)  { munmap(r->sqRingMapping,r->sqRingMappingSize); }
  if (r->recvBufferRing!=0) { munmap(r->recvBufferRing,r->recvBufferRingSize); }
  if (r->recvBufferMemory!=0) { free(r->recvBufferMemory); }
  if (r->connections!=0)    { free(r->connections); }
  if (r->eventFd>=0)        { close(r->eventFd); }
  if (r->fd>=0)             { close(r->fd); }

  unsigned int wasIndex = r->index;
  memset(r,0,sizeof(struct UringRing));
  r->fd = -1;
  r->eventFd = -1;
  r->index = wasIndex;
}

static int SubmitPending(struct UringRing * r,unsigned int minComplete,struct __kernel_timespec * waitFor)
{
  __atomic_store_n(r->sqTail,r->sqTailLocal,__ATOMIC_RELEASE);

  unsigned int flags = 0;
  struct io_uring_getevents_arg arg;
  void * argPointer = 0;
  size_t argSize = 0;

  if ( (minComplete>0) || (waitFor!=0) )
   {
     flags |= IORING_ENTER_GETEVENTS;
     if (waitFor!=0)
      {
        memset(&arg,0,sizeof(arg));
        arg.ts = (unsigned long long)(uintptr_t) waitFor;
        flags |= IORING_ENTER_EXT_ARG;
        argPointer = &arg;
        argSize = sizeof(arg);
      }
   }

  int res = uring_enter(r->fd,r->toSubmit,minComplete,flags,argPointer,argSize);
  if (res>=0) { r->toSubmit=0; return 1; }

  //Everything we asked to be woken for is a normal outcome of waiting , not a failure to submit.
  if ( (errno==ETIME) || (errno==EINTR) || (errno==EBUSY) || (errno==EAGAIN) ) { r->toSubmit=0; return 1; }

  return 0;
}

//Ask the kernel to run this ring's deferred completion work and post it , without waiting for anything new.
//With DEFER_TASKRUN that is the only thing that makes a completion visible , so it is what a ring that said
//it had something gets before its completion queue is read.
static int FlushRing(struct UringRing * r)
{
  __atomic_store_n(r->sqTail,r->sqTailLocal,__ATOMIC_RELEASE);

  int res = uring_enter(r->fd,r->toSubmit,0,IORING_ENTER_GETEVENTS,0,0);
  if (res>=0) { r->toSubmit=0; return 1; }
  if ( (errno==ETIME) || (errno==EINTR) || (errno==EBUSY) || (errno==EAGAIN) ) { r->toSubmit=0; return 1; }

  return 0;
}

static struct io_uring_sqe * GetSqe(struct UringRing * r)
{
  unsigned int head = __atomic_load_n(r->sqHead,__ATOMIC_ACQUIRE);

  if ( (r->sqTailLocal-head) >= r->sqEntries )
   {
     //The submission queue is full , which only happens if we queued thousands of operations without once
     //going back to the kernel. Hand them over and look again.
     if (!SubmitPending(r,0,0)) { return 0; }
     head = __atomic_load_n(r->sqHead,__ATOMIC_ACQUIRE);
     if ( (r->sqTailLocal-head) >= r->sqEntries ) { return 0; }
   }

  unsigned int index = r->sqTailLocal & *r->sqRingMask;
  struct io_uring_sqe * sqe = &r->sqes[index];
  memset(sqe,0,sizeof(struct io_uring_sqe));
  r->sqArray[index] = index;

  ++r->sqTailLocal;
  ++r->toSubmit;
  return sqe;
}

/* ------------------------------------------------------------------ provided receive buffers --------- */

//A receive that could not be armed - no provided buffer free , or no submission queue entry - leaves its
//connection with nothing outstanding on it at all : no completion will ever arrive to notice , so unless
//it is written down here it is stuck until the client gives up. It is written down here , and armed again
//from RecycleRecvBuffer as buffers come back , one connection per buffer. That bound is what keeps it
//from spinning : a connection that finds the buffer already taken simply queues again , and the next
//attempt waits for the next buffer , which waits for a request to be served.
static void QueueForRearm(struct UringRing * r,unsigned int slot)
{
  struct UringConnection * conn = &r->connections[slot];
  if (conn->waitingForRearm) { return; }

  conn->waitingForRearm=1;
  conn->nextRearm=URING_NO_SLOT;

  if (r->rearmTail==URING_NO_SLOT) { r->rearmHead=slot; r->rearmTail=slot; return; }

  r->connections[r->rearmTail].nextRearm = slot;
  r->rearmTail = slot;
}

static void ArmRecv(struct UringRing * r,unsigned int slot);

//One buffer has come back , so one connection that stopped for want of one is armed again. Connections that
//closed while they were queued are skipped here rather than unlinked when they closed , which is what keeps
//closing a constant time operation however long this queue is.
static void RearmOneWaiting(struct UringRing * r)
{
  while (r->rearmHead!=URING_NO_SLOT)
   {
     unsigned int slot = r->rearmHead;
     struct UringConnection * conn = &r->connections[slot];

     r->rearmHead = conn->nextRearm;
     if (r->rearmHead==URING_NO_SLOT) { r->rearmTail=URING_NO_SLOT; }

     conn->nextRearm=URING_NO_SLOT;
     conn->waitingForRearm=0;

     if ( (conn->inUse) && (!conn->recvArmed) && (!conn->sendInFlight) ) { ArmRecv(r,slot); return; }
   }
}

//Hand `count` buffers starting at `first` to the kernel the classic way : one submission , which is queued
//with everything else and goes out on the next enter.
static void ProvideBuffersClassic(struct UringRing * r,unsigned int first,unsigned int count)
{
  struct io_uring_sqe * sqe = GetSqe(r);
  if (sqe==0) { return; }

  sqe->opcode    = IORING_OP_PROVIDE_BUFFERS;
  sqe->fd        = (int) count;   //how many
  sqe->addr      = (unsigned long long)(uintptr_t)( r->recvBufferMemory + (size_t)first*URING_RECV_BUFFER_SIZE );
  sqe->len       = URING_RECV_BUFFER_SIZE;  //each
  sqe->off       = first;         //the first buffer id
  sqe->buf_group = URING_RECV_BUFFER_GROUP;
  sqe->user_data = URING_USER_DATA(URING_OP_PROVIDE,0,first);
}

static int RegisterRecvBuffers(struct UringRing * r,unsigned int count)
{
  r->recvBufferCount = count;
  r->recvBufferRingSize = count*sizeof(struct io_uring_buf);
  r->recvBufferRing = (struct io_uring_buf_ring *) mmap(0,r->recvBufferRingSize,PROT_READ|PROT_WRITE,
                                                        MAP_ANONYMOUS|MAP_PRIVATE,-1,0);
  if (r->recvBufferRing==MAP_FAILED) { r->recvBufferRing=0; return 0; }

  r->recvBufferMemory = (char*) malloc((size_t)count*URING_RECV_BUFFER_SIZE);
  if (r->recvBufferMemory==0) { errorID(ASV_ERROR_COULD_NOT_ALLOCATE_MEMORY); return 0; }

  struct io_uring_buf_reg registration;
  memset(&registration,0,sizeof(registration));
  registration.ring_addr    = (unsigned long long)(uintptr_t) r->recvBufferRing;
  registration.ring_entries = count;
  registration.bgid         = URING_RECV_BUFFER_GROUP;

  if (uring_register(r->fd,IORING_REGISTER_PBUF_RING,&registration,1)==0)
   {
     r->recvBuffersAreRing = 1;

     r->recvBufferRing->tail = 0;
     unsigned int i=0;
     for (i=0; i<count; i++)
      {
        struct io_uring_buf * buffer = &r->recvBufferRing->bufs[i];
        buffer->addr = (unsigned long long)(uintptr_t)( r->recvBufferMemory + (size_t)i*URING_RECV_BUFFER_SIZE );
        buffer->len  = URING_RECV_BUFFER_SIZE;
        buffer->bid  = (unsigned short) i;
      }
     __atomic_store_n(&r->recvBufferRing->tail,(unsigned short)count,__ATOMIC_RELEASE);

     return 1;
   }

  //No mapped ring here. The buffers themselves are fine , they just have to be handed over one submission
  //at a time , so the whole set goes over now and each one goes back as it is consumed.
  munmap(r->recvBufferRing,r->recvBufferRingSize);
  r->recvBufferRing = 0;
  r->recvBufferRingSize = 0;
  r->recvBuffersAreRing = 0;

  ProvideBuffersClassic(r,0,count);

  //Waited for , rather than left to go out with the first accept : until it has completed there is nowhere
  //for a receive to put anything.
  if (!SubmitPending(r,1,0)) { return 0; }

  unsigned int head = *r->cqHead;
  unsigned int tail = __atomic_load_n(r->cqTail,__ATOMIC_ACQUIRE);
  int provided = 0;

  while (head!=tail)
   {
     struct io_uring_cqe * cqe = &r->cqes[head & *r->cqRingMask];
     if (URING_USER_DATA_OP(cqe->user_data)==URING_OP_PROVIDE) { provided = (cqe->res>=0); }
     ++head;
   }
  __atomic_store_n(r->cqHead,head,__ATOMIC_RELEASE);

  return provided;
}

//Hand one buffer back to the kernel , once whatever arrived in it has been consumed.
static void RecycleRecvBuffer(struct UringRing * r,unsigned int bufferId)
{
  if (!r->recvBuffersAreRing) { ProvideBuffersClassic(r,bufferId,1); RearmOneWaiting(r); return; }

  //We are the only writer of this ring's tail , so adding to it is a store , not a compare and swap.
  unsigned short tail = r->recvBufferRing->tail;
  struct io_uring_buf * buffer = &r->recvBufferRing->bufs[tail & (r->recvBufferCount-1)];

  buffer->addr = (unsigned long long)(uintptr_t)( r->recvBufferMemory + (size_t)bufferId*URING_RECV_BUFFER_SIZE );
  buffer->len  = URING_RECV_BUFFER_SIZE;
  buffer->bid  = (unsigned short) bufferId;

  __atomic_store_n(&r->recvBufferRing->tail,(unsigned short)(tail+1),__ATOMIC_RELEASE);

  RearmOneWaiting(r);
}

/* ------------------------------------------------------------------ send tickets -------------------- */

static int AcquireSendTicket(void)
{
  unsigned int i=0;
  for (i=0; i<URING_SEND_TICKETS; i++)
   {
     if (!sendTicket[i].busy) { sendTicket[i].busy=1; sendTicket[i].sent=0; return (int) i; }
   }
  return -1;
}

static void ReleaseSendTicket(unsigned int ticket)
{
  sendTicket[ticket].busy=0;
  sendTicket[ticket].data=0;
  sendTicket[ticket].total=0;
  sendTicket[ticket].sent=0;
  sendTicket[ticket].slot=URING_NO_SLOT;
}

//A connection that wanted a ticket while every one of them was busy waits in its ring's queue , in the order
//it asked , and is served from HandleSendCompletion as tickets come back.
static void QueueForTicket(struct UringRing * r,unsigned int slot)
{
  struct UringConnection * conn = &r->connections[slot];
  if (conn->waitingForTicket) { return; }

  conn->waitingForTicket=1;
  conn->nextWaiting=URING_NO_SLOT;

  if (r->waitingTail==URING_NO_SLOT) { r->waitingHead=slot; r->waitingTail=slot; return; }

  r->connections[r->waitingTail].nextWaiting = slot;
  r->waitingTail = slot;
}

//The next connection waiting for a ticket , from whichever ring , taking the rings in turn so that the pool
//is shared rather than held by whoever asks most often.
static struct UringRing * TakeNextWaitingForTicket(unsigned int * slotFound)
{
  unsigned int tried=0;
  for (tried=0; tried<ringCount; tried++)
   {
     struct UringRing * r = &ringAt[ (ticketCursor+tried) % ringCount ];

     while (r->waitingHead!=URING_NO_SLOT)
      {
        unsigned int slot = r->waitingHead;
        struct UringConnection * conn = &r->connections[slot];

        r->waitingHead = conn->nextWaiting;
        if (r->waitingHead==URING_NO_SLOT) { r->waitingTail=URING_NO_SLOT; }

        conn->nextWaiting=URING_NO_SLOT;
        conn->waitingForTicket=0;

        //A connection that closed while it was queued is skipped rather than removed when it closed , so
        //closing stays a constant time operation however long this queue is.
        if (conn->inUse)
         {
           ticketCursor = (r->index+1) % ringCount;
           *slotFound = slot;
           return r;
         }
      }
   }

  return 0;
}

/* ------------------------------------------------------------------ operations ---------------------- */

static unsigned int FreeSlots(struct UringRing * r)
{
  if (r->connectionsOpen>=r->connectionSlots) { return 0; }
  return r->connectionSlots - r->connectionsOpen;
}

static void ArmAccept(struct UringRing * r)
{
  if (r->acceptArmed) { return; }

  struct io_uring_sqe * sqe = GetSqe(r);
  if (sqe==0) { return; }

  //One multishot accept covers every connection that will ever arrive , and IORING_FILE_INDEX_ALLOC puts
  //each of them into a free slot of this ring's registered table instead of a file descriptor.
  sqe->opcode     = IORING_OP_ACCEPT;
  sqe->fd         = listenSocket;
  sqe->ioprio     = IORING_ACCEPT_MULTISHOT;
  sqe->file_index = IORING_FILE_INDEX_ALLOC;
  sqe->user_data  = URING_USER_DATA(URING_OP_ACCEPT,0,0);

  r->acceptArmed = 1;
  acceptStalled  = 0;
}

static void CancelAccept(struct UringRing * r)
{
  if (!r->acceptArmed) { return; }

  struct io_uring_sqe * sqe = GetSqe(r);
  if (sqe==0) { return; }

  //The multishot accept stays armed until this reports back : the -ECANCELED completion is what clears it ,
  //and anything the kernel had already accepted on the way still arrives and is still served , which is why
  //the ring being moved away from has to have slots left for it.
  sqe->opcode       = IORING_OP_ASYNC_CANCEL;
  sqe->addr         = URING_USER_DATA(URING_OP_ACCEPT,0,0);
  sqe->cancel_flags = IORING_ASYNC_CANCEL_ALL;
  sqe->user_data    = URING_USER_DATA(URING_OP_CANCEL,0,0);
}

/**
* @brief Put the server's one accept on a ring that has room for what it brings in , looking from `startAt`
*        onwards. A ring with room to spare is preferred over one with a handful of slots left ; only when no
*        ring has any room to spare does a nearly full one get it , and only when none has a single free slot
*        does accepting stop until a close frees one. */
static void ArmAcceptSomewhere(unsigned int startAt)
{
  unsigned int i=0;

  for (i=0; i<ringCount; i++)
   {
     unsigned int which = (startAt+i) % ringCount;
     if (FreeSlots(&ringAt[which])>URING_ACCEPT_RESERVE) { acceptRing=which; ArmAccept(&ringAt[which]); return; }
   }

  for (i=0; i<ringCount; i++)
   {
     unsigned int which = (startAt+i) % ringCount;
     if (FreeSlots(&ringAt[which])>0) { acceptRing=which; ArmAccept(&ringAt[which]); return; }
   }

  //Every table is full : every slot on every ring is a connection being served. Accepting is hopeless until
  //one of them closes , so the accept is not armed again until that happens - the same shape as the epoll
  //layer's EMFILE backoff , and for the same reason.
  acceptStalled = 1;

  time_t rightNow = time(0);
  if (rightNow!=lastStallReport)
   {
     lastStallReport = rightNow; //at most one line a second , however long it lasts
     errorID(ASV_ERROR_OUT_OF_RESOURCES_TO_ACCOMODATE_CLIENT);
   }
}

//Move the accept off a ring that is nearly full , onto one that is not. Both are briefly armed , which is
//the point of the reserve : the ring being left still has slots for whatever the kernel accepts on it in the
//meantime.
static void MoveAcceptOn(struct UringRing * from)
{
  unsigned int leaving = from->index;

  ArmAcceptSomewhere(leaving+1);

  if ( (!acceptStalled) && (acceptRing!=leaving) ) { CancelAccept(from); }
}

static void ArmRecv(struct UringRing * r,unsigned int slot)
{
  struct UringConnection * conn = &r->connections[slot];

  struct io_uring_sqe * sqe = GetSqe(r);
  if (sqe==0) { QueueForRearm(r,slot); return; }

  //Multishot , and reading into whichever provided buffer is free when bytes turn up : the connection is not
  //holding one in the meantime , which is what makes an idle connection cost nothing but its struct.
  sqe->opcode    = IORING_OP_RECV;
  sqe->fd        = (int) slot;
  sqe->flags     = IOSQE_FIXED_FILE|IOSQE_BUFFER_SELECT;
  sqe->ioprio    = IORING_RECV_MULTISHOT;
  sqe->buf_group = URING_RECV_BUFFER_GROUP;
  sqe->user_data = URING_USER_DATA(URING_OP_RECV,conn->generation,slot);

  conn->recvArmed = 1;
}

static void SubmitSend(struct UringRing * r,unsigned int slot,unsigned int ticket)
{
  struct UringConnection * conn = &r->connections[slot];
  struct UringSendTicket * send = &sendTicket[ticket];

  struct io_uring_sqe * sqe = GetSqe(r);
  if (sqe==0) { return; }

  sqe->opcode    = IORING_OP_SEND;
  sqe->fd        = (int) slot;
  sqe->flags     = IOSQE_FIXED_FILE;
  sqe->addr      = (unsigned long long)(uintptr_t)( send->data + send->sent );
  sqe->len       = send->total - send->sent;
  sqe->msg_flags = MSG_NOSIGNAL;
  sqe->user_data = URING_USER_DATA(URING_OP_SEND,ticket,slot);

  conn->sendInFlight = 1;
}

static void CloseConnection(struct UringRing * r,unsigned int slot)
{
  struct UringConnection * conn = &r->connections[slot];
  if (!conn->inUse) { return; }

  int receiveWasArmed = conn->recvArmed;  //read before it is cleared : the cancel below depends on it

  conn->inUse=0;
  conn->recvArmed=0;
  ApiProtocol_PartialFree(&conn->in);
  if (r->connectionsOpen>0) { --r->connectionsOpen; }

  //A multishot receive is a request that is still live , and a live request holds its own reference to the
  //socket. Closing the table slot only drops the table's reference , so on its own it would take the
  //connection out of our hands without ever closing the socket : no FIN , and a client waiting for one until
  //it times out. ( Measured : "Connection: close" answered and then hanging. ) So the receive is canceled
  //first , and the close below is what the socket actually goes away on.
  if (receiveWasArmed)
   {
     struct io_uring_sqe * cancel = GetSqe(r);
     if (cancel!=0)
      {
        cancel->opcode       = IORING_OP_ASYNC_CANCEL;
        cancel->addr         = URING_USER_DATA(URING_OP_RECV,conn->generation,slot);
        cancel->cancel_flags = IORING_ASYNC_CANCEL_ALL;
        cancel->user_data    = URING_USER_DATA(URING_OP_CANCEL,conn->generation,slot);
      }
   }

  struct io_uring_sqe * sqe = GetSqe(r);
  if (sqe!=0)
   {
     sqe->opcode     = IORING_OP_CLOSE;
     sqe->fd         = 0;
     sqe->file_index = slot+1;  //one based , zero means "this is not a direct descriptor"
     sqe->user_data  = URING_USER_DATA(URING_OP_CLOSE,conn->generation,slot);
   }
}

//Answer with a status line and nothing else , then close : for the failures this layer decides by itself ,
//all of which have already made the connection's framing unusable.
static void SendStatusAndClose(struct UringRing * r,unsigned int slot,unsigned int statusCode)
{
  int ticket = AcquireSendTicket();
  if (ticket<0) { CloseConnection(r,slot); return; } //nothing to answer through , so just go

  struct UringSendTicket * send = &sendTicket[ticket];
  send->data  = send->buffer.memory;
  send->total = ApiProtocol_StatusOnlyResponse(statusCode,send->buffer.memory,API_RESPONSE_HEADER_RESERVE);
  send->sent  = 0;
  send->slot  = slot;
  send->generation = r->connections[slot].generation;
  send->closeWhenWritten = 1;

  if (send->total==0) { ReleaseSendTicket((unsigned int) ticket); CloseConnection(r,slot); return; }

  SubmitSend(r,slot,(unsigned int) ticket);
}

/**
* @brief Answer at most one request out of buf. One , because the send that answers it is asynchronous and
*        nothing behind it may be answered until it has completed , or two responses would be racing each
*        other out of the same connection. Whatever is left over is kept for when that send completes. */
static void ServeNextRequest(struct AmmServer_Instance * instance,struct UringRing * r,unsigned int slot,char * buf,unsigned int used)
{
  struct UringConnection * conn = &r->connections[slot];

  struct ApiFrame frame;
  enum ApiFrameResult framing = ApiProtocol_NextFrame(buf,used,0,&frame);

  if (framing==API_FRAME_INCOMPLETE)
   {
     //Not all of it is here yet. A buffer that is already at the cap with no complete request in it holds a
     //request too big to ever be answered , which is a 413 rather than a wait.
     if (!ApiProtocol_PartialKeepTail(&conn->in,buf,used,0)) { SendStatusAndClose(r,slot,413); }
     return;
   }

  if (framing==API_FRAME_REFUSED)
   {
     //Its framing cannot be worked out , so where the next request would begin is unknown and nothing behind
     //it can be trusted either.
     ApiProtocol_PartialFree(&conn->in);
     SendStatusAndClose(r,slot,frame.refuseStatus);
     return;
   }

  int ticket = AcquireSendTicket();
  if (ticket<0)
   {
     //Every response buffer is still being sent. Keep the request and come back to it when one frees.
     if (!ApiProtocol_PartialKeepTail(&conn->in,buf,used,0)) { SendStatusAndClose(r,slot,413); return; }
     QueueForTicket(r,slot);
     return;
   }

  struct UringSendTicket * send = &sendTicket[ticket];

  char * response=0;
  unsigned int failStatus=0;
  unsigned int responseLength = ApiProtocol_Serve(instance,&frame,&send->buffer,&response,&failStatus);

  if (responseLength==0)
   {
     ReleaseSendTicket((unsigned int) ticket);
     ApiProtocol_PartialFree(&conn->in);
     SendStatusAndClose(r,slot,failStatus);
     return;
   }

  //Taken before the send is submitted , because by the time its completion arrives buf may be a provided
  //buffer that has long since been handed back to the kernel.
  if (!ApiProtocol_PartialKeepTail(&conn->in,buf,used,frame.totalLength))
   {
     ReleaseSendTicket((unsigned int) ticket);
     SendStatusAndClose(r,slot,413);
     return;
   }

  send->data  = response;
  send->total = responseLength;
  send->sent  = 0;
  send->slot  = slot;
  send->generation = conn->generation;
  send->closeWhenWritten = (frame.keepAlive==0);

  SubmitSend(r,slot,(unsigned int) ticket);
}

/* ------------------------------------------------------------------ completions --------------------- */

static void HandleAcceptCompletion(struct UringRing * r,struct io_uring_cqe * cqe)
{
  int more = ( (cqe->flags & IORING_CQE_F_MORE)!=0 );

  //A multishot accept that is not continuing is no longer armed , whatever the reason , and that has to be
  //true before anything below decides where to arm the next one.
  if (!more) { r->acceptArmed = 0; }

  if (cqe->res>=0)
   {
     unsigned int slot = (unsigned int) cqe->res;

     if (slot<r->connectionSlots)
      {
        struct UringConnection * conn = &r->connections[slot];
        unsigned int generation = conn->generation + 1;

        //A slot can come back into use while it is still linked into one of this ring's queues : a connection
        //that closes while queued is left to be skipped when it comes up rather than unlinked , which is what
        //keeps closing constant time however long the queue is. Its place in the queue belongs to the queue ,
        //not to the connection , so it has to survive the reuse - clearing it here would cut the queue in two
        //and strand every connection behind it , and leave a tail pointing at a link that no longer leads
        //anywhere. For a slot that is in no queue these are never read , so carrying them over costs nothing.
        unsigned int  queuedNextWaiting = conn->nextWaiting;
        unsigned int  queuedNextRearm   = conn->nextRearm;
        unsigned char queuedForTicket   = conn->waitingForTicket;
        unsigned char queuedForRearm    = conn->waitingForRearm;

        memset(conn,0,sizeof(struct UringConnection));
        conn->generation       = generation;  //never reset , so a late completion can always be told apart
        conn->nextWaiting      = queuedNextWaiting;
        conn->nextRearm        = queuedNextRearm;
        conn->waitingForTicket = queuedForTicket;
        conn->waitingForRearm  = queuedForRearm;
        conn->inUse            = 1;
        ++r->connectionsOpen;

        ArmRecv(r,slot);
      }

     //This ring is filling up. With somewhere else to put the next connections , move over now rather than
     //on the EMFILE that running out would otherwise mean - that one costs a client its connection. With
     //only one ring there is nowhere to move to , and every slot is used before accepting gives up.
     if ( (ringCount>1) && (acceptRing==r->index) && (FreeSlots(r)<=URING_ACCEPT_RESERVE) )
      {
        MoveAcceptOn(r);
        return;
      }
   } else
   {
     if (cqe->res==-ECANCELED) { return; } //we moved the accept to another ring , this is it standing down

     if ( (cqe->res==-EMFILE) || (cqe->res==-ENFILE) )
      {
        //This table is full. Another ring may still have room ; if none has , accepting stops until a close.
        ArmAcceptSomewhere(r->index+1);
        return;
      }
   }

  if (!more && (acceptRing==r->index) && !acceptStalled) { ArmAccept(r); }
}

static void HandleRecvCompletion(struct AmmServer_Instance * instance,struct UringRing * r,struct io_uring_cqe * cqe)
{
  unsigned int slot = URING_USER_DATA_SLOT(cqe->user_data);
  unsigned int generation = URING_USER_DATA_GENERATION(cqe->user_data);

  unsigned int bufferId = URING_NO_SLOT;
  if (cqe->flags & IORING_CQE_F_BUFFER) { bufferId = (unsigned int)(cqe->flags >> IORING_CQE_BUFFER_SHIFT); }

  struct UringConnection * conn = (slot<r->connectionSlots) ? &r->connections[slot] : 0;

  //A completion for a connection that has already gone , possibly one whose slot now holds somebody else.
  //The buffer it consumed still has to go back , whoever it was for.
  if ( (conn==0) || (!conn->inUse) || (conn->generation!=generation) )
   {
     if (bufferId!=URING_NO_SLOT) { RecycleRecvBuffer(r,bufferId); }
     return;
   }

  if (!(cqe->flags & IORING_CQE_F_MORE)) { conn->recvArmed=0; }

  if (cqe->res==0)
   {
     //Orderly shutdown by the client.
     if (bufferId!=URING_NO_SLOT) { RecycleRecvBuffer(r,bufferId); }
     CloseConnection(r,slot);
     return;
   }

  if (cqe->res<0)
   {
     if (bufferId!=URING_NO_SLOT) { RecycleRecvBuffer(r,bufferId); }

     if (cqe->res==-ENOBUFS)
      {
        //Every provided buffer is holding bytes that have not been consumed yet. This connection now has
        //nothing outstanding on it , so nothing would ever come back to notice ; it waits in the queue and
        //is armed again from RecycleRecvBuffer as the buffers are consumed and handed back.
        conn->recvArmed=0;
        QueueForRearm(r,slot);
        return;
      }

     if (cqe->res==-ECANCELED) { return; } //the close we submitted is what canceled it

     CloseConnection(r,slot);
     return;
   }

  if (bufferId==URING_NO_SLOT) { CloseConnection(r,slot); return; } //bytes with no buffer cannot happen

  char * data = r->recvBufferMemory + (size_t)bufferId*URING_RECV_BUFFER_SIZE;
  unsigned int length = (unsigned int) cqe->res;

  if (conn->sendInFlight)
   {
     //Its answer is still going out , so these bytes wait their turn ; HandleSendCompletion serves them.
     if (!ApiProtocol_PartialAppend(&conn->in,data,length))
      {
        RecycleRecvBuffer(r,bufferId);
        CloseConnection(r,slot);
        return;
      }
     RecycleRecvBuffer(r,bufferId);
   } else
  if (conn->in.used>0)
   {
     //Continuing a request that arrived in pieces.
     int appended = ApiProtocol_PartialAppend(&conn->in,data,length);
     RecycleRecvBuffer(r,bufferId);

     if (!appended) { SendStatusAndClose(r,slot,413); return; }

     ServeNextRequest(instance,r,slot,conn->in.data,conn->in.used);
   } else
   {
     //The usual case : a whole request , in a buffer the kernel chose , served without it being copied
     //anywhere first.
     ServeNextRequest(instance,r,slot,data,length);
     RecycleRecvBuffer(r,bufferId);
   }

  //A buffer has just gone back , so a receive that stopped for want of one can be armed again.
  if ( (conn->inUse) && (!conn->recvArmed) ) { ArmRecv(r,slot); }
}

static void HandleSendCompletion(struct AmmServer_Instance * instance,struct UringRing * r,struct io_uring_cqe * cqe)
{
  unsigned int ticket = URING_USER_DATA_GENERATION(cqe->user_data);
  unsigned int slot   = URING_USER_DATA_SLOT(cqe->user_data);

  if (ticket>=URING_SEND_TICKETS) { return; }

  struct UringSendTicket * send = &sendTicket[ticket];
  struct UringConnection * conn = (slot<r->connectionSlots) ? &r->connections[slot] : 0;

  //Is this still the connection the send was for ? If not , all that is left to do is give the ticket back.
  int stillTheirs = ( (conn!=0) && (conn->inUse) && (conn->generation==send->generation) );

  if (cqe->res<=0)
   {
     ReleaseSendTicket(ticket);
     if (stillTheirs) { conn->sendInFlight=0; CloseConnection(r,slot); }
     return;
   }

  send->sent += (unsigned int) cqe->res;

  if (send->sent < send->total)
   {
     //The socket took part of it. The rest goes out of the same buffer , which is why the ticket owns it.
     if (!stillTheirs) { ReleaseSendTicket(ticket); return; }
     SubmitSend(r,slot,ticket);
     return;
   }

  int closeWhenWritten = send->closeWhenWritten;
  ReleaseSendTicket(ticket);

  if (stillTheirs)
   {
     conn->sendInFlight = 0;

     if (closeWhenWritten) { CloseConnection(r,slot); } else
      {
        //Anything that arrived behind this response is in hand already and will not be reported again.
        if (conn->in.used>0) { ServeNextRequest(instance,r,slot,conn->in.data,conn->in.used); }
        if ( (conn->inUse) && (!conn->recvArmed) && (!conn->sendInFlight) ) { ArmRecv(r,slot); }
      }
   }

  //A ticket has come free , so a connection that was waiting for one can be served now - on any ring , since
  //the pool is the whole server's.
  unsigned int waiting = URING_NO_SLOT;
  struct UringRing * waitingRing = TakeNextWaitingForTicket(&waiting);
  if (waitingRing!=0)
   {
     struct UringConnection * waitingConn = &waitingRing->connections[waiting];
     if ( (!waitingConn->sendInFlight) && (waitingConn->in.used>0) )
      {
        ServeNextRequest(instance,waitingRing,waiting,waitingConn->in.data,waitingConn->in.used);
      }
   }
}

static void HandleProvideCompletion(struct io_uring_cqe * cqe)
{
  if (cqe->res>=0) { return; }

  //A buffer that could not be handed back is a buffer out of circulation for good , so this is worth a line ,
  //rate limited like every other repeatable complaint in here.
  time_t rightNow = time(0);
  if (rightNow!=lastBufferFailureReport)
   {
     lastBufferFailureReport = rightNow;
     fprintf(stderr,"API mode : could not give a receive buffer back to the ring ( %s )\n",strerror(-cqe->res));
   }
}

static void HandleCloseCompletion(struct UringRing * r,struct io_uring_cqe * cqe)
{
  //The slot is back in this ring's table , so if accepting had stopped for want of one it can start again ,
  //here , where the slot just came free.
  if (acceptStalled) { ArmAcceptSomewhere(r->index); }
}

/* ------------------------------------------------------------------ the loop ------------------------ */

static int DrainRing(struct AmmServer_Instance * instance,struct UringRing * r)
{
  int handled = 0;

  unsigned int head = *r->cqHead;
  unsigned int tail = __atomic_load_n(r->cqTail,__ATOMIC_ACQUIRE);

  while (head!=tail)
   {
     struct io_uring_cqe * cqe = &r->cqes[head & *r->cqRingMask];

     switch (URING_USER_DATA_OP(cqe->user_data))
      {
        case URING_OP_ACCEPT : HandleAcceptCompletion(r,cqe);            break;
        case URING_OP_RECV   : HandleRecvCompletion(instance,r,cqe);     break;
        case URING_OP_SEND   : HandleSendCompletion(instance,r,cqe);     break;
        case URING_OP_CLOSE  : HandleCloseCompletion(r,cqe);             break;
        case URING_OP_PROVIDE: HandleProvideCompletion(cqe);             break;
        //Nothing to do for a cancel : what it cancels reports back on its own , and a cancel that found
        //nothing to cancel ( -ENOENT , -EALREADY ) is a race we have already handled by closing.
        case URING_OP_CANCEL : break;
      };

     ++head;
     ++handled;
     //Told to the kernel as we go rather than at the end , so a burst that fills the completion queue
     //while we are working through it still has somewhere to land.
     __atomic_store_n(r->cqHead,head,__ATOMIC_RELEASE);
   }

  return handled;
}

//Block until some ring has something , or a second has gone by so that being told to stop is noticed. With
//one ring that is the ring itself , the cheapest possible wait ; with several it is their eventfds , because
//io_uring_enter can only ever wait on one ring.
static int WaitForAnyRing(struct __kernel_timespec * oneSecond)
{
  if (ringCount==1) { return SubmitPending(&ringAt[0],1,oneSecond); }

  struct epoll_event woken[URING_MAX_RINGS];
  int count = epoll_wait(waitEpollFd,woken,(int)ringCount,1000);

  if (count<=0)
   {
     //A timeout , or EINTR - which is how a ring set up without DEFER_TASKRUN reports in , its completion
     //work having interrupted the wait to run. Either way every ring is worth a look.
     unsigned int i=0;
     for (i=0; i<ringCount; i++) { ringAt[i].needsFlush=1; }
     return 1;
   }

  int i=0;
  for (i=0; i<count; i++)
   {
     unsigned int which = woken[i].data.u32;
     if (which>=ringCount) { continue; }

     //Read so that it stops being readable ; what it counted does not matter , only that this ring has to be
     //entered before its completion queue says anything.
     unsigned long long signalled=0;
     if (read(ringAt[which].eventFd,&signalled,sizeof(signalled))<0) { /* nothing to do about it */ }
     ringAt[which].needsFlush=1;
   }

  return 1;
}

/* ------------------------------------------------------------------ the thread ---------------------- */

static int RegisterConnectionTable(struct UringRing * r)
{
  //The table may be declared no larger than RLIMIT_NOFILE allows , which is checked once , here. After that
  //the descriptors in it are the ring's and are not charged against that limit again - so the whole of it is
  //available for connections , where a process accepting into file descriptors has to share the same number
  //with its listening socket , its database and its standard streams. The check is per table , not per
  //process , which is what makes a second ring a second budget.
  struct rlimit descriptorLimit;
  unsigned int slots = URING_MAX_SLOTS;

  if (getrlimit(RLIMIT_NOFILE,&descriptorLimit)==0)
   {
     //It is the *soft* limit the kernel checks the table against , and a process may raise its own soft
     //limit to its hard limit freely. Doing that here is the difference between a table sized for the
     //connections this service was actually granted and one silently sized for a login shell's default of
     //1024 , which is the shape of mistake that shows up as EMFILE with a million descriptors going spare.
     //Only ever upwards , only within what was already granted , and silent unless it changed something.
     if ( (descriptorLimit.rlim_cur!=RLIM_INFINITY) && (descriptorLimit.rlim_cur<descriptorLimit.rlim_max) )
      {
        struct rlimit raised;
        raised.rlim_cur = descriptorLimit.rlim_max;
        raised.rlim_max = descriptorLimit.rlim_max;

        if (setrlimit(RLIMIT_NOFILE,&raised)==0)
         {
           descriptorLimit.rlim_cur = raised.rlim_cur;
           fprintf(stderr,"API mode : raised this process's descriptor soft limit to its hard limit , which is what bounds each ring's table\n");
         }
      }

     if ( (descriptorLimit.rlim_cur!=RLIM_INFINITY) && (descriptorLimit.rlim_cur<slots) )
      { slots = (unsigned int) descriptorLimit.rlim_cur; }
   }
  if (slots==0) { return 0; }

  struct io_uring_rsrc_register registration;
  memset(&registration,0,sizeof(registration));
  registration.nr    = slots;
  registration.flags = IORING_RSRC_REGISTER_SPARSE;

  while (uring_register(r->fd,IORING_REGISTER_FILES2,&registration,sizeof(registration))<0)
   {
     if ( (errno!=EMFILE) && (errno!=ENOMEM) ) { return 0; }
     if (slots<=1024) { return 0; }
     slots /= 2;
     registration.nr = slots;
   }

  //calloc rather than malloc , and never touched until a slot is used : the pages behind the slots that
  //never see a connection are never faulted in , so a table sized for 65,000 costs what is in use.
  r->connections = (struct UringConnection *) calloc(slots,sizeof(struct UringConnection));
  if (r->connections==0) { errorID(ASV_ERROR_COULD_NOT_ALLOCATE_MEMORY); return 0; }

  r->connectionSlots = slots;
  return 1;
}

//The eventfd this ring reports completions on , for when the thread cannot simply block on the ring itself.
static int RegisterEventFd(struct UringRing * r)
{
  r->eventFd = eventfd(0,EFD_NONBLOCK|EFD_CLOEXEC);
  if (r->eventFd<0) { return 0; }

  if (uring_register(r->fd,IORING_REGISTER_EVENTFD,&r->eventFd,1)<0) { return 0; }

  struct epoll_event ev;
  memset(&ev,0,sizeof(ev));
  ev.events  = EPOLLIN;
  ev.data.u32 = r->index;

  if (epoll_ctl(waitEpollFd,EPOLL_CTL_ADD,r->eventFd,&ev)!=0) { return 0; }

  return 1;
}

//Build one ring , whole : its queues , its file table , its share of the receive buffers , and the eventfd it
//announces itself on when it is not the only one.
static int RingBringUp(struct UringRing * r,unsigned int recvBuffers)
{
  if (!RingSetup(r))                         { return 0; }
  if (!RegisterConnectionTable(r))           { return 0; }
  if (!RegisterRecvBuffers(r,recvBuffers))   { return 0; }
  if ( (ringCount>1) && (!RegisterEventFd(r)) ) { return 0; }

  r->waitingHead = URING_NO_SLOT;
  r->waitingTail = URING_NO_SLOT;
  r->rearmHead   = URING_NO_SLOT;
  r->rearmTail   = URING_NO_SLOT;

  return 1;
}

static int RingStartup(struct AmmServer_Instance * instance)
{
  //The receive buffers are a budget for the server rather than a cost per ring , so they are shared out -
  //halved rather than divided so that the count stays a power of two , which is what the mapped buffer ring
  //indexes with.
  unsigned int buffersPerRing = URING_RECV_BUFFER_COUNT;
  while ( (buffersPerRing>URING_RECV_BUFFER_MINIMUM) && (buffersPerRing*ringCount>URING_RECV_BUFFER_COUNT) )
   { buffersPerRing /= 2; }

  if (ringCount>1)
   {
     waitEpollFd = epoll_create1(EPOLL_CLOEXEC);
     if (waitEpollFd<0) { fprintf(stderr,"API mode : could not create the epoll the rings are waited on ( %s )\n",strerror(errno)); return 0; }
   }

  unsigned int i=0;
  for (i=0; i<ringCount; i++)
   {
     memset(&ringAt[i],0,sizeof(struct UringRing));
     ringAt[i].fd      = -1;
     ringAt[i].eventFd = -1;
     ringAt[i].index   = i;
   }

  for (i=0; i<ringCount; i++)
   {
     if (RingBringUp(&ringAt[i],buffersPerRing)) { continue; }

     //The first ring is the server ; without it there is nothing to fall back from , and UringAPI_Start says
     //so to its caller. A later one is only capacity , so what came up is served with and the rest is said
     //out loud rather than taking the whole server down with it.
     if (i==0)
      {
        fprintf(stderr,"API mode : could not set up an io_uring ring ( %s )\n",strerror(errno));
        return 0;
      }

     fprintf(stderr,"API mode : ring %u of %u would not come up ( %s ) , serving with %u\n",
             i+1,ringCount,strerror(errno),i);
     RingRelease(&ringAt[i]);
     ringCount = i;
     break;
   }

  for (i=0; i<URING_SEND_TICKETS; i++)
   {
     if (!ApiProtocol_AllocateResponseBuffer(instance,&sendTicket[i].buffer)) { return 0; }
     sendTicket[i].slot = URING_NO_SLOT;
   }

  //Both are inherited by every socket the accept creates , which is the only way to set them when the
  //accepted socket never becomes a file descriptor this process can call setsockopt on.
  int yes=1;
  setsockopt(listenSocket,IPPROTO_TCP,TCP_NODELAY,&yes,sizeof(yes));
  setsockopt(listenSocket,SOL_SOCKET,SO_KEEPALIVE,&yes,sizeof(yes));

  unsigned long long totalSlots = 0;
  for (i=0; i<ringCount; i++) { totalSlots += ringAt[i].connectionSlots; }

  fprintf(stderr,"API mode : io_uring , %u ring%s , %llu connection slots ( %u each ) , %u receive buffers of %u bytes each ( %s )\n",
          ringCount,(ringCount==1)?"":"s",totalSlots,ringAt[0].connectionSlots,
          buffersPerRing*ringCount,(unsigned int)URING_RECV_BUFFER_SIZE,
          ringAt[0].recvBuffersAreRing ? "mapped buffer ring" : "classic provided buffers");

  return 1;
}

static void * UringAPIThread(void * ptr)
{
  struct AmmServer_Instance * instance = (struct AmmServer_Instance *) ptr;

  if (!RingStartup(instance)) { ringStartupResult=-1; return 0; }

  ArmAcceptSomewhere(0);
  ringStartupResult=1;

  struct __kernel_timespec oneSecond;
  oneSecond.tv_sec  = 1;  //only so that stopping is noticed ; an idle ring wakes for nothing else
  oneSecond.tv_nsec = 0;

  while ( (!ringShouldStop) && (instance->stop_server==0) && (GLOBAL_KILL_SERVER_SWITCH==0) )
   {
     int handled = 0;
     int broken  = 0;

     unsigned int i=0;
     for (i=0; i<ringCount; i++)
      {
        struct UringRing * r = &ringAt[i];

        //Submitting and asking for completions are the same syscall , so a ring with either to do gets one
        //enter and no more. With DEFER_TASKRUN a ring that said it had something has to be entered before
        //its completion queue will show it , which is what needsFlush is.
        if ( (r->toSubmit>0) || (r->needsFlush) )
         {
           r->needsFlush = 0;
           if (!FlushRing(r)) { broken=1; break; }
         }

        handled += DrainRing(instance,r);
      }

     if (broken) { break; }

     //Serving one ring queues work on others , and more may have arrived while we were at it , so the rings
     //are gone round again before anything blocks.
     if (handled>0) { continue; }

     if (!WaitForAnyRing(&oneSecond)) { break; }
   }

  return 0;
}

int UringAPI_Start(struct AmmServer_Instance * instance,int serverSocket,unsigned int rings)
{
  if (instance==0)             { return 0; }
  if (instance->apiHandler==0) { return 0; }
  if (ringThreadStarted)       { return 0; }

  if (rings<1)               { rings=1; }
  if (rings>URING_MAX_RINGS) { rings=URING_MAX_RINGS; }

  listenSocket = serverSocket;
  ringCount    = rings;

  //The rings are set up by the thread that will submit to them , not here : IORING_SETUP_SINGLE_ISSUER binds
  //a ring to the task that creates it , and it is that restriction which lets the kernel skip locking.
  if (pthread_create(&ringThread,0,UringAPIThread,(void*)instance)!=0) { return 0; }
  ringThreadStarted=1;

  while (ringStartupResult==0) { usleep(1000); } //startup only : the thread is setting the rings up or failing

  if (ringStartupResult<0) { pthread_join(ringThread,0); ringThreadStarted=0; return 0; }

  return 1;
}

void UringAPI_Wait(struct AmmServer_Instance * instance)
{
  if (!ringThreadStarted) { return; }

  pthread_join(ringThread,0);
  ringThreadStarted=0;
}

void UringAPI_Stop(struct AmmServer_Instance * instance)
{
  if (!ringThreadStarted) { return; }

  ringShouldStop=1;
  pthread_join(ringThread,0);
  ringThreadStarted=0;
}

#endif // URING_API_SUPPORTED
