#include "networking.h"
#define HS 3 //header size

SOCKET create_socket(void);
int bind_socket(SOCKET* sock, int port);
int sendPacket(SOCKET* sock, char* buffer, int buff_size);
int getClientNum(Peer* peer, char* name);
int sendHeader(SOCKET* sock, int param1, int param2);

Peer* startup(int port)
{
    Peer* peer = (Peer*) malloc(sizeof(Peer));
    if(!peer)
        return NULL;

    peer->sock = create_socket();
    peer->num_cls = 0;

    if(peer->sock == INVALID_SOCKET)
    {
        printf("Failed. Error Code : %d", WSAGetLastError());
        return NULL;
    }

    if(bind_socket(&peer->sock, port) == SOCKET_ERROR)
    {
        printf("Failed. Error Code : %d", WSAGetLastError());
        return NULL;
    }

    listen(peer->sock, MAX_CLIENT);

    return peer;
}

int checkPackets(Peer* peer)
{
    if(peer == NULL)
    {
        perror("erro");
        return 0;
    }

    if(peer->num_cls == 0)
        return 0;

    int i, max;
    for(i = 0, max = peer->num_cls; i < max; i++)
    {
        if((peer->cl_sock[i] == INVALID_SOCKET) || (peer->packet[i].state == FULL))
            continue;

        char buffer[2048] = {0};
        int dataLen = 0;
        SOCKET sock = peer->cl_sock[i];

        if((dataLen = recv(sock, buffer, 2048, 0)) > 0)
        {
            if(peer->packet[i].state == NONE)
            {
                if(peer->packet[i].data)
                    free((void*) peer->packet[i].data);

                peer->packet[i].message = buffer[0];
                if(buffer[0] > 3) return 0;
                peer->packet[i].length = ntohs(*(u_short*)(buffer+1));
                peer->packet[i].data = (uint8_t*) calloc(peer->packet[i].length+1, sizeof(uint8_t));

                memcpy(peer->packet[i].data, buffer+3, strlen(buffer+3));

                if(strlen((const char*)peer->packet[i].data) == peer->packet[i].length)
                    peer->packet[i].state = FULL;
                else
                {
                    peer->packet[i].state = HALF;
                    peer->packet[i].data[strlen(buffer+3)] = 0;
                }

            }

            else if(peer->packet[i].state == HALF)
            {
                memcpy(peer->packet[i].data+strlen((const char*)peer->packet[i].data), buffer, strlen(buffer));

                if(strlen((const char*)peer->packet[i].data) == peer->packet[i].length)
                    peer->packet[i].state = FULL;
                else
                    peer->packet[i].data[strlen(buffer)] = 0;
            }

            if(buffer[0] > 3)
                buffer[1]=0xff, buffer[2]=0xff;
            if(peer->packet[i].name)
                printf("%s: 0x0%c/%d/%s | %d bytes  c%d\n", peer->packet[i].name, peer->packet[i].message+48, peer->packet[i].length, peer->packet[i].data, strlen(buffer), i);
            else
                printf("????: 0x0%c/%d/%s | %d bytes  c%d\n", peer->packet[i].message+48, peer->packet[i].length, peer->packet[i].data, strlen(buffer), i);
        }

        else if(WSAGetLastError() != WSAETIMEDOUT)
        {
            printf("%s deconectou.\n", peer->packet[i].name);
            closesocket(sock);
            peer->cl_sock[i] = peer->cl_sock[peer->num_cls-1];
            peer->cl_sock[peer->num_cls-1] = INVALID_SOCKET;
            peer->packet[i] = peer->packet[peer->num_cls-1];
            peer->num_cls--;

            if(peer->packet[i].data)
                free((void*) peer->packet[i].data);
            if(peer->packet[i].name)
                free((void*) peer->packet[i].name);
        }
    }

    return 1;
}

