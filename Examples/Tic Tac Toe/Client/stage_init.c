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
    PACKETTYPE_SERVERINFO = 0,
} NetPacketType;

void netcallback_serverinfo(size_t size, ClientNumber client);


/*==============================
    stage_init_init
    Initialize the stage
==============================*/

void stage_init_init(void)
{
    netlib_register(PACKETTYPE_SERVERINFO, &netcallback_serverinfo);
    
    // Generate the wait text
    text_setfont(&font_default);
    text_setalign(ALIGN_CENTER);
    text_setcolor(255, 255, 255, 255);
    text_create("Waiting for Server...", SCREEN_WD/2, SCREEN_HT/2 - 64);
    text_setfont(&font_small);
    text_create("Ensure the USB is connected", SCREEN_WD/2, SCREEN_HT/2 + 16);
    text_create("and the client is running", SCREEN_WD/2, SCREEN_HT/2 + 32);
}


/*==============================
    stage_init_update
    Update stage variables every frame
==============================*/

void stage_init_update(void)
{
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

void netcallback_serverinfo(size_t size, ClientNumber client)
{

}