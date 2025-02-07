#ifndef REALTIME_OBJECTS_H
#define REALTIME_OBJECTS_H

    #include "mathtypes.h"
    
    
    /*********************************
                 Structs
    *********************************/

    typedef struct {
        u8 connected;
        void* obj;
    } Player;
    
    typedef struct {
        Vector2D pos;
        Vector2D dir;
        Vector2D size;
        int speed;
        Color col;
    } GameObject;

    
    /*********************************
                 Globals
    *********************************/
    
    extern Player global_players[];
    
#endif