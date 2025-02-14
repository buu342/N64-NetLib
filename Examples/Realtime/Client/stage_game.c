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
#include "objects.h"

#define INPUTRATE 15.0f

static NUContData global_contdata;

static OSTime global_nextsend;

static bool global_prediction;
static bool global_reconciliation;
static bool global_interpolation;


/*==============================
    stage_game_init
    Initialize the stage
==============================*/

void stage_game_init(void)
{
    global_nextsend = osGetTime();
    global_prediction = TRUE;
    global_reconciliation = TRUE;
    global_interpolation = TRUE;
}


/*==============================
    stage_game_update
    Update stage variables every frame
==============================*/

void stage_game_update(void)
{
    const s8 MAXSTICK = 80;
    const s8 MINSTICK = 5;
    
    // Get controller data and apply some basic deadzoning to it
    nuContDataGetEx(&global_contdata, 0);
    if (global_contdata.stick_x < MINSTICK && global_contdata.stick_x > -MINSTICK)
        global_contdata.stick_x = 0;
    else if (global_contdata.stick_x > MAXSTICK || global_contdata.stick_x < -MAXSTICK)
        global_contdata.stick_x = MAXSTICK;
    if (global_contdata.stick_y < MINSTICK && global_contdata.stick_y > -MINSTICK)
        global_contdata.stick_y = 0;
    else if (global_contdata.stick_y > MAXSTICK || global_contdata.stick_y < -MAXSTICK)
        global_contdata.stick_y = MAXSTICK;
    
    // Send the client input to the server every 15hz (if you do too high a rate, you risk flooding the USB/router)
    if (global_nextsend < osGetTime())
    {
        netlib_start(PACKETID_CLIENTINPUT);
            netlib_writeqword((u64)osGetTime());
            netlib_writebyte((u8)global_contdata.stick_x);
            netlib_writebyte((u8)global_contdata.stick_y);
        netlib_sendtoserver();
        global_nextsend = osGetTime() + OS_USEC_TO_CYCLES((u64)(1000000.0f*(1.0f/INPUTRATE)));
    }
    
    // Predict the player movement before the server acknowledges the input
    if (global_prediction)
    {
    
    }
}


/*==============================
    stage_game_draw
    Draw the stage
==============================*/

void stage_game_draw(void)
{
    int i;
    glistp = glist;

    // Initialize the RCP and framebuffer
    rcp_init();
    fb_clear(100, 100, 100);
    
    // Render stuff
    objects_draw();
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
    int i;
    
    // Destroy all objects
    objects_destroyall();
    
    // Cleanup player structs
    for (i=0; i<MAXPLAYERS; i++)
    {
        global_players[i].connected = FALSE;
        global_players[i].obj = NULL;
    }
    
    // Other cleanup
    text_cleanup();
}