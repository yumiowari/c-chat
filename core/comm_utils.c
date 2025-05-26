#include <stdlib.h> // strtol()
#include <stdio.h>  // sprintf()
#include <string.h> // strcpy(), strcat()

#include "comm_utils.h"

void wrap(char *buffer, char *username, long secret, char *message){
// função p/ encapsular a mensagem

    sprintf(buffer, "%s;%d;%s;", username, secret, message);
}

void unwrap(char *buffer, char *username, long *secret, char *message){
// função p/ desencapsular a mensagem

    char *token = strtok(buffer, ";");
    if(token)strcpy(username, token);

    token = strtok(NULL, ";");
    if(token)*secret = (long int) strtol(token, NULL, 10);

    token = strtok(NULL, ";");
    if(token)strcpy(message, token);
}