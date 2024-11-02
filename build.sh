#!/bin/bash

rm -f one_byte_inc.o libone_byte_inc.a libone_byte_inc.so example

gcc -c -O2 -o ./one_byte_inc.o -fPIC ./one_byte_inc.c
ar rcs ./libone_byte_inc.a ./one_byte_inc.o
gcc -shared -o ./libone_byte_inc.so ./one_byte_inc.o

gcc -L. ./example.c -o example -Wl,-Bstatic -lone_byte_inc -Wl,-Bdynamic
