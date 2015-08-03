#include <stdio.h>
#include <stdlib.h>
#include "SDL_thread.h"
#include "networking.h"

int conectar(Peer** peer);
int menu(Peer** peer);
int input();

int done = 0;

int main(int argc, char *argv[])
{
    Peer* peer;
    SDL_Thread *input_thread;

    if(!menu(&peer))
        exit(1);

    input_thread = SDL_CreateThread(input, "Input", (void*)peer);

    if(!input_thread)
    {
        printf("\nSDL_CreateThread failed: %s\n", SDL_GetError());
        exit(1);
    }

    while(!done)
    {
        if(!checkPackets(peer))
            done = 1;

        if(hasMessage(peer))
        {
            switch(getMessageType(peer))
            {
            case 0:
                break;
            case 1:
                printf("%s\n", getMessage(peer));
                break;
            case 2:
                printf("[PM] %s\n", getMessage(peer));
                break;
            case 3:
                break;
            default:
                printf("mensagem desconhecida: %d\n", getMessageType(peer));
            }

            flushClientState(peer);
        }
    }

    SDL_WaitThread(input_thread, NULL);
    disconnect(peer);

    return 0;
}

int input(void* peer)
{
    while(!done)
    {
        int read = 512;
        char buffer[read];
        char* tok;

        rewind(stdin);
        if(fgets(buffer, read, stdin))
        {
			buffer[strlen(buffer)-1] = 0;

			if(buffer[0] == '/')
            {
                tok = strtok(buffer+1, " ");

                if(!strcmp(tok, "quit"))
                {
                    done = 1;
                }
                else if(!strcmp(tok, "pm"))
                {
                    tok = strtok(NULL, " ");
                    char* msg = tok+strlen(tok)+1;
                    sendMessageTo((Peer*)peer, tok, msg, strlen(msg));
                }
                else
                    printf("Comando desconhecido.\n");
            }
            else
            {
                broadcastMessage((Peer*)peer, buffer, strlen(buffer));
                memset((void*) buffer, 0, read);
            }
        }
    }

    return 1;
}

int conectar(Peer** peer)
{
    char ip[16];
    int porta;
    char nome[16];

    printf("Digite o ip: ");
    rewind(stdin);
    scanf("%s", ip);
    printf("Digite a porta: ");
    rewind(stdin);
    scanf("%d", &porta);
    printf("Digite seu nickname: ");
    rewind(stdin);
    fgets(nome, 16, stdin);
    nome[strlen(nome)-1] = 0;
	
	printf("\nConectando...\n");
    *peer = startup(ip, porta);

    if(*peer)
    {
		printf("k.\n");
        changeName(*peer, nome, strlen(nome));
    }

    return (*peer) != NULL;
}

int menu(Peer** peer)
{
    while(1)
    {
        int op;
        printf("\n1 - Conectar\n0 - sair\n- ");
        rewind(stdin);
        scanf("%d", &op);

        switch(op)
        {
        case 1:
            if(conectar(peer))
			{
				printf("Conectado.\n");
                return 1;
			} 
            break;
        case 0:
            return 0;
            break;
        default:
            printf("Opcao invalida. tente novamente\n");
        }
    }
	printf("Oacacacacac\n");
}
