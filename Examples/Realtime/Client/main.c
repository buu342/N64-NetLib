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
#include "objects.h"


/*********************************
        Function Prototypes
*********************************/

static void callback_prenmi();
static void callback_vsync(int tasksleft);
static void stagetable_init();
static void callback_disconnect();


/*********************************
             Globals
*********************************/

// Half a megabyte of heap memory
char heapmem[1024*512];

// Stage globals
volatile StageNum global_curstage = STAGE_INIT;
volatile StageNum global_nextstage = STAGE_NONE;
StageDef global_stagetable[STAGE_COUNT];


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
    
    // Initialize the stage table
    stagetable_init();
    
    // Initialize the object system
    objects_initsystem();
    
    // Initialize the net library
    netlib_initialize();
    netcallback_initall();
    //netlib_callback_disconnect(1000*5, callback_disconnect);

    // Initialize the font system
    text_initialize();
    
    // Game loop
    while(1)
    {
        global_stagetable[global_curstage].funcptr_init();
    
        // Set callback functions for reset and graphics
        nuGfxFuncSet((NUGfxFunc)callback_vsync);
        
        // Turn on the screen and loop forever to keep the idle thread busy
        nuGfxDisplayOn();
        
        // Wait while the stage hasn't changed
        while (global_nextstage == STAGE_NONE)
            ;
        
        // Stop
        nuGfxFuncRemove();
        nuGfxDisplayOff();
        
        // Cleanup and change the stage function pointer
        global_stagetable[global_curstage].funcptr_cleanup();
        global_curstage = global_nextstage;
        global_nextstage = STAGE_NONE;
    }
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
    global_stagetable[global_curstage].funcptr_update();
    
    // Draw it
    if (tasksleft < 1 && global_nextstage == STAGE_NONE)
        global_stagetable[global_curstage].funcptr_draw();
}


/*==============================
    stagetable_init
    Initialize the stage table
==============================*/

static void stagetable_init()
{
    global_stagetable[STAGE_INIT].funcptr_init = &stage_init_init;
    global_stagetable[STAGE_INIT].funcptr_update = &stage_init_update;
    global_stagetable[STAGE_INIT].funcptr_draw = &stage_init_draw;
    global_stagetable[STAGE_INIT].funcptr_cleanup = &stage_init_cleanup;
    
    global_stagetable[STAGE_GAME].funcptr_init = &stage_game_init;
    global_stagetable[STAGE_GAME].funcptr_update = &stage_game_update;
    global_stagetable[STAGE_GAME].funcptr_draw = &stage_game_draw;
    global_stagetable[STAGE_GAME].funcptr_cleanup = &stage_game_cleanup;
    
    global_stagetable[STAGE_DISCONNECTED].funcptr_init = &stage_disconnected_init;
    global_stagetable[STAGE_DISCONNECTED].funcptr_update = &stage_disconnected_update;
    global_stagetable[STAGE_DISCONNECTED].funcptr_draw = &stage_disconnected_draw;
    global_stagetable[STAGE_DISCONNECTED].funcptr_cleanup = &stage_disconnected_cleanup;
}


/*==============================
    stages_changeto
    Changes the stage
    @param The stage number
==============================*/

void stages_changeto(StageNum num)
{
    if (num == STAGE_NONE)
        return;
    global_nextstage = num;
}


/*==============================
    stages_getcurrent
    Gets the current stage
    @return The stage number
==============================*/

StageNum stages_getcurrent()
{
    return global_curstage;
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


/*==============================
    callback_disconnect
    Callback function for when the player disconnects
==============================*/

static void callback_disconnect()
{
    netlib_setclient(0);
    stages_changeto(STAGE_DISCONNECTED);
}