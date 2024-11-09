/***************************************************************
                         stage_lobby.c
                               
Handles the waiting room while we wait for everyone to connect
and ready up
***************************************************************/

#include <nusys.h>
#include "config.h"
#include "netlib.h"
#include "packets.h"
#include "helper.h"
#include "text.h"

// Controller data
static NUContData global_contdata;
static char global_isready;

/*==============================
    refresh_lobbytext
    Refreshes the text on the screen with the lobby ststus
==============================*/

void refresh_lobbytext()
{
    text_cleanup();

    // Generate the Lobby Title
    text_setfont(&font_default);
    text_setalign(ALIGN_CENTER);
    text_setcolor(255, 255, 255, 255);
    text_create("Player Lobby", SCREEN_WD/2, SCREEN_HT/2 - 64);
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
}


/*==============================
    stage_lobby_init
    Initialize the stage
==============================*/

void stage_lobby_init(void)
{
    refresh_lobbytext();
    global_isready = FALSE;
}


/*==============================
    stage_lobby_update
    Update stage variables every frame
==============================*/

void stage_lobby_update(void)
{
    if (global_players[0].connected && global_players[1].connected)
    {
        nuContDataGetEx(&global_contdata, 0);
        
        // Toggle ready state
        if(global_contdata.trigger & A_BUTTON)
        {
            global_isready != global_isready;
            global_players[netlib_getclient()-1].ready = global_isready;
            netlib_start(PACKETID_PLAYERREADY);
                netlib_writebyte(netlib_getclient());
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
    
    // Poll for incoming data
    netlib_poll();
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
    Refreshes the screen text when a player status change ocurred
==============================*/

void stage_lobby_playerchange()
{
    refresh_lobbytext();
}