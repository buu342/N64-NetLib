#ifndef TICTACTOE_HELPER_H
#define TICTACTOE_HELPER_H

    typedef struct {
        u8 connected;
        u8 ready;
    } Player;
    
    extern Player global_players[];
    

    /*********************************
                Functions
    *********************************/

    void rcp_init();
    void fb_clear(u8 r, u8 g, u8 b);

#endif