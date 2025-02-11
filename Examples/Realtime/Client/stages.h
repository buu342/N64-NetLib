#ifndef REALTIME_STAGES_H
#define REALTIME_STAGES_H

    /*********************************
                  Macros
    *********************************/

    #define STAGE_COUNT  2

    
    /*********************************
               Enumerations
    *********************************/

    typedef enum {
        STAGE_NONE          = -1,
        STAGE_INIT          = 0,
        STAGE_GAME          = 1,
    } StageNum;
    

    /*********************************
                 Structs
    *********************************/

    typedef struct {
        void (*funcptr_init)(void);
        void (*funcptr_update)(void);
        void (*funcptr_draw)(void);
        void (*funcptr_cleanup)(void);
    } StageDef;


    /*********************************
            Function Prototypes
    *********************************/

    extern StageNum stages_getcurrent();
    extern void stages_changeto(StageNum num);

    extern void stage_init_init();
    extern void stage_init_update();
    extern void stage_init_draw();
    extern void stage_init_cleanup();
    extern void stage_init_connectpacket();
    extern void stage_init_serverfull();
    extern void stage_init_clockpacket();

    extern void stage_game_init();
    extern void stage_game_update();
    extern void stage_game_draw();
    extern void stage_game_cleanup();
    extern void stage_game_createobject();
    extern void stage_game_updateobject(size_t size);
    
#endif