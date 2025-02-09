/***************************************************************
                          stage_game.c
                               
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
    stage_game_init
    Initialize the stage
==============================*/

void stage_game_init(void)
{

}


/*==============================
    stage_game_update
    Update stage variables every frame
==============================*/

void stage_game_update(void)
{

}


/*==============================
    stage_game_draw
    Draw the stage
==============================*/

void stage_game_draw(void)
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
    stage_game_cleanup
    Cleans up memory used by the stage
==============================*/

void stage_game_cleanup(void)
{
    text_cleanup();
}