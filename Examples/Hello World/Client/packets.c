/***************************************************************
                           packets.c
                               
Various callback functions for the different packet types in
this game.
***************************************************************/

#include "netlib.h"
#include "stages.h"
#include "packets.h"
#include "helper.h"


/*********************************
        Function Prototypes
*********************************/

static void netcallback_heartbeat(size_t size);
static void netcallback_playerconnect(size_t size);


/*==============================
    netcallback_initall
    Initializes all the packet callback functions
==============================*/

void netcallback_initall()
{
    netlib_register(PACKETID_ACKBEAT, &netcallback_heartbeat);
    netlib_register(PACKETID_CLIENTCONNECT, &netcallback_playerconnect);
}

/*==============================
    netcallback_heartbeat
    Handles the PACKETID_ACKBEAT packet
    @param The size of the incoming data
==============================*/

static void netcallback_heartbeat(size_t size)
{
    netlib_start(PACKETID_ACKBEAT); // If we get an ack or heartbeat, send one back
    netlib_sendtoserver();
}


/*==============================
    netcallback_playerinfo
    Handles the PACKETID_PLAYERINFO packet
    @param The size of the incoming data
==============================*/

static void netcallback_playerconnect(size_t size)
{
    stage_hello_connectpacket();
}