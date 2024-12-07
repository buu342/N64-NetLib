#ifndef _N64_PACKETS_H
#define _N64_PACKETS_H
    
    
    /*********************************
               Enumerations
    *********************************/

    // Packet types
    typedef enum {
        PACKETID_ACKBEAT = 0,
        PACKETID_CLIENTCONNECT = 1, // Unused in the client app
        PACKETID_PLAYERINFO = 2,
        PACKETID_PLAYERDISCONNECT = 3,
        PACKETID_SERVERFULL = 4,
        PACKETID_PLAYERREADY = 5,
        PACKETID_GAMESTATECHANGE = 6,
        PACKETID_PLAYERTURN = 7,
        PACKETID_PLAYERMOVE = 8,
        PACKETID_PLAYERCURSOR = 9,
        PACKETID_BOARDCOMPLETED = 10,
    } NetPacketIDs;
    
    // Game states
    typedef enum {
        GAMESTATE_LOBBY = 0,
        GAMESTATE_READY = 1,
        GAMESTATE_PLAYING = 2,
        GAMESTATE_ENDED_WINNER_1 = 3,
        GAMESTATE_ENDED_WINNER_2 = 4,
        GAMESTATE_ENDED_TIE = 5,
        GAMESTATE_ENDED_DISCONNECT = 6,
    } GameState;
    
    
    /*********************************
                Functions
    *********************************/
    
    extern void netcallback_initall();

#endif