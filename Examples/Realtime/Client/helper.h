#ifndef REALTIME_HELPER_H
#define REALTIME_HELPER_H

    /*********************************
                  Macros
    *********************************/
    
    #define CLAMP(val, min, max) (MAX(min, MIN(max, val)))
    
    #define SEC_TO_USEC(a)   (((f64)a)*1000000.0f)
    #define MSEC_TO_USEC(a)  (((f64)a)*1000.0f)
    #define USEC_TO_MSEC(a)  (((f64)a)/1000.0f)
    #define USEC_TO_SEC(a)   (((f64)a)/1000000.0f)
    

    /*********************************
                Functions
    *********************************/

    extern void rcp_init();
    extern void fb_clear(u8 r, u8 g, u8 b);
    extern int timecompare(const void* a, const void* b);
    extern f32 flerp(f32 a, f32 b, f32 f);

#endif