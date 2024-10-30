/***************************************************************
                          stage_init.c
                               
Handles the first level of the game.
***************************************************************/

#include <nusys.h>
#include "config.h"
#include "netlib.h"
#include "helper.h"
#include "text.h"

typedef enum {
    PACKETTYPE_PLAYERINFO = 0,
} NetPacketType;

bool global_hasrequestedinfo;

void netcallback_playerinfo(size_t size, ClientNumber client);


/*==============================
    stage_init_init
    Initialize the stage
==============================*/

void stage_init_init(void)
{
    netlib_register(PACKETTYPE_PLAYERINFO, &netcallback_playerinfo);
    
    // Generate the wait text
    text_setfont(&font_default);
    text_setalign(ALIGN_CENTER);
    text_setcolor(255, 255, 255, 255);
    text_create("Waiting for Server...", SCREEN_WD/2, SCREEN_HT/2 - 64);
    text_setfont(&font_small);
    text_create("Ensure the USB is connected", SCREEN_WD/2, SCREEN_HT/2 + 16);
    text_create("and the client is running", SCREEN_WD/2, SCREEN_HT/2 + 32);
    
    // Get player info from the server
    global_hasrequestedinfo = FALSE;
}


/*==============================
    stage_init_update
    Update stage variables every frame
==============================*/

void stage_init_update(void)
{
    // If we have no player info, then request it from the server
    if (!usb_timedout() && !global_hasrequestedinfo)
    {
        global_hasrequestedinfo = TRUE;
        netlib_start(PACKETTYPE_PLAYERINFO);
        netlib_sendtoserver();
    }
    
    // Poll for incoming data
    netlib_poll();
}


/*==============================
    stage00_draw
    Draw the stage
==============================*/

void stage_init_draw(void)
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
    stage_init_cleanup
    Cleans up memory used by the stage
==============================*/

void stage_init_cleanup(void)
{
    text_cleanup();
}


/**************************************************************
                         Net Callbacks
**************************************************************/

void netcallback_playerinfo(size_t size, ClientNumber client)
{

}