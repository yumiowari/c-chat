#ifndef GUI_UTILS_H
#define GUI_UTILS_H

/*
 *  Estruturas
 */
struct message {
    char username[16];
    long secret;
    char buffer[1024];
    int counter; // quando -1, indica que a mensagem é inválida
}typedef(message_t);

struct client{
    char username[16]; // 15 char + '\0'
    long secret;
}typedef(client_t);

int setupComm(int port);
// função para estabelecer conexão com o mensageiro

bool sendMsgr(message_t buffer, int ui_fd);
// função para enviar a mensagem ao mensageiro

message_t recvMsgr(int ui_fd);
// função para receber a mensagem do mensageiro

#endif // GUI_UTILS_H