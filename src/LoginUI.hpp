#ifndef LOGINUI_HPP
#define LOGINUI_HPP

#include <string>

#include "ChatUI.hpp"

class LoginUI{
    private:
        char usernameBuffer[32];
        char secretBuffer[32];
        bool sendUsernameNow;
        bool sendSecretNow;

        // módulo para iniciar o mensageiro em paralelo
        void launchMsgr(const std::string &username, const std::string &secret, const int &modifier);
    public:
        bool isOpen = true;

        // construtor padrão
        LoginUI();

        // módulo para renderizar o conteúdo da janela
        void Render(std::vector<ChatUI*> &chats, int &modifier);
};

#endif // LOGINUI_HPP