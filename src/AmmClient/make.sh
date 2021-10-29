#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"

gcc -o AmmClient AmmClient.c network.c protocol.c tools.c test.c

gcc curltest.c -o curltest -lcurl


exit 0
