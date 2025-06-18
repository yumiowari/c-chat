#!/bin/bash

echo "Compilando códigos-fonte..."

gcc -c client_utils.c -o client_utils.o
gcc -c server_utils.c -o server_utils.o
gcc client.c client_utils.o -o client -fopenmp
gcc server.c client_utils.o server_utils.o -o server -fopenmp

echo "Procedimento concluído."