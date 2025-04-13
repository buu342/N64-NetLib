#ifndef REALTIME_OBJECTS_H
#define REALTIME_OBJECTS_H

    #include <nusys.h>
    #include "config.h"
    #include "datastructs.h"
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
        Vector2D oldpos[CLIENTLAG];
        Vector2D pos;
        Vector2D dir;
        Vector2D size;
        float speed;
        Color col;
        u8 bounce;
        OSTime lastupdate;
    } GameObject;

    
    /*********************************
                 Globals
    *********************************/
    
    extern Player global_players[];


    /*********************************
            Function Prototypes
    *********************************/
    
    extern void objects_initsystem();
    
    extern GameObject* objects_create();
    extern GameObject* objects_findbyid(u32 id);
    extern linkedList* objects_getall();
    extern void        objects_destroy(GameObject* obj);
    extern void        objects_destroyall();
    
    extern void objects_connectplayer(u8 num, GameObject* obj);
    extern void objects_disconnectplayer(u8 num);
    
    extern void objects_applycont(GameObject* obj, NUContData contdata);
    extern void objects_applyphys(GameObject* obj, float dt);
    
#endif