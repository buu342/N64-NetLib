/***************************************************************
                          stage_init.c
                               
Handles the initial connection of the client to the server.
This differs from the TicTacToe game because we need to sync the
game clocks between the client and server, as we have to 
timestamp packets for reconciliation.
***************************************************************/

#include <nusys.h>
#include "config.h"
#include "netlib.h"
#include "packets.h"
#include "helper.h"
#include "stages.h"
#include "text.h"

    
/*********************************
              Macros
*********************************/

#define CLOCKCOUNT 5

    
/*********************************
           Enumerations
*********************************/

typedef enum {
    CONNSTATE_UNCONNECTED,
    CONNSTATE_SYNCING,
    CONNSTATE_PLYINFO,
    CONNSTATE_CONNECTED,
} ConnectState;

    
/*********************************
             Globals
*********************************/

// Connection state
static ConnectState global_connectionstate;

// Server clock estimation
static u8 global_clocksdone;
static bool global_clocksent;
static OSTime global_latencytests[CLOCKCOUNT];


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
    text_create("Establishing connection", SCREEN_WD/2, SCREEN_HT/2 - 64);
    text_create("to server...", SCREEN_WD/2, SCREEN_HT/2 - 44);
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

void stage_init_update(float dt)
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
                int i;
                
                // Burn some time before sending another clock sync
                for (i=0; i<10000000; i++)
                    asm(""); // Prevent GCC from optimizing this loop away
                    
                // Send a clock request packet
                global_latencytests[global_clocksdone] = osGetTime();
                netlib_start(PACKETID_CLOCKSYNC);
                netlib_sendtoserver();
                global_clocksent = TRUE;
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
    OSTime curtime = osGetTime();
    netlib_readqword(&clocktime);
    global_clocksent = FALSE;
    global_latencytests[global_clocksdone] = (curtime - global_latencytests[global_clocksdone])/2;
    global_clocksdone++;
    
    // Once we have enough samples, calculate the median sample
    if (global_clocksdone == CLOCKCOUNT)
    {
        int i, samplecount;
        OSTime median, mean, stdev, final;
        
        // Sort the list of latencies from lowest to highest, then select the median
        qsort(global_latencytests, CLOCKCOUNT, sizeof(OSTime), timecompare);
        median = global_latencytests[CLOCKCOUNT/2];
        
        // Calculate the standard deviation
        mean = 0;
        for (i=0; i<CLOCKCOUNT; i++)
            mean += global_latencytests[i];
        mean *= 1.0f/((float)CLOCKCOUNT);
        stdev = 0;
        for (i=0; i<CLOCKCOUNT; i++)
            stdev += powf(global_latencytests[i] - mean, 2);
        stdev *= 1.0f/((float)CLOCKCOUNT);
        stdev = sqrtf(stdev);
        
        // Ignore samples that are 1 standard deviation from the median, and average the result
        final = 0;
        samplecount = 0;
        for (i=0; i<CLOCKCOUNT; i++)
        {
            if (global_latencytests[i] < median + stdev)
            {
                final += global_latencytests[i];
                samplecount++;
            }
        }
        final /= (float)samplecount;
        
        // Set the OS Time to the clock time from the server + the calculated latency + time taken to do all the above math
        osSetTime(OS_NSEC_TO_CYCLES(clocktime) + final + (osGetTime() - curtime));
    }
}