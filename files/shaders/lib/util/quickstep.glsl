#ifndef LIB_UTIL_QUICKSTEP
#define LIB_UTIL_QUICKSTEP

float quickstep(float x)
{
    x = clamp(x, 0.0, 1.0);
    x = 1.0 - x * x;
    x = 1.0 - x * x;
    return x;
}

#endif
