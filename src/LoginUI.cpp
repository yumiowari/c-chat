#include <imgui.h>
#include <string>
#include <stdio.h>

#include "LoginUI.hpp"
#include "ChatUI.hpp"

// construtor padrão
LoginUI::LoginUI() {
    usernameBuffer[0] = '\0';
    secretBuffer[0] = '\0';
    sendUsernameNow = false;
    sendSecretNow = false;
}

// módulo para renderizar o conteúdo da janela
void LoginUI::Render(std::vector<ChatUI*> &chats){
    // limita o tamanho inicial da janela
    ImGui::SetNextWindowSize(ImVec2(150, 150), ImGuiCond_Once);
    if(!ImGui::Begin(std::string("Login").c_str(), &isOpen)){ // isOpen indica se a janela está aberta ou fechada
        ImGui::End();

        return;
    }

    ImGui::TextWrapped("Nome de Usuário:");
    if(ImGui::InputText(std::string("##username").c_str(), usernameBuffer, sizeof(usernameBuffer), ImGuiInputTextFlags_EnterReturnsTrue)){
        if(usernameBuffer[0] != '\0'){
            sendUsernameNow = true;
        }
    }

    ImGui::TextWrapped("Segredo do Grupo:");
    if(ImGui::InputText(std::string("##secret").c_str(), secretBuffer, sizeof(secretBuffer), ImGuiInputTextFlags_EnterReturnsTrue)){
        if(secretBuffer[0] != '\0'){
            sendSecretNow = true;
        }
    }

    if(ImGui::Button("Enviar")){
        if(sendUsernameNow && sendSecretNow){
            std::string username = usernameBuffer;
            long secret = atof(secretBuffer);

            // instancia uma novo chatroom
            chats.push_back(new ChatUI(username, secret));

            // limpa o input
            usernameBuffer[0] = '\0';
            secretBuffer[0] = '\0';

            // limpa as flags
            sendUsernameNow = false;
            sendSecretNow = false;
        }
    }

    ImGui::End();
}