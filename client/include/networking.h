#ifndef NETWORKING_H_INCLUDED
#define NETWORKING_H_INCLUDED
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#define MAX_CLIENT 20

enum PACKET_STATE
{
    NONE,
    HALF,
    FULL
};

typedef struct packet
{
    uint8_t message;
    char* name;
    uint8_t* data;
	uint16_t length;
	uint8_t state;
}Packet;

typedef struct peer
{
    SOCKET sock;
    Packet packet;
}Peer;

Peer* startup(char* ip, int port);
int checkPackets(Peer* peer);
void wait_connection(Peer* peer);
void disconnect(Peer* peer);

void broadcastMessage(Peer* peer, char* buffer, int buffer_size);
void sendMessageTo(Peer* peer, char* name, char* msg, int msg_size);
void changeName(Peer* peer, char* name, int size);
int hasMessage(Peer* peer);
char* getMessage(Peer* peer);
int getMessageSize(Peer* peer);
int getMessageType(Peer* peer);
void flushClientState(Peer* peer);

#endif // NETWORKING_H_INCLUDED
