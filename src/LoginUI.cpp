#include <imgui.h>
#include <string>
#include <stdio.h>
#include <cstdlib>
#include <filesystem>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

namespace fs = std::filesystem;

#include "LoginUI.hpp"
#include "ChatUI.hpp"

// construtor padrão
LoginUI::LoginUI() {
    usernameBuffer[0] = '\0';
    secretBuffer[0] = '\0';
    sendUsernameNow = false;
    sendSecretNow = false;
}

// módulo para iniciar o mensageiro em paralelo
void LoginUI::launch_msgr(const std::string &username, const std::string &secret, const int &modifier){
    pid_t pid = fork();

    if (pid == 0) {
    // processo filho

        std::string path = "../core/client";

        int msgrPort = 8080 + modifier;

        // monta os argumentos
        std::vector<char*> args;
        args.push_back(const_cast<char*>(path.c_str()));
        args.push_back(const_cast<char*>(username.c_str()));
        args.push_back(const_cast<char*>(secret.c_str()));
        args.push_back(const_cast<char*>("8080"));
        args.push_back(const_cast<char*>("-gui"));
        args.push_back(const_cast<char*>(std::to_string(msgrPort).c_str()));

        args.push_back(nullptr); // termina com NULL

        // executa o mensageiro
        execv(path.c_str(), args.data());
    } else if (pid > 0) {
    // processo pai
    } else {
    // caso de erro
    }
}

// módulo para renderizar o conteúdo da janela
void LoginUI::Render(std::vector<ChatUI*> &chats, int &modifier){
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

            // inicia o mensageiro
            launch_msgr(username, std::to_string(secret), modifier);

            sleep(1); // espera 1 segundo
            
            // instancia uma novo chatroom
            chats.push_back(new ChatUI(username, secret, modifier));

            modifier++;

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