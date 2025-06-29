# C-Chat
Sistema de chatting cliente-servidor em C.

## Sumário

1. [Sobre](#sobre)
2. [Requisitos](#requisitos)
3. [Compilação](#compilação)
    1. [Servidor](#servidor)
    2. [Cliente](#Cliente)
        1. [CLI](#cli)
        2. [GUI](#gui)
4. [Execução](#execução)
5. [Compatibilidade](#compatibilidade)
6. [Créditos](#créditos)

## Sobre

O projeto de desenvolvimento __C-Chat__ constitui da elaboração de um sistema de chat cliente-servidor para a linguagem C, com comunicação simultânea de muitos usuários através de multiprocessamento em uma máquina servidora principal, que pode ser hospedada em serviços de nuvem.

Foi desenvolvido um servidor _backend_ em C (`server.c`) utilizando multi-processamento e _multi-threading_ para orquestrar a comunicação simultânea de muitos usuários em grupos distintos. Além disso, a aplicação utiliza uma interface em linha de comando (CLI) (`client.c`) para viabilizar a utilização da ferramenta pelo terminal do Linux.

Alternativamente, foi desenolvida uma interface gráfica em C++ utilizando a biblioteca [DearImGui](https://github.com/ocornut/imgui). Nesse cenário, o executável `client_gui` permite inicializar um ou vários _chats_, onde a aplicação `client.c` passa a atuar como mensageiro (_middleware_) entre a interface gráfica e o servidor _backend_.

## Requisitos

A interface gráfica implementa [OpenGL](https://www.opengl.org/) + [GLFW](https://www.glfw.org/). Dessa forma, pode ser necessário instalar o GLFW em algumas distribuições Linux:

`sudo pacman -S glfw` (Arch) ou `sudo apt install libglfw3` (Ubuntu).



Além disso, para compilar a aplicação, é necessário o CMake:

`sudo pacman -S gcc make` (Arch) ou `sudo apt install gcc make` (Ubuntu).

## Compilação

### Servidor

Execute... 

`gcc -c server_utils.c -o server_utils.o`

`gcc server.c client_utils.o server_utils.o -o server -fopenmp`

...em `./core/`.

### Cliente

#### CLI

Execute...

`gcc -c client_utils.c -o client_utils.o`

`gcc client.c client_utils.o -o client -fopenmp`

...em `./core/`.

#### GUI

Execute...

`make`

...em `./src/`.

> Certifique-se de ter o GLFW instalado.

## Execução

Altere `SERVER_IP` em `client.c` para o endereço IPv4 da máquina servidora.

Execute `./server <port>` para iniciar o servidor.

Execute `./client <username> <secret> <port>` para iniciar o cliente (quando CLI).

Altere a definição `PORT` (linha 55) no arquivo `./src/main.cpp` para a porta do servidor, depois refaça a passo da compilação (`make` em `./src/`)

Execute `./client_gui` para iniciar a interface gráfica do cliente.

## Compatibilidade

Disponível para sistemas Linux. Testado no Arch Linux x86_64.

## Créditos

Desenvolvido e mantido por Rafael Renó Corrêa (owariyumi@gmail.com).

Contribuidor: Lucas Ferreira Alves (lucas.3451@hotmail.com).

Todos os direitos reservados.

A alteração e redistribuição do código sem permissão expressa e por escrito do autor é proíbida.
