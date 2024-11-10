#ifndef _N64_PACKETS_H
#define _N64_PACKETS_H

    typedef enum {
        PACKETID_CLIENTCONNECT = 0,
        PACKETID_PLAYERINFO = 1,
        PACKETID_PLAYERDISCONNECT = 2,
        PACKETID_SERVERFULL = 3,
        PACKETID_HEARTBEAT = 4,
        PACKETID_PLAYERREADY = 5,
    } NetPacketIDs;

    void netcallback_initall();

#endif