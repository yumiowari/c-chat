#ifndef CHATUI_HPP
#define CHATUI_HPP

#include <vector>
#include <string>
#include <cstring>
#include <pthread.h>

class ChatUI{
    private:
        // vetor de mensagens registradas na interface
        std::vector<std::string> messages;
        bool scrollToBottom;
        std::string title;     // título da janela do chat
        std::string inputID;   // identificador da janela do chat
        std::string msgBuffer; // texto digitado
        std::string username;
        long roomID;  // i. e. "secret"
        int ui_fd;    // file descriptor do soquete de comm.
        int modifier; // modificador para porta do mensageiro
    public:
        bool isOpen = true;

        // construtor parametrizado
        ChatUI(std::string &n, long s, int m);

        // método para adicionar uma nova mensagem
        void addMsg(const std::string &msg);

        // módulo para renderizar o conteúdo da janela
        void Render();
};

#endif // CHATUI_HPP