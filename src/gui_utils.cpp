/*
 *  Bibliotecas
 */
#include <sys/types.h>  // pid_t
#include <sys/ipc.h>    // key_t
#include <arpa/inet.h>  // inet_pton(), htons(), etc.
#include <sys/socket.h> // socket(), connect(), bind(), listen(), accept()
#include <netinet/in.h> // struct sockaddr_in
#include <string.h>     // strcpy()
#include <stdbool.h>    // bool type
#include <fcntl.h>
#include <errno.h>

#include "gui_utils.hpp"

int setupComm(int port){
// função para estabelecer conexão com o mensageiro

    struct sockaddr_in client_addr;
    struct sockaddr *client_addr_ptr
    = (struct sockaddr*)&client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // cria o soquete de interface
    int ui_fd = socket(AF_INET,     // com protocolo IPv4 e
                       SOCK_STREAM, // baseado em conexão
                       0);

    // define o endereço do cliente
    client_addr.sin_family = AF_INET; // para protocolo IPv4,
    inet_pton(AF_INET,
              "127.0.0.1",            // no endereço IP localhost
              &client_addr.sin_addr);
    client_addr.sin_port = htons(port);

    connect(ui_fd,
            client_addr_ptr,
            client_addr_len);

    // define o socket como não bloqueante
    int flags = fcntl(ui_fd, F_GETFL, 0);
    fcntl(ui_fd, F_SETFL, flags | O_NONBLOCK);

    return ui_fd;
}

bool sendMsgr(message_t message, int ui_fd){
// função para enviar a mensagem ao mensageiro

    ssize_t sent = send(ui_fd,
                        &message,
                        sizeof(message_t),
                        0);
    if(sent < 0)
        return false;
    else
        return true;
}

void resetMsg(message_t *msg){
// função p/ resetar os atributos da mensagem

    memset(msg->username, 0, 16);
    memset(msg->buffer, 0, 1024);
    msg->secret = -1;
    msg->counter = -1;
}

message_t recvMsgr(int ui_fd, int &flag){
// função para receber a mensagem do mensageiro

    message_t message;
    resetMsg(&message);

    ssize_t rcvd = recv(ui_fd,
                        &message,
                        sizeof(message_t),
                        0);
    if(rcvd > 0){
    // mensagem válida
    
        flag = 0;
    }else if(rcvd == 0){
    // conexão fechada

        flag = 1;
    }else if(errno == EAGAIN || errno == EWOULDBLOCK){
    // sem dados, não faz nada

        flag = 2;
    }else{ // rcvd < 0
    // erro real

        flag = 3;
    }
    
    return message;
}