#ifndef _N64_PACKETS_H
#define _N64_PACKETS_H

    typedef enum {
        PACKETID_CLIENTCONNECT = 0,
        PACKETID_PLAYERINFO = 1,
        PACKETID_PLAYERDISCONNECT = 2,
        PACKETID_SERVERFULL = 3,
        PACKETID_HEARTBEAT = 4,
        PACKETID_PLAYERREADY = 5,
        PACKETID_GAMESTATECHANGE = 6,
        PACKETID_PLAYERTURN = 7,
        PACKETID_PLAYERMOVE = 8,
    } NetPacketIDs;
    
    typedef enum {
        GAMESTATE_LOBBY = 0,
        GAMESTATE_READY = 1,
        GAMESTATE_PLAYING = 2,
        GAMESTATE_ENDED_WINNER_1 = 3,
        GAMESTATE_ENDED_WINNER_2 = 4,
        GAMESTATE_ENDED_TIE = 5,
        GAMESTATE_ENDED_DISCONNECT = 6,
    } GameState;

    void netcallback_initall();

#endif