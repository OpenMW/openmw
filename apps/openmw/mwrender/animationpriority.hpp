#ifndef GAME_RENDER_ANIMATIONPRIORITY_H
#define GAME_RENDER_ANIMATIONPRIORITY_H

#include "blendmask.hpp"
#include "bonegroup.hpp"

namespace MWRender
{
    /// Holds an animation priority value for each BoneGroup.
    struct AnimPriority
    {
        /// Convenience constructor, initialises all priorities to the same value.
        AnimPriority(int priority)
        {
            for (unsigned int i = 0; i < sNumBlendMasks; ++i)
                mPriority[i] = priority;
        }

        bool operator==(const AnimPriority& other) const
        {
            for (unsigned int i = 0; i < sNumBlendMasks; ++i)
                if (other.mPriority[i] != mPriority[i])
                    return false;
            return true;
        }

        int& operator[](BoneGroup n) { return mPriority[n]; }

        const int& operator[](BoneGroup n) const { return mPriority[n]; }

        bool contains(int priority) const
        {
            for (unsigned int i = 0; i < sNumBlendMasks; ++i)
                if (priority == mPriority[i])
                    return true;
            return false;
        }

        int mPriority[sNumBlendMasks];
    };
}
#endif
