#!/bin/bash

./AmmMessages -msg samples/msg/person.msg

gcc person.c -o person

exit 0
