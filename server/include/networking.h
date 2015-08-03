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
    SOCKET cl_sock[MAX_CLIENT];
    Packet packet[MAX_CLIENT];
	int num_cls;
}Peer;

Peer* startup(int port);
int checkPackets(Peer* peer);
int wait_connection(Peer* peer);
void disconnect(Peer* peer);

void broadcastMessage(Peer* peer, int clt_excl);
void sendMessageTo(Peer* peer, int clt_from);
void serverBroadcast(Peer* peer, char* msg, int clt_excl);
void changeClientName(Peer* peer, int clt_num);
int getClientsNum(Peer* peer);
int hasMessage(Peer* peer, int clt_num);
char* getMessage(Peer* peer, int clt_num);
int getMessageSize(Peer* peer, int clt_num);
int getMessageType(Peer* peer, int clt_num);
void flushClientState(Peer* peer, int clt_num);

#endif // NETWORKING_H_INCLUDED
