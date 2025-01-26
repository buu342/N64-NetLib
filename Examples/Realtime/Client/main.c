/***************************************************************
                            main.c
                               
Program entrypoint.
***************************************************************/

#include <nusys.h>
#include <malloc.h>
#include "config.h"
#include "debug.h"
#include "helper.h"
#include "stages.h"
#include "netlib.h"
#include "packets.h"
#include "text.h"


/*********************************
        Function Prototypes
*********************************/

static void callback_prenmi();
static void callback_vsync(int tasksleft);


/*********************************
             Globals
*********************************/

// Half a megabyte of heap memory
char heapmem[1024*512];


/*==============================
    mainproc
    Initializes the game
==============================*/

void mainproc(void)
{
    // Start by selecting the proper television
    if (TV_TYPE == PAL)
    {
        osViSetMode(&osViModeTable[OS_VI_FPAL_LAN1]);
        osViSetYScale(0.833);
        osViSetSpecialFeatures(OS_VI_DITHER_FILTER_ON | OS_VI_GAMMA_OFF | OS_VI_GAMMA_DITHER_OFF | OS_VI_DIVOT_ON);
    }
    else if (TV_TYPE == MPAL)
    {
        osViSetMode(&osViModeTable[OS_VI_MPAL_LAN1]);
        osViSetSpecialFeatures(OS_VI_DITHER_FILTER_ON | OS_VI_GAMMA_OFF | OS_VI_GAMMA_DITHER_OFF | OS_VI_DIVOT_ON);
    }
    
    // Initialize and activate the graphics thread and Graphics Task Manager.
    nuGfxInit();

    // Initialize the heap
    InitHeap(heapmem, sizeof(heapmem));
    
    // Initialize the controller
    nuContInit();
    
    // Initialize the debug library
    debug_initialize();
    
    // Initialize the net library
    netlib_initialize();
    netcallback_initall();

    // Initialize the font system
    text_initialize();
    
    // Initialize the stage
    stage_hello_init();
    
    // Set callback functions for reset and graphics
    nuPreNMIFuncSet((NUScPreNMIFunc)callback_prenmi);
    nuGfxFuncSet((NUGfxFunc)callback_vsync);
    
    // Game loop
    nuGfxDisplayOn();
    while(1)
        ;
}


/*==============================
    callback_vsync
    Code that runs on on the graphics
    thread
    @param The number of tasks left to execute
==============================*/

static void callback_vsync(int tasksleft)
{
    // Poll the net library
    netlib_poll();
    
    // Update the stage
    stage_hello_update();
    
    // Draw it
    if (tasksleft < 1)
        stage_hello_draw();
}


/*==============================
    callback_prenmi
    Code that runs when the reset button
    is pressed. Required to prevent crashes
==============================*/

static void callback_prenmi()
{
    nuGfxDisplayOff();
    osViSetYScale(1);
}