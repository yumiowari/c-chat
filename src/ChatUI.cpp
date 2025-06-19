#include <vector>
#include <string>
#include <imgui.h>

#include "ChatUI.hpp"
#include "gui_utils.hpp"

ChatUI::ChatUI(std::string &n, long secret) : scrollToBottom(false), username(n), roomID(secret)
{
    title = std::to_string(roomID);
    inputID = "##input" + title;
    // *Feature*: Não possível existir duas instâncias de chat do mesmo grupo no mesmo computador.

    // estabele conexão com a interface
    ui_fd = setupComm(8080 + 1);
}

void ChatUI::addMsg(const std::string &msg){
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
        std::string msg = std::string(message.username) + " > " + std::string(message.buffer);
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

    // área de mensagens com scroll
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
    for(const std::string& msg : messages){
        ImGui::TextWrapped("%s", msg.c_str());
    }

    if(scrollToBottom){
        ImGui::SetScrollHereY(1.0f);
        scrollToBottom = false;
    }

    ImGui::EndChild();

    // campo de entrada de texto
    char inputBuffer[1024] = {};
    bool sendNow = false;

    if(ImGui::InputText(inputID.c_str(), inputBuffer, sizeof(inputBuffer), ImGuiInputTextFlags_EnterReturnsTrue)){
        if(inputBuffer[0] != '\0'){
            sendNow = true;
        }
    }

    ImGui::SameLine();

    if(ImGui::Button("Enviar")){
        if(inputBuffer[0] != '\0'){
            sendNow = true;
        }
    }

    if(sendNow){
        message_t letter;
        resetMsg(&letter);

        // copia strings para arrays fixos
        strncpy(letter.username, username.c_str(), sizeof(letter.username) - 1);
        strncpy(letter.buffer, inputBuffer, sizeof(letter.buffer) - 1);

        letter.secret = roomID;
        letter.counter = 1;

        isOpen = sendMsgr(letter, ui_fd); // envia a mensagem para o mensageiro
        // se o retorno for false, o servidor caiu e janela termina.

        std::string msg = std::string(letter.username) + " > " + std::string(letter.buffer);
        addMsg(msg);

        inputBuffer[0] = '\0'; // limpa o input
    }

    ImGui::End();
}
