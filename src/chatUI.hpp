#ifndef CHAT_UI_HPP
#define CHAT_UI_HPP

#include <vector>
#include <string>
#include <cstring>
#include <pthread.h>

class chatUI{
    private:
        // vetor de mensagens registradas na interface
        std::vector<std::string> messages;
        bool scrollToBottom;
        std::string title;         // título da janela do chat
        std::string inputID;       // identificador da janela do chat
        std::string msgBuffer;     // texto digitado
        std::string username;
        long roomID;   // i. e. "secret"
        int ui_fd;     // file descriptor do soquete de comm.
        pthread_t tid; // thread id
    public:
        bool isOpen = true;

        // construtor parametrizado
        chatUI(std::string &n, long s);

        // método para adicionar uma nova mensagem
        void addMsg(const std::string &msg);

        // módulo para renderizar o conteúdo da janela
        void Render();

        // thread para receber a mensagem do mensageiro
        static void *recvThread(void *arg);

        // Getter para msgBuffer
        const std::string& getMsgBuffer() const {
            return msgBuffer;
        }

        // Getter estilo C (útil para funções C)
        const char* getMsgBufferCStr() const {
            return msgBuffer.c_str();
        }

        // Setter para msgBuffer
        void setMsgBuffer(const std::string& msg) {
            msgBuffer = msg;
        }

        // Setter com C-string (opcional)
        void setMsgBuffer(const char* msg) {
            msgBuffer = msg ? std::string(msg) : "";
        }

        const std::string& getUsername() const {
            return username;
        }

        const char* getUsernameCStr() const {
            return username.c_str();
        }

        void setUsername(const std::string& name) {
            username = name;
        }

        void setUsername(const char* name) {
            username = name ? std::string(name) : "";
        }
};

#endif // CHAT_UI_HPP