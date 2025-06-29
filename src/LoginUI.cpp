#include <imgui.h>
#include <string>
#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "LoginUI.hpp"
#include "ChatUI.hpp"

using namespace std;

namespace fs = filesystem;

// construtor padrão
LoginUI::LoginUI() {
    usernameBuffer[0] = '\0';
    secretBuffer[0] = '\0';
    sendUsernameNow = false;
    sendSecretNow = false;
}

// módulo para iniciar o mensageiro em paralelo
void LoginUI::launchMsgr(const string &username, const string &secret, const int &modifier, int port){
    pid_t pid = fork();

    if (pid == 0) {
    // processo filho

        string path = "../core/client";
        string serverPort = to_string(port);
        string msgrPort = to_string(port + modifier);

        // monta os argumentos
        vector<char*> args;
        args.push_back(const_cast<char*>(path.c_str()));
        args.push_back(const_cast<char*>(username.c_str()));
        args.push_back(const_cast<char*>(secret.c_str()));
        args.push_back(const_cast<char*>(serverPort.c_str()));
        args.push_back(const_cast<char*>("-gui"));
        args.push_back(const_cast<char*>(msgrPort.c_str()));

        args.push_back(nullptr); // termina com NULL

        // executa o mensageiro
        execv(path.c_str(), args.data());
    } else if (pid > 0) {
    // processo pai

    } else {
    // caso de erro

        cerr << "Falha na criação do processo filho." << endl;
    }
}

// módulo para renderizar o conteúdo da janela
void LoginUI::Render(vector<ChatUI*> &chats, int &modifier, int port){
    // limita o tamanho inicial da janela
    ImGui::SetNextWindowSize(ImVec2(150, 150), ImGuiCond_Once);
    if(!ImGui::Begin(string("Login").c_str(), &isOpen)){ // isOpen indica se a janela está aberta ou fechada
        ImGui::End();

        return;
    }

    ImGui::TextWrapped("Nome de Usuário:");
    if(ImGui::InputText(string("##username").c_str(), usernameBuffer, sizeof(usernameBuffer), ImGuiInputTextFlags_EnterReturnsTrue)){
        if(usernameBuffer[0] != '\0'){
            sendUsernameNow = true;
        }
    }

    ImGui::TextWrapped("Segredo do Grupo:");
    if(ImGui::InputText(string("##secret").c_str(), secretBuffer, sizeof(secretBuffer), ImGuiInputTextFlags_EnterReturnsTrue)){
        if(secretBuffer[0] != '\0'){
            sendSecretNow = true;
        }
    }
    
    if(
       ImGui::Button("Enviar") ||
       (sendUsernameNow && sendSecretNow)
      )
    {
        if(
           usernameBuffer[0] != '\0' &&
           secretBuffer[0] != '\0'
          )
        {
            string username = usernameBuffer;
            long secret = atof(secretBuffer);

            // inicia o mensageiro
            launchMsgr(username, to_string(secret), modifier, port);

            usleep(250000); // espera 250ms
            
            // instancia uma novo chatroom
            chats.push_back(new ChatUI(username, secret, modifier, port));

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