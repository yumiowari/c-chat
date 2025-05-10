# C-Chat
Sistema de chatting cliente-servidor em C.

## Sumário

1. [Sobre](#sobre)
2. [Compilação](#compilação)
3. [Execução](#execução)
4. [Compatibilidade](#compatibilidade)
4. [Créditos](#créditos)

## Sobre

O projeto de desenvolvimento __C-Chat__ constitui da elaboração de um sistema de chatting cliente-servidor para a linguagem C, com comunicação simultânea de muitos usuários e orquestração por multiprocessamento em uma máquina servidora principal, que pode ser hospedada em serviços de nuvem.

## Compilação

`gcc -c client_utils.c -o client_utils.o`

`gcc client.c client_utils.o -o client -fopenmp && gcc server.c client_utils.o -o server -fopenmp`

## Execução

Altere `SERVER_IP` em `client.c` para o endereço IPv4 da máquina servidora.

Libere a porta 8080 no Firewall (ou a definida por `PORT` em `server.c`) para entrada/saída de dados na máquina servidora.

Execute `./server` para iniciar o servidor.

Execute `./client <username>` para iniciar o cliente.

## Compatibilidade

Disponível para sistemas Linux. Testado no Ubuntu 25.04.02.

## Créditos

Desenvolvido e mantido por Rafael Renó Corrêa (owariyumi@gmail.com). 

Todos os direitos reservados.

A alteração e redistribuição do código sem permissão expressa e por escrito do autor é proíbida.
