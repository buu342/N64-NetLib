/***************************************************************
                            main.c
                               
Program entrypoint.
***************************************************************/

#include <nusys.h>
#include <malloc.h>
#include "config.h"
#include "debug.h"
#include "stages.h"
#include "netlib.h"
#include "text.h"


/*********************************
        Function Prototypes
*********************************/

static void callback_prenmi();
static void callback_vsync(int tasksleft);
static void stagetable_init();


/*********************************
             Globals
*********************************/

StageNum global_curstage = STAGE_INIT;
StageNum global_nextstage = STAGE_NONE;
StageDef global_stagetable[STAGE_COUNT];

// Controller data
NUContData contdata[1];

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
    
    // Initialize the controller
    nuContInit();
    
    // Initialize the stage table
    stagetable_init();
    
    // Initialize the net library
    netlib_initialize();

    // Initialize the font system
    text_initialize();

    // Initialize the heap
    InitHeap(heapmem, sizeof(heapmem));
    
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
    // Update the stage, then draw it when the RCP is ready
    global_stagetable[global_curstage].funcptr_update();
    if (tasksleft < 1)
        global_stagetable[global_curstage].funcptr_draw();
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
    stages_changeto
    Changes the stage
    @param The stage number
==============================*/

void stages_changeto(StageNum num)
{
    if (num == STAGE_NONE)
        return;
    global_curstage = num;
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
}