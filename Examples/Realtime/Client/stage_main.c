/***************************************************************
                          stage_main.c
                               
TODO
***************************************************************/

#include <nusys.h>
#include "config.h"
#include "netlib.h"
#include "packets.h"
#include "helper.h"
#include "stages.h"
#include "text.h"


/*==============================
    stage_main_init
    Initialize the stage
==============================*/

void stage_main_init(void)
{

}


/*==============================
    stage_main_update
    Update stage variables every frame
==============================*/

void stage_main_update(void)
{

}


/*==============================
    stage_main_draw
    Draw the stage
==============================*/

void stage_main_draw(void)
{
    glistp = glist;

    // Initialize the RCP and framebuffer
    rcp_init();
    fb_clear(100, 100, 100);

    // Render some text
    text_render();

    // Finish
    gDPFullSync(glistp++);
    gSPEndDisplayList(glistp++);
    nuGfxTaskStart(glist, (s32)(glistp - glist) * sizeof(Gfx), NU_GFX_UCODE_F3DEX, NU_SC_SWAPBUFFER);
}


/*==============================
    stage_main_cleanup
    Cleans up memory used by the stage
==============================*/

void stage_main_cleanup(void)
{
    text_cleanup();
}