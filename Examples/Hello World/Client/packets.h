#ifndef _N64_PACKETS_H
#define _N64_PACKETS_H
    
    
    /*********************************
               Enumerations
    *********************************/

    // Packet types
    typedef enum {
        PACKETID_ACKBEAT = 0,
        PACKETID_CLIENTCONNECT = 1,
    } NetPacketIDs;
    
    
    /*********************************
                Functions
    *********************************/
    
    extern void netcallback_initall();

#endif