int wait_connection(Peer* peer)
{
    if(peer == NULL)
    {
        perror("erro");
        return -1;
    }

    if(peer->num_cls == MAX_CLIENT)
    {
        printf("O servidor esta cheio.\n");
        return 0;
    }

    SOCKET new_sock = INVALID_SOCKET;

    if(peer->sock != INVALID_SOCKET)
    {
        int clilen;
        struct sockaddr_in cli_addr;

        listen(peer->sock, MAX_CLIENT);
        clilen = sizeof(cli_addr);

		struct timeval tv = {0};
		tv.tv_usec = 100;
		FD_SET reader = {0};
		FD_SET(peer->sock, &reader);

		if(select(0, &reader, NULL, NULL, &tv) > 0)
		{
			new_sock = accept(peer->sock, (struct sockaddr *) &cli_addr, (socklen_t*) &clilen);

			if(new_sock != INVALID_SOCKET)
			{
				tv.tv_sec = 10;
				setsockopt(new_sock, SOL_SOCKET, SO_RCVTIMEO, (char*) &tv, sizeof(struct timeval));

				peer->cl_sock[peer->num_cls++] = new_sock;
				peer->packet[peer->num_cls-1].name = NULL;
				flushClientState(peer, peer->num_cls-1);

				printf("Nova conexao: %s:%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

				return 1;
			}
			else
                perror("erro");
		}
	}
	else
        perror("erro");

    return 0;
}

SOCKET create_socket(void)
{
    SOCKET sock = INVALID_SOCKET;
    WSADATA wsaData = {0};

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0)
    {
        sock = socket(AF_INET, SOCK_STREAM, 0);
    }
	else
		printf("Failed. Error Code : %d", WSAGetLastError());

    return sock;
}

int bind_socket(SOCKET* sock, int port)
{
    int result = 0;

    if(sock)
    {
        struct timeval tv = {0};
        tv.tv_sec = 10;
        setsockopt(*sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval));

        struct sockaddr_in serv_addr = {0};

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port);

		result =  bind(*sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    }

    return result;
}

void serverBroadcast(Peer* peer, char* msg, int clt_excl)
{
    int i;
    for(i = 0; i < peer->num_cls; i++)
    {
        if(i != clt_excl)
        {
            if(sendHeader(&peer->cl_sock[i], 1, strlen(msg)) < 0)
                return;

            if(sendPacket(&peer->cl_sock[i], msg, strlen(msg)) < 0)
                printf("falha ao enviar mensagem.\n");
        }
    }
}

void broadcastMessage(Peer* peer, int clt_excl)
{
    char* buffer = (char*) calloc(peer->packet[clt_excl].length + strlen(peer->packet[clt_excl].name) + 3, sizeof(char));
    strcat(buffer, peer->packet[clt_excl].name);
    strcat(buffer, ": ");
    strcat(buffer, (char*)peer->packet[clt_excl].data);

    char header[3] = {1};
    header[2] = strlen(buffer) & 0xff;
    header[1] = (strlen(buffer) >> 8) & 0xff;

    int i;
    for(i = 0; i < peer->num_cls; i++)
    {
        if(i != clt_excl)
        {
            if(sendPacket(&peer->cl_sock[i], header, 3) < 0)
            {
                printf("falha ao enviar mensagem.\n");
                return;
            }

            if(sendPacket(&peer->cl_sock[i], buffer, strlen(buffer)) < 0)
                printf("falha ao enviar mensagem.\n");

            printf("sending %s\n",(char*)(peer->packet[clt_excl].data));
        }
    }

    free((void*)buffer);
}

