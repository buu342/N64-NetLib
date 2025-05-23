#ifndef TICTACTOE_CONFIG_H
#define TICTACTOE_CONFIG_H

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
    #define HEAP_LENGTH  1024*512
    
    
    /*********************************
                 Globals
    *********************************/
    
    extern Gfx glist[];
    extern Gfx* glistp;
    
    extern Gfx rspinit_dl[];
    extern Gfx rdpinit_dl[];
    
    extern NUContData contdata[1];
    
#endif