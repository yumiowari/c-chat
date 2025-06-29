#ifndef LOGINUI_HPP
#define LOGINUI_HPP

#include <string>

using namespace std;

#include "ChatUI.hpp"

class LoginUI{
    private:
        char usernameBuffer[32];
        char secretBuffer[32];
        bool sendUsernameNow;
        bool sendSecretNow;

        // módulo para iniciar o mensageiro em paralelo
        void launchMsgr(const string &username, const string &secret, const int &modifier, int port);
    public:
        bool isOpen = true;

        // construtor padrão
        LoginUI();

        // módulo para renderizar o conteúdo da janela
        void Render(vector<ChatUI*> &chats, int &modifier, int port);
};

#endif // LOGINUI_HPP