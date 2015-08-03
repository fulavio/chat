#include <stdio.h>
#include <stdlib.h>
#include "networking.h"

int main()
{
    Peer* peer = startup(666);

    if(!peer)
        exit(1);

    while(1)
    {
        wait_connection(peer);
        checkPackets(peer);

        int clt;
        for(clt = 0; clt < getClientsNum(peer); clt++)
        {
            if(hasMessage(peer, clt))
            {
                switch(getMessageType(peer, clt))
                {
                case 0:
                    break;
                case 1:
                    broadcastMessage(peer, clt);
                    break;
                case 2:
                    sendMessageTo(peer, clt);
                    break;
                case 3:
                    changeClientName(peer, clt);
                    break;
                default:
                    printf("mensagem desconhecida: %d\n", getMessageType(peer, clt));
                }

                flushClientState(peer, clt);
            }
        }
    }

    disconnect(peer);

    return 0;
}
