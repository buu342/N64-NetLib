#include "netlib.h"
#include "stages.h"
#include "packets.h"
#include "helper.h"

void netcallback_playerinfo(size_t size);
void netcallback_disconnect(size_t size);
void netcallback_serverfull(size_t size);
void netcallback_heartbeat(size_t size);
void netcallback_playerready(size_t size);
void netcallback_gamestate(size_t size);
void netcallback_playerturn(size_t size);
void netcallback_playermove(size_t size);
void netcallback_playercursor(size_t size);
void netcallback_boardcompleted(size_t size);

void netcallback_initall()
{
    netlib_register(PACKETID_PLAYERINFO, &netcallback_playerinfo);
    netlib_register(PACKETID_PLAYERDISCONNECT, &netcallback_disconnect);
    netlib_register(PACKETID_SERVERFULL, &netcallback_serverfull);
    netlib_register(PACKETID_HEARTBEAT, &netcallback_heartbeat);
    netlib_register(PACKETID_PLAYERREADY, &netcallback_playerready);
    netlib_register(PACKETID_GAMESTATECHANGE, &netcallback_gamestate);
    netlib_register(PACKETID_PLAYERTURN, &netcallback_playerturn);
    netlib_register(PACKETID_PLAYERMOVE, &netcallback_playermove);
    netlib_register(PACKETID_PLAYERCURSOR, &netcallback_playercursor);
    netlib_register(PACKETID_BOARDCOMPLETED, &netcallback_boardcompleted);
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

void netcallback_gamestate(size_t size)
{
    if (stages_getcurrent() == STAGE_LOBBY)
        stage_lobby_statechange();
    else if (stages_getcurrent() == STAGE_GAME)
        stage_game_statechange();
}

void netcallback_playerturn(size_t size)
{
    stage_game_playerturn();
}

void netcallback_playermove(size_t size)
{
    stage_game_makemove();
}

void netcallback_playercursor(size_t size)
{
    stage_game_playercursor();
}

void netcallback_boardcompleted(size_t size)
{
    stage_game_boardcompleted();
}