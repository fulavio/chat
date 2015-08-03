#include "networking.h"
#define HS 3 //header size

SOCKET create_socket(void);
int connect_socket(SOCKET* sock, char* ip, int port);
int sendPacket(SOCKET* sock, char* buffer, int buff_size);

Peer* startup(char* ip, int port)
{
    Peer* peer = (Peer*) malloc(sizeof(Peer));
    if(!peer)
        return NULL;

    peer->sock = create_socket();
    peer->packet.name = NULL;
    peer->packet.state = NONE;
    peer->packet.message = 0;

    if(peer->sock == INVALID_SOCKET)
    {
        printf("Failed. Error Code : %d", WSAGetLastError());
		free((void*) peer);
        peer = NULL;
    }

    if(connect_socket(&peer->sock, ip, port) == SOCKET_ERROR)
    {
        printf("Failed. Error Code : %d\n", WSAGetLastError());
		free((void*) peer);
        peer = NULL;
    }

    return peer;
}

int checkPackets(Peer* peer)
{
    if((!peer) || (peer->sock == INVALID_SOCKET))
    {
        perror("erro");
        return 0;
    }

    if(peer->packet.state == FULL)
        return 1;

    char buffer[2048] = {0};
    int dataLen = 0;
    SOCKET sock = peer->sock;

    if((dataLen = recv(sock, buffer, 2048, 0)) > 0)
    {
        if(peer->packet.state == NONE)
        {
            if(peer->packet.data)
                free((void*) peer->packet.data);

            peer->packet.message = buffer[0];
            if(buffer[0] > 3) return 1;
            peer->packet.length = ntohs(*(u_short*)(buffer+1));
            peer->packet.data = (uint8_t*) calloc(peer->packet.length+1, sizeof(uint8_t));

            memcpy(peer->packet.data, buffer+3, strlen(buffer+3));

            if(strlen((const char*)peer->packet.data) == peer->packet.length)
                peer->packet.state = FULL;
            else
            {
                peer->packet.state = HALF;
                peer->packet.data[strlen(buffer+3)] = 0;
            }

        }

        else if(peer->packet.state == HALF)
        {
            memcpy(peer->packet.data+strlen((const char*)peer->packet.data), buffer, strlen(buffer));

            if(strlen((const char*)peer->packet.data) == peer->packet.length)
                peer->packet.state = FULL;
            else
                peer->packet.data[strlen(buffer)] = 0;
        }

//        if(buffer[0] > 3)
//            buffer[1]=0xff, buffer[2]=0xff;
//        if(peer->packet.name)
//            printf("%s: 0x0%c/%d/%s | %d bytes\n", peer->packet.name, peer->packet.message+48, peer->packet.length, peer->packet.data, strlen(buffer));
//        else
//            printf("????: 0x0%c/%d/%s | %d bytes\n", peer->packet.message+48, peer->packet.length, peer->packet.data, strlen(buffer));
    }

    else if(WSAGetLastError() != WSAETIMEDOUT)
    {
        printf("deconectado.\n");
        disconnect(peer);
        return 0;
    }

    return 1;
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

int connect_socket(SOCKET* sock, char* ip, int port)
{
    int result = 0;

    if(sock)
    {
        struct timeval tv = {0};
        tv.tv_sec = 10;
        setsockopt(*sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval));

        struct sockaddr_in serv_addr = {0};

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(ip);;
        serv_addr.sin_port = htons(port);

		result =  connect(*sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
    }

    return result;
}

void broadcastMessage(Peer* peer, char* buffer, int buffer_size)
{
    if((!peer) || (peer->sock == INVALID_SOCKET) || (!buffer))
    {
        perror("errow");
        return;
    }
	
	if(buffer_size <= 0)
		return;

    char header[3] = {1};

    header[2] = buffer_size & 0xff;
	header[1] = (buffer_size >> 8) & 0xff;

    if(!sendPacket(&peer->sock, header, 3))
    {
        printf("falha ao enviar mensagem.\n");
        return;
    }

	if(!sendPacket(&peer->sock, buffer, buffer_size))
        printf("falha ao enviar mensagem.\n");
}

void sendMessageTo(Peer* peer, char* name, char* msg, int msg_size)
{
    if((!peer) || (peer->sock == INVALID_SOCKET) || (!msg) || (!name))
    {
        perror("errow");
        return;
    }
	
	if(msg_size <= 0)
		return;

    char* buffer = (char*) calloc(msg_size + strlen(name) + 1, sizeof(char));
    char c[] = {2, 0};
    strcat(buffer, name);
    strcat(buffer, c);
    strcat(buffer, msg);

    char header[3] = {2};

    header[2] = strlen(buffer) & 0xff;
	header[1] = (strlen(buffer) >> 8) & 0xff;

    if(!sendPacket(&peer->sock, header, 3))
    {
        printf("falha ao enviar mensagem.\n");
        return;
    }

	if(!sendPacket(&peer->sock, buffer, strlen(buffer)))
        printf("falha ao enviar mensagem.\n");
}

int sendPacket(SOCKET* sock, char* buffer, int buff_size)
{
    int sent = 0;

    if(sock && buffer && (buff_size > 0))
    {
        //printf("sending %s\n", buffer);
        sent = send(*sock, buffer, buff_size, 0);
    }

    return sent;
}

void changeName(Peer* peer, char* name, int size)
{
    if((!peer) || peer->sock == INVALID_SOCKET)
        return;

    if(peer->packet.name)
        free((void*)peer->packet.name);

    peer->packet.name = (char*) calloc(size+1, sizeof(char));

    char header[3] = {3, 0, size};
    int len;

    len = sendPacket(&peer->sock, header, 3);

	if(len >= 0)
		len = sendPacket(&peer->sock, name, size);

	if(len < 0)
		perror("ERROR writing to socket");
}

int hasMessage(Peer* peer)
{
    if((!peer) || peer->sock == INVALID_SOCKET)
    {
        perror("erro");
        return -1;
    }

    return peer->packet.state == FULL;
}

char* getMessage(Peer* peer)
{
    if((!peer) || peer->sock == INVALID_SOCKET)
    {
        perror("erro");
        return NULL;
    }

    return (char*) peer->packet.data;
}

int getMessageType(Peer* peer)
{
    if((!peer) || peer->sock == INVALID_SOCKET)
    {
        perror("erro");
        return -1;
    }

    return peer->packet.message;
}

int getMessageSize(Peer* peer)
{
     if((!peer) || peer->sock == INVALID_SOCKET)
    {
        perror("erro");
        return -1;
    }

    return peer->packet.length;
}

void flushClientState(Peer* peer)
{
    peer->packet.state = NONE;
    peer->packet.message = 0;
}

void disconnect(Peer* peer)
{
    if(peer)
    {
        closesocket(peer->sock);
        free((void*) peer);
    }

    WSACleanup();
}
