#ifndef OPENMW_MWPHYSICS_CONSTANTS_H
#define OPENMW_MWPHYSICS_CONSTANTS_H

namespace MWPhysics
{
    static const float sStepSizeUp = 34.0f;
    static const float sStepSizeDown = 62.0f;

    static const float sMinStep = 10.0f; // hack to skip over tiny unwalkable slopes
    static const float sMinStep2 = 20.0f; // hack to skip over shorter but longer/wider/further unwalkable slopes
    // whether to do the above stairstepping logic hacks to work around bad morrowind assets - disabling causes problems but improves performance
    static const bool sDoExtraStairHacks = true;

    static const float sGroundOffset = 1.0f;
    static const float sMaxSlope = 49.0f;

    // Arbitrary number. To prevent infinite loops. They shouldn't happen but it's good to be prepared.
    static const int sMaxIterations = 8;
    // Allows for more precise movement solving without getting stuck or snagging too easily.
    static const float sCollisionMargin = 0.1;
    // Allow for a small amount of penetration to prevent numerical precision issues from causing the "unstuck"ing code to run unnecessarily
    // Currently set to 0 because having the "unstuck"ing code run whenever possible prevents some glitchy snagging issues
    static const float sAllowedPenetration = 0.0;
}

#endif
