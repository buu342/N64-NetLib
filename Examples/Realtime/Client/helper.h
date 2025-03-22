#ifndef REALTIME_HELPER_H
#define REALTIME_HELPER_H
    

    /*********************************
                Functions
    *********************************/

    extern void rcp_init();
    extern void fb_clear(u8 r, u8 g, u8 b);
    extern int timecompare(const void* a, const void* b);
    extern f32 flerp(f32 a, f32 b, f32 f);

#endif