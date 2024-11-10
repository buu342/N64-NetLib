#include "netlib.h"
#include "stages.h"
#include "packets.h"
#include "helper.h"

void netcallback_playerinfo(size_t size);
void netcallback_disconnect(size_t size);
void netcallback_serverfull(size_t size);
void netcallback_heartbeat(size_t size);
void netcallback_playerready(size_t size);

void netcallback_initall()
{
    netlib_register(PACKETID_PLAYERINFO, &netcallback_playerinfo);
    netlib_register(PACKETID_PLAYERDISCONNECT, &netcallback_disconnect);
    netlib_register(PACKETID_SERVERFULL, &netcallback_serverfull);
    netlib_register(PACKETID_HEARTBEAT, &netcallback_heartbeat);
    netlib_register(PACKETID_PLAYERREADY, &netcallback_playerready);
}

void netcallback_playerinfo(size_t size)
{
    u8 plynum;
    netlib_readbyte(&plynum);
    
    // If the client number is not set, then we are receiving our player info.
    if (netlib_getclient() == 0)
        netlib_setclient(plynum);
    global_players[plynum-1].ready = FALSE;
    global_players[plynum-1].connected = TRUE;
    if (stages_getcurrent() == STAGE_LOBBY)
        stage_lobby_playerchange();
}

void netcallback_disconnect(size_t size)
{
    u8 plynum;
    netlib_readbyte(&plynum);
    global_players[plynum-1].connected = FALSE;
    global_players[plynum-1].ready = FALSE;
    if (stages_getcurrent() == STAGE_LOBBY)
        stage_lobby_playerchange();
}

void netcallback_serverfull(size_t size)
{
    if (stages_getcurrent() == STAGE_INIT)
        stage_init_serverfull();
}

void netcallback_heartbeat(size_t size)
{
    netlib_start(PACKETID_HEARTBEAT);
    netlib_sendtoserver();
}

void netcallback_playerready(size_t size)
{
    u8 plynum, ready;
    netlib_readbyte(&plynum);
    netlib_readbyte(&ready);
    global_players[plynum-1].ready = ready;
    if (stages_getcurrent() == STAGE_LOBBY)
        stage_lobby_playerchange();
}