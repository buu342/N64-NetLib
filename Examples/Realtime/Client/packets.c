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
    int i;
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
    netlib_readfloat(&obj->sv_trans.pos.x);
    netlib_readfloat(&obj->sv_trans.pos.y);
    netlib_readfloat(&obj->sv_trans.dir.x);
    netlib_readfloat(&obj->sv_trans.dir.y);
    netlib_readfloat(&obj->sv_trans.size.x);
    netlib_readfloat(&obj->sv_trans.size.y);
    netlib_readfloat(&obj->sv_trans.speed);
    netlib_readbyte(&obj->bounce);
    netlib_readbyte(&obj->sv_trans.col.r);
    netlib_readbyte(&obj->sv_trans.col.g);
    netlib_readbyte(&obj->sv_trans.col.b);
    for (i=0; i<TICKSTOKEEP-1; i++)
        obj->old_trans[i].timestamp = 0;
    obj->sv_trans.timestamp = osGetTime();
    objects_synctransforms(obj);
    return obj;
}


/*==============================
    packet_readobjectupdate
    A helper function for reading an object's update from a packet
    Useful since player and object update format is almost exactly the same
    @param The object to read the packet info into
    @param The data size left to read
==============================*/

static void packet_readobjectupdate(GameObject* obj, u8 size)
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
                    int i;
                    netlib_readfloat(&obj->sv_trans.pos.x);
                    netlib_readfloat(&obj->sv_trans.pos.y);
                }
                else
                    netlib_skipbytes(sizeof(f32)*2);
                size -= sizeof(f32)*2;
                break;
            case 1:
                if (obj != NULL)
                {
                    netlib_readfloat(&obj->sv_trans.dir.x);
                    netlib_readfloat(&obj->sv_trans.dir.y);
                }
                else
                    netlib_skipbytes(sizeof(f32)*2);
                size -= sizeof(f32)*2;
                break;
            case 2:
                if (obj != NULL)
                {
                    netlib_readfloat(&obj->sv_trans.size.x);
                    netlib_readfloat(&obj->sv_trans.size.y);
                }
                else
                    netlib_skipbytes(sizeof(f32)*2);
                size -= sizeof(f32)*2;
                break;
            case 3:
                if (obj != NULL)
                    netlib_readfloat(&obj->sv_trans.speed);
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
    u64 time;
    OSTime ctime;
    
    // Read the object count and update time
    netlib_readbyte(&objcount);
    netlib_readqword(&time);
    ctime = OS_NSEC_TO_CYCLES(time);
    
    // Read each object's data
    while (objcount > 0)
    {
        u32 id;
        u8 datasize;
        GameObject* obj;
        
        // Get the affected object's ID and the data size
        netlib_readdword(&id);
        netlib_readbyte(&datasize);
        obj = objects_findbyid(id);
        
        // Update the object and handle the next one
        objects_pusholdtransforms(obj);
        packet_readobjectupdate(obj, datasize);
        if (obj != NULL)
        {
            obj->sv_trans.timestamp = ctime;
            objects_synctransforms(obj);
        }
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
    u8 objcount;
    u64 time;
    bool reconcile = FALSE;
    GameObject* clobj = global_players[netlib_getclient()-1].obj;
    
    // Read the object count and the last acknowledged input time
    netlib_readbyte(&objcount);
    netlib_readqword(&time);
    
    // Read each object's data
    while (objcount > 0)
    {
        u8 plynum;
        u8 datasize;
        GameObject* obj;
        
        // Get the affected player's object and the data size
        netlib_readbyte(&plynum);
        netlib_readbyte(&datasize);
        obj = global_players[plynum-1].obj;
        
        // If this object is the client, then we need to reconcile later
        if (obj == clobj)
            reconcile = TRUE;
        
        // Read the player update data
        // We pass in obj, even if null, so that it still reads the data from the packet (rather, skips the data)
        objects_pusholdtransforms(obj);
        packet_readobjectupdate(obj, datasize);
        if (obj != NULL)
        {
            obj->sv_trans.timestamp = time;
            objects_synctransforms(obj);
        }
        
        // Next object
        objcount--;
    }
        
    // Reconcile input
    // For less "abrasive" correction, you should lerp to the player's position over some frames instead of setting it instantly.
    // Like that the client won't just teleport if the prediction was off.
    stage_game_ackinput(time, reconcile);
}