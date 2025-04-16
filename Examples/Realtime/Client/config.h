#ifndef REALTIME_CONFIG_H
#define REALTIME_CONFIG_H

    /*********************************
           Configuration Macros
    *********************************/

    // TV Types
    #define NTSC    0
    #define PAL     1
    #define MPAL    2
    
    // TV setting
    #define TV_TYPE NTSC

    // Rendering resolution
    #define SCREEN_WD 320
    #define SCREEN_HT 240

    // Array sizes
    #define GLIST_LENGTH 4096
    #define HEAP_LENGTH  1024

    // In a real game, you would have the server send this value during connection
    // I'm forgoing that just for simplification reasons
    #define SERVERTICKRATE  5.0f
    #define DELTATIME       1.0f/((float)SERVERTICKRATE)
    
    // Interpolation
    #define VIEWLAG      0.4f // Time (in seconds) that the view should be behind the server's
    #define TICKSTOKEEP  5 // Should be at least 2, and enough to cover VIEWLAG
    
    // Controller config
    #define MAXSTICK 80
    #define MINSTICK 5
    
    
    /*********************************
                 Globals
    *********************************/
    
    extern Gfx glist[];
    extern Gfx* glistp;
    
    extern Gfx rspinit_dl[];
    extern Gfx rdpinit_dl[];
    
    extern NUContData contdata[1];
    
#endif