#include <string.h>

#include "client_utils.h"

/*
 *  Funções
 */
long hashing(char username[16]){
// função p/ converter o nome de usuário para um código hash

    long secret = 0;
    int p = 1;

    for(int i = 0; i < strlen(username); i++){
        if(username[i] >= 'A' && username[i] <= 'Z'){
            secret += (username[i] - 'A') * p;
        }else if(username[i] >= 'a' && username[i] <= 'z'){
            secret += (username[i] - 'a') * p;
        }

        p *= 10;
    }

    return secret;
}