/***************************************************************
                           packets.c
                               
Various callback functions for the different packet types in
this game.
***************************************************************/

#include "netlib.h"
#include "stages.h"
#include "packets.h"
#include "helper.h"
#include "objects.h"


/*********************************
        Function Prototypes
*********************************/

static void netcallback_heartbeat(size_t size);
static void netcallback_clientconnect(size_t size);
static void netcallback_serverfull(size_t size);
static void netcallback_clocksync(size_t size);
static void netcallback_clientinfo(size_t size);
static void netcallback_playerinfo(size_t size);


/*==============================
    netcallback_initall
    Initializes all the packet callback functions
==============================*/

void netcallback_initall()
{
    netlib_register(PACKETID_ACKBEAT, &netcallback_heartbeat);
    netlib_register(PACKETID_CLIENTCONNECT, &netcallback_clientconnect);
    netlib_register(PACKETID_SERVERFULL, &netcallback_serverfull);
    netlib_register(PACKETID_CLOCKSYNC, &netcallback_clocksync);
    netlib_register(PACKETID_CLIENTINFO, &netcallback_clientinfo);
    netlib_register(PACKETID_PLAYERINFO, &netcallback_playerinfo);
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
    netcallback_clientconnect
    Handles the PACKETID_CLIENTCONNECT packet
    @param The size of the incoming data
==============================*/

static void netcallback_clientconnect(size_t size)
{
    stage_init_connectpacket();
}


/*==============================
    netcallback_serverfull
    Handles the PACKETID_SERVERFULL packet
    @param The size of the incoming data
==============================*/

static void netcallback_serverfull(size_t size)
{
    stage_init_serverfull();
}


/*==============================
    netcallback_clocksync
    Handles the PACKETID_CLOCKSYNC packet
    @param The size of the incoming data
==============================*/

static void netcallback_clocksync(size_t size)
{
    stage_init_clockpacket();
}


/*==============================
    netcallback_clientinfo
    Handles the PACKETID_CLIENTINFO packet
    @param The size of the incoming data
==============================*/

static void netcallback_clientinfo(size_t size)
{
    u8 plynum;
    netlib_readbyte(&plynum);
    
    // Set our own player info
    netlib_setclient(plynum);
    objects_connectplayer(plynum, NULL);

    // Once we receive client info, we can move from the init stage to the main stage
    stages_changeto(STAGE_GAME);
}


/*==============================
    netcallback_playerinfo
    Handles the PACKETID_PLAYERINFO packet
    @param The size of the incoming data
==============================*/

static void netcallback_playerinfo(size_t size)
{
    
}