#ifndef _N64_PACKETS_H
#define _N64_PACKETS_H
    
    #include <nusys.h>
    #include "mathtypes.h"
    
    
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
        PACKETID_CLIENTINFO = 5,
        PACKETID_PLAYERINFO = 6,
        PACKETID_PLAYERDISCONNECT = 7,
        PACKETID_CLIENTINPUT = 8,
        PACKETID_OBJECTCREATE = 9,
        PACKETID_OBJECTUPDATE = 10,
        PACKETID_PLAYERUPDATE = 11,
    } NetPacketIDs;
    
    
    /*********************************
                 Structs
    *********************************/
    
    typedef struct {
        OSTime time;
        NUContData contdata;
        float dt;
    } InputToAck;
    
    
    /*********************************
                Functions
    *********************************/
    
    extern void netcallback_initall();

#endif