/***************************************************************
                          stage_hello.c
                               
Handles the main level of the game.
***************************************************************/

#include <nusys.h>
#include "config.h"
#include "netlib.h"
#include "packets.h"
#include "helper.h"
#include "stages.h"
#include "text.h"


/*==============================
    stage_hello_init
    Initialize the stage
==============================*/

void stage_hello_init(void)
{
    // Generate the wait text
    text_setfont(&font_default);
    text_setalign(ALIGN_CENTER);
    text_setcolor(255, 255, 255, 255);
    text_create("Connecting to Server...", SCREEN_WD/2, SCREEN_HT/2 - 64);
    text_setfont(&font_small);
    text_create("Ensure the USB is connected", SCREEN_WD/2, SCREEN_HT/2 + 16);
    text_create("and the client is running", SCREEN_WD/2, SCREEN_HT/2 + 32);
    
    // Get player info from the server
    netlib_start(PACKETID_CLIENTCONNECT);
    netlib_sendtoserver();
}


/*==============================
    stage_hello_update
    Update stage variables every frame
==============================*/

void stage_hello_update(void)
{
    // Nothing to do here, the main loop already polls for networking packets
}


/*==============================
    stage_hello_draw
    Draw the stage
==============================*/

void stage_hello_draw(void)
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
    stage_hello_cleanup
    Cleans up memory used by the stage
==============================*/

void stage_hello_cleanup(void)
{
    text_cleanup();
}


/*==============================
    stage_hello_connectpacket
    Handles what happens when the player receives the connect
    packet from the server
==============================*/

void stage_hello_connectpacket(void)
{
    char textbuff[32];
    u32 read;
    
    // Read the magic value from the packet
    netlib_readdword(&read);
    
    // Print to the screen
    text_cleanup();
    text_setfont(&font_default);
    text_setalign(ALIGN_CENTER);
    text_setcolor(255, 255, 255, 255);
    text_create("Server replied back with:", SCREEN_WD/2, SCREEN_HT/2 - 64);
    sprintf(textbuff, "%d", read);
    text_create(textbuff, SCREEN_WD/2, SCREEN_HT/2 + 16);
}