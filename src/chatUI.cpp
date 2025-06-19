#include <vector>
#include <string>
#include <imgui.h>
#include <pthread.h>

#include "chatUI.hpp"
#include "gui_utils.hpp"

chatUI::chatUI(std::string &n, long secret) : scrollToBottom(false), username(n), roomID(secret)
{
    title = std::to_string(roomID);
    inputID = "##input" + title;
    // *Feature*: Não possível existir duas instâncias de chat do mesmo grupo no mesmo computador.

    // estabele conexão com a interface
    ui_fd = setupComm(8080 + 1);
}

void chatUI::addMsg(const std::string &msg){
    this->messages.push_back(msg);
    scrollToBottom = true;
}

// renderiza a janela do chat (ImGui)
void chatUI::Render(){
    // recebe a mensagem do cliente
    message_t message = recvMsgr(ui_fd);
    if(message.counter != -1){
        std::string msg = std::string(message.username) + " > " + std::string(message.buffer);
        addMsg(msg);
    }

    // limita o tamanho inicial da janela
    ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_Once);
    ImGui::Begin(title.c_str(), &isOpen); // isOpen indica se a janela está aberta ou fechada

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
    static char inputBuffer[1024] = {};
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

        inputBuffer[0] = '\0';   // limpa o input
    }

    ImGui::End();
}
