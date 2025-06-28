#ifndef GUI_UTILS_HPP
#define GUI_UTILS_HPP

/*
 *  Estruturas
 */
struct message{
    char username[16];
    long secret;
    char buffer[1024];
    int counter; // quando -1, indica que a mensagem é inválida
}typedef(message_t);

int setupComm(int port);
// função para estabelecer conexão com o mensageiro

bool sendMsgr(message_t buffer, int ui_fd);
// função para enviar a mensagem ao mensageiro

void resetMsg(message_t *msg);
// função p/ resetar os atributos da mensagem

message_t recvMsgr(int ui_fd, int &flag);
// função para receber a mensagem do mensageiro

#endif // GUI_UTILS_HPP