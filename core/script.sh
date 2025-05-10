#!/bin/bash

gcc -c client_utils.c -o client_utils.o

gcc client.c client_utils.o -o client -fopenmp && gcc server.c client_utils.o -o server -fopenmp