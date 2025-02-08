# C Chat

Sistema de chatting cliente-servidor em C.

## Compilação:

`gcc client2.c -o client2 && gcc server2.c -o server2`

## Uso:

No Terminal do Linux em um computador (servidor) execute:

`./server2 <porta>`
> A porta deve ser um número inteiro entre 0 - 65535.

Em outro computador (cliente) altere a macro `SERVER_IP` na linha 34 para o IPv4 do computador utilizado no passo anterior e execute:

`./client2 <porta> <nome de usuário>`
> A porta DEVE ser a mesma do servidor e o nome de usuário NÃO pode exceder 15 caracteres.

Digite alguma coisa no Terminal do cliente e veja a mágica acontecer 😘

---

Feito com amor por Yumiowari 🪶