void sendMessageTo(Peer* peer, int clt_from) //formato da msg privada: tipo_da_msg/tamanho_do_campo_de_dados/dados - campo de dados: nome/2/msg
{
    char c[2] = {2, 0};
    int clNum;
    int n = strstr((const char*)peer->packet[clt_from].data, c) - (char*) peer->packet[clt_from].data; //encontra a posiçao do byte de valor 2 nos dados, que deve ser encontrado apos o nome
    peer->packet[clt_from].data[n] = 0;

    if((clNum = getClientNum(peer, (char*)peer->packet[clt_from].data)) < 0)
    {
        char msg[] = "usuario inexistente.";

        sendHeader(&peer->cl_sock[clt_from], 1, strlen(msg));
        sendPacket(&peer->cl_sock[clt_from], msg, strlen(msg));

        printf("%s\n", msg);
        return;
    }

    char* buffer = (char*) calloc(peer->packet[clt_from].length-n-1 + strlen(peer->packet[clt_from].name) + 7, sizeof(char));
    strcat(buffer, "[PM]");
    strcat(buffer, peer->packet[clt_from].name);
    strcat(buffer, ": ");
    strcat(buffer, (char*)peer->packet[clt_from].data+n+1);

    printf("pm from %s to %s\n", peer->packet[clt_from].name, (char*)peer->packet[clt_from].data);
    //printf("sending %d/%d/%s\n", header[0], (uint16_t)((header[2])|header[1]<<8), (char*)(peer->packet[clt_from].data+n+1));

    if(sendHeader(&peer->cl_sock[clNum], 1, strlen(buffer)) < 0)
        return;

    if(sendPacket(&peer->cl_sock[clNum], buffer, strlen(buffer)) < 0)
        printf("falha ao enviar mensagem.\n");

    free((void*)buffer);
}

int sendPacket(SOCKET* sock, char* buffer, int buff_size)
{
    int sent = 0;

    if(sock && buffer && (buff_size > 0))
    {
        sent = send(*sock, buffer, buff_size, 0);
        printf("sending %s | sent %d\n", buffer, sent);
    }

    return sent;
}

int sendHeader(SOCKET* sock, int param1, int param2)
{
    int sent = 0;
    char header[3] = {param1};

    header[2] = param2 & 0xff;
    header[1] = (param2 >> 8) & 0xff;

    if((sent = sendPacket(sock, header, HS)) < 0)
        printf("falha ao enviar mensagem.\n");

    return sent;
}

void changeClientName(Peer* peer, int clt_num)
{
    peer->packet[clt_num].name = (char*) malloc(sizeof(char) * peer->packet[clt_num].length+1);

    memcpy((void*)peer->packet[clt_num].name, (void*)peer->packet[clt_num].data, peer->packet[clt_num].length);
    peer->packet[clt_num].name[peer->packet[clt_num].length] = 0;

    char msg[] = " conectou.";
    char* buffer = (char*) calloc(strlen(peer->packet[clt_num].name) + strlen(msg) + 1, sizeof(char));
    strcat(buffer, peer->packet[clt_num].name);
    strcat(buffer, msg);

    serverBroadcast(peer, buffer, clt_num);

    free((void*)buffer);
}

int getClientsNum(Peer* peer)
{
    return peer->num_cls;
}

int hasMessage(Peer* peer, int clt_num)
{
    return peer->packet[clt_num].state == FULL;
}

char* getMessage(Peer* peer, int clt_num)
{
    return (char*) peer->packet[clt_num].data;
}

int getMessageType(Peer* peer, int clt_num)
{
    return peer->packet[clt_num].message;
}

int getMessageSize(Peer* peer, int clt_num)
{
    return peer->packet[clt_num].length;
}

void flushClientState(Peer* peer, int clt_num)
{
    peer->packet[clt_num].state = NONE;
    peer->packet[clt_num].message = 0;
}

int getClientNum(Peer* peer, char* name)
{
    int i;
    for(i = 0; i < peer->num_cls; i++)
    {
        if(!strcmp((char*)peer->packet[i].name, name))
            return i;
    }

    return -1;
}

void disconnect(Peer* peer)
{
    if(peer)
    {
        int i;
        for (i = 0; i < peer->num_cls; i++)
        {
            if(peer->packet[i].data)
                free((void*) peer->packet[i].data);
            if(peer->packet[i].name)
                free((void*) peer->packet[i].name);

            closesocket(peer->cl_sock[i]);
        }

        closesocket(peer->sock);
    }

    WSACleanup();
    free((void*) peer);
}
