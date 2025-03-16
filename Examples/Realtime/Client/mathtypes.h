#ifndef REALTIME_MATH_H
#define REALTIME_MATH_H
    
    
    /*********************************
                 Structs
    *********************************/

    typedef struct {
        float x;
        float y;
    } Vector2D;
    
    typedef struct {
        u8 r;
        u8 g;
        u8 b;
        u8 a;
    } Color;
    
    Vector2D vector_normalize(Vector2D v);
    
#endif