#ifndef CHATUI_HPP
#define CHATUI_HPP

#include <vector>
#include <string>
#include <cstring>
#include <pthread.h>

using namespace std;

class ChatUI{
    private:
        // vetor de mensagens registradas na interface
        vector<string> messages;
        bool scrollToBottom;
        string title;     // título da janela do chat
        string inputID;   // identificador da janela do chat
        string msgBuffer; // texto digitado
        string username;
        long roomID;  // i. e. "secret"
        int ui_fd;    // file descriptor do soquete de comm.
        int modifier; // modificador para porta do mensageiro
    public:
        bool isOpen = true;

        // construtor parametrizado
        ChatUI(string &n, long s, int m);

        // método para adicionar uma nova mensagem
        void addMsg(const string &msg);

        // módulo para renderizar o conteúdo da janela
        void Render();
};

#endif // CHATUI_HPP