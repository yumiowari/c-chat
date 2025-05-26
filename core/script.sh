#!/bin/bash

echo "Compilando código-fonte..."

gcc -c client_utils.c -o client_utils.o
gcc -c comm_utils.c -o comm_utils.o
gcc client.c client_utils.o comm_utils.o -o client -fopenmp
gcc server.c client_utils.o comm_utils.o -o server -fopenmp