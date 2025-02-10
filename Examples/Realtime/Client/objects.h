#ifndef REALTIME_OBJECTS_H
#define REALTIME_OBJECTS_H

    #include "mathtypes.h"

    #define MAXPLAYERS 32
    
    
    /*********************************
                 Structs
    *********************************/

    typedef struct {
        u8 connected;
        void* obj;
    } Player;
    
    typedef struct {
        u32 id;
        Vector2D pos;
        Vector2D dir;
        Vector2D size;
        s32 speed;
        Color col;
    } GameObject;

    
    /*********************************
                 Globals
    *********************************/
    
    extern Player global_players[];


    /*********************************
            Function Prototypes
    *********************************/
    
    extern void objects_initsystem();
    
    extern void objects_connectplayer(u8 num, GameObject* obj);
    extern void objects_disconnectplayer(u8 num);
    
    extern GameObject* objects_create();
    extern void        objects_destroy(GameObject* obj);
    
#endif