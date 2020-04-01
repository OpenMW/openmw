#ifndef OPENMW_MWPHYSICS_CONSTANTS_H
#define OPENMW_MWPHYSICS_CONSTANTS_H

namespace MWPhysics
{
    static const float sStepSizeUp = 34.0f;
    static const float sStepSizeDown = 62.0f;
    static const float sMinStep = 10.f;
    static const float sGroundOffset = 1.0f;
    static const float sMaxSlope = 49.0f;

    // Arbitrary number. To prevent infinite loops. They shouldn't happen but it's good to be prepared.
    static const int sMaxIterations = 8;
}

#endif
