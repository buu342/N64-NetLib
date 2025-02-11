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
static void netcallback_createobject(size_t size);
static void netcallback_updateobject(size_t size);


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
    netlib_register(PACKETID_OBJECTCREATE, &netcallback_createobject);
    netlib_register(PACKETID_OBJECTUPDATE, &netcallback_updateobject);
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
    GameObject* obj = objects_create();
    netlib_readbyte(&plynum);
    netlib_readdword((u32*)&obj->id);
    netlib_readfloat(&obj->pos.x);
    netlib_readfloat(&obj->pos.y);
    netlib_readfloat(&obj->dir.x);
    netlib_readfloat(&obj->dir.y);
    netlib_readfloat(&obj->size.x);
    netlib_readfloat(&obj->size.y);
    netlib_readdword((u32*)&obj->speed);
    netlib_readbyte(&obj->col.r);
    netlib_readbyte(&obj->col.g);
    netlib_readbyte(&obj->col.b);
    
    // Set our own player info
    netlib_setclient(plynum);
    objects_connectplayer(plynum, obj);

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
    u8 plynum;
    GameObject* obj = objects_create();
    netlib_readbyte(&plynum);
    netlib_readdword((u32*)&obj->id);
    netlib_readfloat(&obj->pos.x);
    netlib_readfloat(&obj->pos.y);
    netlib_readfloat(&obj->dir.x);
    netlib_readfloat(&obj->dir.y);
    netlib_readfloat(&obj->size.x);
    netlib_readfloat(&obj->size.y);
    netlib_readdword((u32*)&obj->speed);
    netlib_readbyte(&obj->col.r);
    netlib_readbyte(&obj->col.g);
    netlib_readbyte(&obj->col.b);
    objects_connectplayer(plynum, obj);
}


/*==============================
    netcallback_createobject
    Handles the PACKETID_OBJECTCREATE packet
    @param The size of the incoming data
==============================*/

static void netcallback_createobject(size_t size)
{
    stage_game_createobject();
}


/*==============================
    netcallback_updateobject
    Handles the PACKETID_OBJECTUPDATE packet
    @param The size of the incoming data
==============================*/

static void netcallback_updateobject(size_t size)
{
    stage_game_updateobject(size);
}