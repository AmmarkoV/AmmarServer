#!/bin/bash

TOTAL_CLIENTS=`cat ammarServerLog.txt  | grep "Server Thread : Accepted new client" | wc -l`
FRESH_THREADS=`cat ammarServerLog.txt  | grep "Spawning a new thread" | wc -l`
PRESPAWNED_THREADS=`cat ammarServerLog.txt  | grep "Decided to use prespawned thread" | wc -l`

PRESPAWNED_TRUNCS=`cat ammarServerLog.txt  | grep "Prespawn jobs counters truncated" | wc -l`
PRESPAWNED_BUSY=`cat ammarServerLog.txt  | grep "All prespawned threads are busy" | wc -l`
PRESPAWNED_SINGLE_BUSY=`cat ammarServerLog.txt  | grep "Seems that the prespawned thread is still busy" | wc -l`




echo "Total Clients $TOTAL_CLIENTS"
echo "Fresh Threads : $FRESH_THREADS"
echo "Prespawned Threads : $PRESPAWNED_THREADS"
echo "Prespawned Truncs Threads : $PRESPAWNED_TRUNCS"
echo "Prespawned All Busy Threads : $PRESPAWNED_BUSY"
echo "Prespawned Single Busy Threads : $PRESPAWNED_SINGLE_BUSY"


exit 0
