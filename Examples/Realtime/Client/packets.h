#ifndef _N64_PACKETS_H
#define _N64_PACKETS_H
    
    
    /*********************************
               Enumerations
    *********************************/

    // Packet types
    typedef enum {
        PACKETID_ACKBEAT = 0,
        PACKETID_CLIENTCONNECT = 1,
        PACKETID_SERVERFULL = 2,
        PACKETID_CLOCKSYNC = 3,
        PACKETID_DONESYNC = 4,
        PACKETID_PLAYERINFO = 5,
        PACKETID_PLAYERDISCONNECT = 6,
    } NetPacketIDs;
    
    
    /*********************************
                Functions
    *********************************/
    
    extern void netcallback_initall();

#endif