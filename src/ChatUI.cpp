#include <vector>
#include <string>
#include <imgui.h>
#include <iostream>

#include "ChatUI.hpp"
#include "gui_utils.hpp"

using namespace std;

ChatUI::ChatUI(string &n, long s, int m, int p) : scrollToBottom(false), username(n), roomID(s), modifier(m), port(p)
{
    title = to_string(roomID);
    inputID = "##input" + title;
    // *Feature*: Não é possível existir duas instâncias de chat do mesmo grupo no mesmo computador.

    // estabele conexão com a interface
    ui_fd = setupComm(port + modifier);
    // a porta do mensageiro é a porta do servidor + modificador (incrementado para cada novo chatroom aberto)

    inputBuffer[0] = '\0';
}

void ChatUI::addMsg(const string &msg){
    this->messages.push_back(msg);
    scrollToBottom = true;
}

// renderiza a janela do chat (ImGui)
void ChatUI::Render(){
    // limita o tamanho inicial da janela
    ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_Once);
    if(!ImGui::Begin(title.c_str(), &isOpen)){ // isOpen indica se a janela está aberta ou fechada
        ImGui::End();

        return;
    }

    // recebe a mensagem do cliente
    int flag = -1;
    message_t message = recvMsgr(ui_fd, flag);
    if(flag == 0){
    // mensagem válida

        string msg = string(message.username) + ": " + string(message.buffer);
        addMsg(msg);
    }else if(flag == 1){
    // conexão fechada

        isOpen = false;
    }else if(flag == 2){
    // sem dados, não faz nada

    }else if(flag == 3){
    // caso de erro

        isOpen = false;
    }

    // área de mensagens (rolável)
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true, ImGuiWindowFlags_HorizontalScrollbar);
    for(const string& msg : messages){
        ImGui::TextWrapped("%s", msg.c_str());
    }

    if(scrollToBottom){ // "puxa" o scroll para baixo quando receber uma mensagem
        ImGui::SetScrollHereY(1.0f);
        scrollToBottom = false;
    }

    ImGui::EndChild();

    bool sendNow = false;
    if(ImGui::InputText(inputID.c_str(), inputBuffer, sizeof(inputBuffer), ImGuiInputTextFlags_EnterReturnsTrue)){
        if(inputBuffer[0] != '\0'){
        // envia a mensagem ao mensageiro quando apertar ENTER

            sendNow = true;
        }
    }

    ImGui::SameLine();

    if(ImGui::Button("Enviar") || sendNow){
        if(inputBuffer[0] != '\0'){
            message_t letter;
            resetMsg(&letter);

            // copia strings para arrays fixos
            strncpy(letter.username, username.c_str(), sizeof(letter.username) - 1);
            strncpy(letter.buffer, inputBuffer, sizeof(letter.buffer) - 1);

            letter.secret = roomID;
            letter.counter = 1; // conceitualmente, o remetente já leu a mensagem

            isOpen = sendMsgr(letter, ui_fd); // envia a mensagem para o mensageiro
            // se o retorno for false, o servidor caiu e a janela termina.

            string msg = string(letter.username) + ": " + string(letter.buffer);
            addMsg(msg);

            inputBuffer[0] = '\0'; // limpa o input
        }
    }

    ImGui::End();
}
