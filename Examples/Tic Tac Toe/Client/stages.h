#ifndef TICTACTOE_STAGES_H
#define TICTACTOE_STAGES_H

    #define STAGE_COUNT  3

    typedef enum {
        STAGE_NONE  = -1,
        STAGE_INIT  = 0,
        STAGE_LOBBY = 1,
        STAGE_GAME  = 2,
    } StageNum;

    typedef struct {
        void (*funcptr_init)(void);
        void (*funcptr_update)(void);
        void (*funcptr_draw)(void);
        void (*funcptr_cleanup)(void);
    } StageDef;


    /*********************************
            Function Prototypes
    *********************************/

    StageNum stages_getcurrent();
    void stages_changeto(StageNum num);
    
    // Init stage functions
    void stage_init_init();
    void stage_init_update();
    void stage_init_draw();
    void stage_init_cleanup();
    void stage_init_serverfull();
    
    // Stage functions
    void stage_lobby_init();
    void stage_lobby_update();
    void stage_lobby_draw();
    void stage_lobby_cleanup();
    void stage_lobby_playerchange();
    
#endif
    