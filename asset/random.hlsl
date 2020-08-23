#ifndef PCL_RANDOM
#define PCL_RANDOM

RWTexture2D<uint> RNGState;

uint loadRNG(int2 xy)
{
    return RNGState[xy];
}

void storeRNG(int2 xy, uint state)
{
    RNGState[xy] = state;
}

// see http://www.reedbeta.com/blog/quick-and-easy-gpu-random-numbers-in-d3d11/
uint wangHash(uint seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

float randomFloat(inout uint state)
{
    //state = 1664525 * state + 1013904223;
    state = wangHash(state);
    return (state & 0x00ffffff) * (1.0 / 0x01000000);
}

#endif // #ifndef PCL_RANDOM
