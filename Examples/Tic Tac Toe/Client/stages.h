#ifndef TICTACTOE_STAGES_H
#define TICTACTOE_STAGES_H

    /*********************************
                  Macros
    *********************************/

    #define STAGE_COUNT  4

    
    /*********************************
               Enumerations
    *********************************/

    typedef enum {
        STAGE_NONE          = -1,
        STAGE_INIT          = 0,
        STAGE_LOBBY         = 1,
        STAGE_GAME          = 2,
        STAGE_DISCONNECTED  = 3,
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
    
    // Init stage functions
    extern void stage_init_init();
    extern void stage_init_update();
    extern void stage_init_draw();
    extern void stage_init_cleanup();
    extern void stage_init_serverfull();
    
    // Lobby functions
    extern void stage_lobby_init();
    extern void stage_lobby_update();
    extern void stage_lobby_draw();
    extern void stage_lobby_cleanup();
    extern void stage_lobby_playerchange();
    
    // Disconnected functions
    extern void stage_disconnected_init();
    extern void stage_disconnected_update();
    extern void stage_disconnected_draw();
    extern void stage_disconnected_cleanup();
    
    // Disconnected functions
    extern void stage_game_init();
    extern void stage_game_update();
    extern void stage_game_draw();
    extern void stage_game_cleanup();
    
    // Netlib callback functions
    extern void stage_init_serverfull();
    extern void stage_lobby_statechange();
    extern void stage_lobby_playerchange();
    extern void stage_game_statechange();
    extern void stage_game_playerturn();
    extern void stage_game_makemove();
    extern void stage_game_playercursor();
    extern void stage_game_boardcompleted();
    
#endif