#ifndef COMM_UTILS_H
#define COMM_UTILS_H

void wrap(char *buffer, char *username, long secret, char *message);
// função p/ encapsular a mensagem

void unwrap(char *buffer, char *username, long *secret, char *message);
// função p/ desencapsular a mensagem

#endif // COMM_UTILS_H