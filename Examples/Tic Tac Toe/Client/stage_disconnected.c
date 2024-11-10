/***************************************************************
                      stage_disconnected.c
                               
Room that shows you have been disconnected from the game
***************************************************************/

#include <nusys.h>
#include "config.h"
#include "netlib.h"
#include "packets.h"
#include "helper.h"
#include "text.h"


/*==============================
    stage_disconnected_init
    Initialize the stage
==============================*/

void stage_disconnected_init(void)
{
    text_setfont(&font_default);
    text_setalign(ALIGN_CENTER);
    text_setcolor(255, 255, 255, 255);
    text_create("You have been\ndisconnected", SCREEN_WD/2, SCREEN_HT/2 - 64);
}


/*==============================
    stage_disconnected_update
    Update stage variables every frame
==============================*/

void stage_disconnected_update(void)
{
    // Nothing to do
}


/*==============================
    stage_disconnected_draw
    Draw the stage
==============================*/

void stage_disconnected_draw(void)
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
    stage_disconnected_cleanup
    Cleans up memory used by the stage
==============================*/

void stage_disconnected_cleanup(void)
{
    text_cleanup();
}