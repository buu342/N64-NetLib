#ifndef REALTIME_STAGES_H
#define REALTIME_STAGES_H

    extern void stage_init_init();
    extern void stage_init_update();
    extern void stage_init_draw();
    extern void stage_init_cleanup();
    extern void stage_init_connectpacket();
    extern void stage_init_serverfull();
    extern void stage_init_clockpacket();
    extern void stage_init_playerinfo();
    
#endif