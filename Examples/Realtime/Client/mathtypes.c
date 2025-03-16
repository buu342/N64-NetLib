#include <ultra64.h>
#include "mathtypes.h"

Vector2D vector_normalize(Vector2D v)
{
    float mag = sqrtf(v.x*v.x + v.y*v.y);
    if (mag == 0) {
        v.x = 0;
        v.y = 0;
    } else {
        mag = 1.0f/mag;
        v.x = v.x*mag;
        v.y = v.y*mag;
    }
    return v;
}