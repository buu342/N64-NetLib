#ifndef REALTIME_STAGES_H
#define REALTIME_STAGES_H

    #include "objects.h"
    

    /*********************************
                  Macros
    *********************************/

    #define STAGE_COUNT  3

    
    /*********************************
               Enumerations
    *********************************/

    typedef enum {
        STAGE_NONE          = -1,
        STAGE_INIT          = 0,
        STAGE_GAME          = 1,
        STAGE_DISCONNECTED  = 2,
    } StageNum;
    

    /*********************************
                 Structs
    *********************************/

    typedef struct {
        void (*funcptr_init)(void);
        void (*funcptr_update)(float dt);
        void (*funcptr_fixedupdate)(float dt);
        void (*funcptr_draw)(void);
        void (*funcptr_cleanup)(void);
    } StageDef;


    /*********************************
            Function Prototypes
    *********************************/

    extern StageNum stages_getcurrent();
    extern void stages_changeto(StageNum num);

    extern void stage_init_init();
    extern void stage_init_update(float dt);
    extern void stage_init_draw();
    extern void stage_init_cleanup();
    extern void stage_init_connectpacket();
    extern void stage_init_serverfull();
    extern void stage_init_clockpacket();

    extern void stage_game_init();
    extern void stage_game_update(float dt);
    extern void stage_game_fixedupdate(float dt);
    extern void stage_game_draw();
    extern void stage_game_cleanup();
    extern void stage_game_ackinput(OSTime time, u8 reconcile);

    extern void stage_disconnected_init();
    extern void stage_disconnected_update(float dt);
    extern void stage_disconnected_draw();
    extern void stage_disconnected_cleanup();
    
#endif