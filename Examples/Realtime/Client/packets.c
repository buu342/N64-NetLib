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

static GameObject* packet_readobject();

static void netcallback_heartbeat(size_t size);
static void netcallback_clientconnect(size_t size);
static void netcallback_serverfull(size_t size);
static void netcallback_clocksync(size_t size);
static void netcallback_clientinfo(size_t size);
static void netcallback_playerinfo(size_t size);
static void netcallback_playerdisconnect(size_t size);
static void netcallback_createobject(size_t size);
static void netcallback_updateobject(size_t size);
static void netcallback_updateplayer(size_t size);


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
    netlib_register(PACKETID_PLAYERDISCONNECT, &netcallback_playerdisconnect);
    netlib_register(PACKETID_OBJECTCREATE, &netcallback_createobject);
    netlib_register(PACKETID_OBJECTUPDATE, &netcallback_updateobject);
    netlib_register(PACKETID_PLAYERUPDATE, &netcallback_updateplayer);
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
    packet_readobject
    A helper function for reading an object's data from a packet
    Useful since player and object data is almost exactly the same
    @return The created object
==============================*/

static GameObject* packet_readobject()
{
    u32 objid;
    GameObject* obj;
    
    // Check if the object already exists, and create it if it doesn't
    netlib_readdword(&objid);
    obj = objects_findbyid(objid);
    if (obj == NULL)
    {
        obj = objects_create();
        obj->id = objid;
    }
    
    // Assign the rest of the values
    netlib_readfloat(&obj->pos.x);
    netlib_readfloat(&obj->pos.y);
    obj->oldpos.x = obj->pos.x;
    obj->oldpos.y = obj->pos.y;
    netlib_readfloat(&obj->dir.x);
    netlib_readfloat(&obj->dir.y);
    netlib_readfloat(&obj->size.x);
    netlib_readfloat(&obj->size.y);
    netlib_readfloat(&obj->speed);
    netlib_readbyte(&obj->bounce);
    netlib_readbyte(&obj->col.r);
    netlib_readbyte(&obj->col.g);
    netlib_readbyte(&obj->col.b);
    return obj;
}


/*==============================
    packet_readobjectupdate
    A helper function for reading an object's update from a packet
    Useful since player and object update format is almost exactly the same
    @param The object to read the packet info into
    @param The data size left to read
==============================*/

static void packet_readobjectupdate(GameObject* obj, size_t size)
{
    // Read the data in the packet
    while (size > 0)
    {
        u8 datatype;
        netlib_readbyte(&datatype);
        size -= sizeof(u8);
        switch (datatype)
        {
            case 0:
                if (obj != NULL)
                {
                    obj->oldpos.x = obj->pos.x;
                    obj->oldpos.y = obj->pos.y;
                    netlib_readfloat(&obj->pos.x);
                    netlib_readfloat(&obj->pos.y);
                }
                else
                    netlib_skipbytes(sizeof(f32)*2);
                size -= sizeof(f32)*2;
                break;
            case 1:
                if (obj != NULL)
                {
                    netlib_readfloat(&obj->dir.x);
                    netlib_readfloat(&obj->dir.y);
                }
                else
                    netlib_skipbytes(sizeof(f32)*2);
                size -= sizeof(f32)*2;
                break;
            case 2:
                if (obj != NULL)
                {
                    netlib_readfloat(&obj->size.x);
                    netlib_readfloat(&obj->size.y);
                }
                else
                    netlib_skipbytes(sizeof(f32)*2);
                size -= sizeof(f32)*2;
                break;
            case 3:
                if (obj != NULL)
                    netlib_readfloat(&obj->speed);
                else
                    netlib_skipbytes(sizeof(f32));
                size -= sizeof(u32);
                break;
        }
    }
}


/*==============================
    netcallback_clientinfo
    Handles the PACKETID_CLIENTINFO packet
    @param The size of the incoming data
==============================*/

static void netcallback_clientinfo(size_t size)
{
    u8 plynum;
    GameObject* obj;
    netlib_readbyte(&plynum);
    obj = packet_readobject();
    
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
    GameObject* obj;
    netlib_readbyte(&plynum);
    
    // Connect the object to the player
    obj = packet_readobject();
    objects_connectplayer(plynum, obj);
}


/*==============================
    netcallback_playerdisconnect
    Handles the PACKETID_PLAYERDISCONNECT packet
    @param The size of the incoming data
==============================*/

static void netcallback_playerdisconnect(size_t size)
{
    u8 plynum;
    netlib_readbyte(&plynum);
    objects_disconnectplayer(plynum);
}


/*==============================
    netcallback_createobject
    Handles the PACKETID_OBJECTCREATE packet
    @param The size of the incoming data
==============================*/

static void netcallback_createobject(size_t size)
{
    packet_readobject();
}


/*==============================
    netcallback_updateobject
    Handles the PACKETID_OBJECTUPDATE packet
    @param The size of the incoming data
==============================*/

static void netcallback_updateobject(size_t size)
{
    u8 objcount;
    
    // Read the object count
    netlib_readbyte(&objcount);
    size -= sizeof(u8);
    
    // Read each object's data
    while (objcount > 0)
    {
        u32 id;
        GameObject* obj;
        
        // Get the affected object's ID
        netlib_readdword(&id);
        size -= sizeof(u32);
        obj = objects_findbyid(id);
        
        // Update the object and handle the next one
        packet_readobjectupdate(obj, size);
        objcount--;
    }
}


/*==============================
    netcallback_updateplayer
    Handles the PACKETID_PLAYERUPDATE packet
    @param The size of the incoming data
==============================*/

static void netcallback_updateplayer(size_t size)
{
    GameObject* clobj;
    u8 objcount;
    u64 time;
    
    // Read the object count
    netlib_readbyte(&objcount);
    size -= sizeof(u8);
    
    // Read the last acknowledged input time
    netlib_readqword(&time);
    size -= sizeof(u64);
    
    // Read each object's data
    while (objcount > 0)
    {
        u8 plynum;
        GameObject* obj;
        
        // Get the affected player's object
        netlib_readbyte(&plynum);
        size -= sizeof(u8);
        obj = global_players[plynum-1].obj;
        
        // Read the player update data
        packet_readobjectupdate(obj, size);
        objcount--;
    }
    
    // Acknowledge our last input
    clobj = global_players[netlib_getclient()-1].obj;
    stage_game_ackinput(time, clobj->pos);
}