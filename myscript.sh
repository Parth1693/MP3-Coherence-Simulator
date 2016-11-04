#!/bin/bash
# My first script

echo "Hello World"
make clean
make
###################################################
#Varying cache size
#####################################################
./smp_cache 262144 524288 4 8 64 trace/canneal.04t.longTrace > mp3.txt
./smp_cache 524288 1048576 4 8 64 trace/canneal.04t.longTrace >> mp3.txt
./smp_cache 1048576 2097152 4 8 64 trace/canneal.04t.longTrace >> mp3.txt
./smp_cache 2097152 4194304 4 8 64 trace/canneal.04t.longTrace >> mp3.txt
####################################################
#Varying block size
###################################################
./smp_cache 2097152 4194304 4 8 32 trace/canneal.04t.longTrace >> mp3.txt
./smp_cache 2097152 4194304 4 8 64 trace/canneal.04t.longTrace >> mp3.txt
./smp_cache 2097152 4194304 4 8 128 trace/canneal.04t.longTrace >> mp3.txt
