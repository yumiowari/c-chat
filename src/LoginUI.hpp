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
    public:
        bool isOpen = true;

        // construtor padrão
        LoginUI();

        // módulo para renderizar o conteúdo da janela
        void Render(std::vector<ChatUI*> &chats);
};

#endif // LOGINUI_HPP