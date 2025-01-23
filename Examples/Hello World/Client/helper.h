#ifndef HELLOSERVER_HELPER_H
#define HELLOSERVER_HELPER_H
    
    
    /*********************************
                 Structs
    *********************************/

    typedef struct {
        u8 connected;
        u8 ready;
    } Player;

    
    /*********************************
                 Globals
    *********************************/
    
    extern Player global_players[];
    

    /*********************************
                Functions
    *********************************/

    extern void rcp_init();
    extern void fb_clear(u8 r, u8 g, u8 b);

#endif