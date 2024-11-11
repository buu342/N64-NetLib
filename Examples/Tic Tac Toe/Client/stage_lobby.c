/***************************************************************
                         stage_lobby.c
                               
Handles the waiting room while we wait for everyone to connect
and ready up
***************************************************************/

#include <nusys.h>
#include "config.h"
#include "stages.h"
#include "netlib.h"
#include "packets.h"
#include "helper.h"
#include "text.h"

// Controller data
static NUContData global_contdata;
static u8 global_isready;
static u8 global_gamestarting;


/*==============================
    refresh_lobbytext
    Refreshes the text on the screen with the lobby status
==============================*/

static void refresh_lobbytext()
{
    text_cleanup();

    // Generate the Lobby Title
    text_setfont(&font_default);
    text_setalign(ALIGN_CENTER);
    text_setcolor(255, 255, 255, 255);
    text_create("Player Lobby", SCREEN_WD/2, SCREEN_HT/2 - 88);
    
    // Game starting notification
    if (global_gamestarting)
        text_create("Game starting...", SCREEN_WD/2, SCREEN_HT/2-40);
    
    // Player state text
    text_setfont(&font_small);
    text_setalign(ALIGN_LEFT);
    
    // Player 1 state
    if (netlib_getclient() == 1)
        text_setcolor(255, 255, 0, 255);
    text_create("Player 1", 32, SCREEN_HT/2 + 16);
    if (global_players[0].connected)
    {
        if (global_players[0].ready)
            text_create("Ready", 128, SCREEN_HT/2 + 16);
        else
            text_create("Not ready", 128, SCREEN_HT/2 + 16);
    }
    else
        text_create("Not connected", 128, SCREEN_HT/2 + 16);
    text_setcolor(255, 255, 255, 255);
    
    // Player 2 state
    if (netlib_getclient() == 2)
        text_setcolor(255, 255, 0, 255);
    text_create("Player 2", 32, SCREEN_HT/2 + 48);
    if (global_players[1].connected)
    {
        if (global_players[1].ready)
            text_create("Ready", 128, SCREEN_HT/2 + 48);
        else
            text_create("Not ready", 128, SCREEN_HT/2 + 48);
    }
    else
        text_create("Not connected", 128, SCREEN_HT/2 + 48);
    text_setcolor(255, 255, 255, 255);
    
    // Press A to toggle ready
    if (global_players[0].connected && global_players[0].connected && !global_gamestarting)
    {
        text_setalign(ALIGN_CENTER);
        text_create("Press A to toggle ready", SCREEN_WD/2, SCREEN_HT/2 + 88);
    }
}


/*==============================
    stage_lobby_init
    Initialize the stage
==============================*/

void stage_lobby_init(void)
{
    refresh_lobbytext();
    global_isready = FALSE;
    global_gamestarting = FALSE;
}


/*==============================
    stage_lobby_update
    Update stage variables every frame
==============================*/

void stage_lobby_update(void)
{
    if (global_players[0].connected && global_players[1].connected && !global_gamestarting)
    {
        nuContDataGetEx(&global_contdata, 0);
        
        // Toggle ready state
        if(global_contdata.trigger & A_BUTTON)
        {
            if (global_isready)
                global_isready = FALSE;
            else
                global_isready = TRUE;
            global_players[netlib_getclient()-1].ready = global_isready;
            netlib_start(PACKETID_PLAYERREADY);
                netlib_writebyte(global_isready);
            netlib_sendtoserver();
            refresh_lobbytext();
        }
    }
    else
    {
        global_isready = FALSE;
        global_players[0].ready = FALSE;
        global_players[1].ready = FALSE;
    }
}


/*==============================
    stage_lobby_draw
    Draw the stage
==============================*/

void stage_lobby_draw(void)
{
    glistp = glist;

    // Initialize the RCP and framebuffer
    rcp_init();
    fb_clear(0, 0, 0);

    // Render some text
    text_render();

    // Finish
    gDPFullSync(glistp++);
    gSPEndDisplayList(glistp++);
    nuGfxTaskStart(glist, (s32)(glistp - glist) * sizeof(Gfx), NU_GFX_UCODE_F3DEX, NU_SC_SWAPBUFFER);
}


/*==============================
    stage_lobby_cleanup
    Cleans up memory used by the stage
==============================*/

void stage_lobby_cleanup(void)
{
    text_cleanup();
}


/*==============================
    stage_lobby_playerchange
    Refreshes the screen text when a player status change occurred
==============================*/

void stage_lobby_playerchange()
{
    global_isready = FALSE;
    global_players[netlib_getclient()-1].ready;
    refresh_lobbytext();
}


/*==============================
    stage_lobby_statechange
    Refreshes the screen text when a player status change occurred
==============================*/

void stage_lobby_statechange()
{
    u8 state;
    netlib_readbyte(&state);
    if (state == GAMESTATE_READY)
    {
        global_gamestarting = TRUE;
        refresh_lobbytext();
    }
    if (state == GAMESTATE_PLAYING)
        stages_changeto(STAGE_GAME);
}