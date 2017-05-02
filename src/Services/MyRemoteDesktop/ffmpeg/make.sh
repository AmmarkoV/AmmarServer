#!/bin/bash

gcc -o tutorial01 tutorial01.c -lswscale -lswresample -lavutil -lavformat -lavcodec -lz -lavutil -lm
gcc -o tutorial03 tutorial03.c -lavutil -lavformat -lavcodec -lswscale -lz -lm `sdl-config --cflags --libs`

exit 0
