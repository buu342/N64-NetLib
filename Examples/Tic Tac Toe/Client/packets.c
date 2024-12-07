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

static void netcallback_playerinfo(size_t size);
static void netcallback_disconnect(size_t size);
static void netcallback_serverfull(size_t size);
static void netcallback_heartbeat(size_t size);
static void netcallback_playerready(size_t size);
static void netcallback_gamestate(size_t size);
static void netcallback_playerturn(size_t size);
static void netcallback_playermove(size_t size);
static void netcallback_playercursor(size_t size);
static void netcallback_boardcompleted(size_t size);


/*==============================
    netcallback_initall
    Initializes all the packet callback functions
==============================*/

void netcallback_initall()
{
    netlib_register(PACKETID_ACKBEAT, &netcallback_heartbeat);
    netlib_register(PACKETID_PLAYERINFO, &netcallback_playerinfo);
    netlib_register(PACKETID_PLAYERDISCONNECT, &netcallback_disconnect);
    netlib_register(PACKETID_SERVERFULL, &netcallback_serverfull);
    netlib_register(PACKETID_PLAYERREADY, &netcallback_playerready);
    netlib_register(PACKETID_GAMESTATECHANGE, &netcallback_gamestate);
    netlib_register(PACKETID_PLAYERTURN, &netcallback_playerturn);
    netlib_register(PACKETID_PLAYERMOVE, &netcallback_playermove);
    netlib_register(PACKETID_PLAYERCURSOR, &netcallback_playercursor);
    netlib_register(PACKETID_BOARDCOMPLETED, &netcallback_boardcompleted);
}


/*==============================
    netcallback_playerinfo
    Handles the PACKETID_PLAYERINFO packet
    @param The size of the incoming data
==============================*/

static void netcallback_playerinfo(size_t size)
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


/*==============================
    netcallback_disconnect
    Handles the PACKETID_PLAYERDISCONNECT packet
    @param The size of the incoming data
==============================*/

static void netcallback_disconnect(size_t size)
{
    u8 plynum;
    netlib_readbyte(&plynum);
    global_players[plynum-1].connected = FALSE;
    global_players[plynum-1].ready = FALSE;
    if (stages_getcurrent() == STAGE_LOBBY)
        stage_lobby_playerchange();
}


/*==============================
    netcallback_serverfull
    Handles the PACKETID_SERVERFULL packet
    @param The size of the incoming data
==============================*/

static void netcallback_serverfull(size_t size)
{
    if (stages_getcurrent() == STAGE_INIT)
        stage_init_serverfull();
}


/*==============================
    netcallback_heartbeat
    Handles the PACKETID_ACKBEAT packet
    @param The size of the incoming data
==============================*/

static void netcallback_heartbeat(size_t size)
{
    netlib_start(PACKETID_ACKBEAT);
    netlib_sendtoserver();
}


/*==============================
    netcallback_playerready
    Handles the PACKETID_PLAYERREADY packet
    @param The size of the incoming data
==============================*/

static void netcallback_playerready(size_t size)
{
    u8 plynum, ready;
    netlib_readbyte(&plynum);
    netlib_readbyte(&ready);
    global_players[plynum-1].ready = ready;
    if (stages_getcurrent() == STAGE_LOBBY)
        stage_lobby_playerchange();
}


/*==============================
    netcallback_gamestate
    Handles the PACKETID_GAMESTATECHANGE packet
    @param The size of the incoming data
==============================*/

static void netcallback_gamestate(size_t size)
{
    if (stages_getcurrent() == STAGE_LOBBY)
        stage_lobby_statechange();
    else if (stages_getcurrent() == STAGE_GAME)
        stage_game_statechange();
}


/*==============================
    netcallback_playerturn
    Handles the PACKETID_PLAYERTURN packet
    @param The size of the incoming data
==============================*/

static void netcallback_playerturn(size_t size)
{
    stage_game_playerturn();
}


/*==============================
    netcallback_playermove
    Handles the PACKETID_PLAYERMOVE packet
    @param The size of the incoming data
==============================*/

static void netcallback_playermove(size_t size)
{
    stage_game_makemove();
}


/*==============================
    netcallback_playercursor
    Handles the PACKETID_PLAYERCURSOR packet
    @param The size of the incoming data
==============================*/

static void netcallback_playercursor(size_t size)
{
    stage_game_playercursor();
}


/*==============================
    netcallback_boardcompleted
    Handles the PACKETID_BOARDCOMPLETED packet
    @param The size of the incoming data
==============================*/

static void netcallback_boardcompleted(size_t size)
{
    stage_game_boardcompleted();
}