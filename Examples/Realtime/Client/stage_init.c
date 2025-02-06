/***************************************************************
                          stage_init.c
                               
Handles the initial connection of the client to the server.
***************************************************************/

#include <nusys.h>
#include "config.h"
#include "netlib.h"
#include "packets.h"
#include "helper.h"
#include "stages.h"
#include "text.h"

#define CLOCKCOUNT 5

typedef enum {
    CONNSTATE_UNCONNECTED,
    CONNSTATE_SYNCING,
    CONNSTATE_PLYINFO,
    CONNSTATE_CONNECTED,
} ConnectState;

static ConnectState global_connectionstate;

static u8 global_clocksdone;
static bool global_clocksent;
static OSTime global_latencytests[CLOCKCOUNT];
static u64 global_clocktimes[CLOCKCOUNT];


/*==============================
    stage_init_init
    Initialize the stage
==============================*/

void stage_init_init(void)
{
    global_connectionstate = CONNSTATE_UNCONNECTED;

    // Generate the wait text
    text_setfont(&font_default);
    text_setalign(ALIGN_CENTER);
    text_setcolor(255, 255, 255, 255);
    text_create("Connecting to Server...", SCREEN_WD/2, SCREEN_HT/2 - 64);
    text_setfont(&font_small);
    text_create("Ensure the USB is connected", SCREEN_WD/2, SCREEN_HT/2 + 16);
    text_create("and the client is running", SCREEN_WD/2, SCREEN_HT/2 + 32);
    
    // Initialize the clock data
    global_clocksdone = 0;
    global_clocksent = FALSE;
    
    // Tell the server we're connecting
    netlib_start(PACKETID_CLIENTCONNECT);
    netlib_sendtoserver();
}


/*==============================
    stage_init_update
    Update stage variables every frame
==============================*/

void stage_init_update(void)
{
    switch (global_connectionstate)
    {
        case CONNSTATE_SYNCING:
            if (global_clocksdone == CLOCKCOUNT)
            {
                // Finished, send a sync done packet so that we can receive player data
                netlib_start(PACKETID_DONESYNC);
                netlib_sendtoserver();
                global_connectionstate = CONNSTATE_PLYINFO;
            }
            else if (!global_clocksent)
            {
                // Send a clock request packet
                netlib_start(PACKETID_CLOCKSYNC);
                netlib_sendtoserver();
                global_clocksent = TRUE;
                global_latencytests[global_clocksdone] = osGetTime();
            }
            break;
    }
}


/*==============================
    stage_init_draw
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


/*==============================
    stage_init_connectpacket
    Handles what happens when the player receives the connect
    packet from the server
==============================*/

void stage_init_connectpacket(void)
{
    global_connectionstate = CONNSTATE_SYNCING;
    
    // Print to the screen
    text_cleanup();
    text_setfont(&font_default);
    text_setalign(ALIGN_CENTER);
    text_setcolor(255, 255, 255, 255);
    text_create("Connecting to server...", SCREEN_WD/2, SCREEN_HT/2);
}


/*==============================
    stage_init_serverfull
    Handles what happens when the player receives the server full
    packet from the server
==============================*/

void stage_init_serverfull(void)
{
    text_cleanup();
    text_setfont(&font_default);
    text_setalign(ALIGN_CENTER);
    text_setcolor(255, 255, 255, 255);
    text_create("Unable to connect", SCREEN_WD/2, SCREEN_HT/2 - 64);
    text_create("Server is full", SCREEN_WD/2, SCREEN_HT/2);
}


/*==============================
    stage_init_clockpacket
    Handles what happens when the player receives the clock
    packet from the server
==============================*/

void stage_init_clockpacket(void)
{
    u64 clocktime;
    netlib_readqword(&clocktime);
    global_clocksent = FALSE;
    global_latencytests[global_clocksdone] = osGetTime() - global_latencytests[global_clocksdone];
    global_clocktimes[global_clocksdone] = clocktime;
    global_clocksdone++;
}


/*==============================
    stage_init_playerinfo
    Handles what happens when the player receives the player info
    packet from the server
==============================*/

void stage_init_playerinfo(void)
{
    // TODO:
}