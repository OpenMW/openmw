#ifndef OPENMW_ESM_ANIMATIONSTATE_H
#define OPENMW_ESM_ANIMATIONSTATE_H

#include <string>
#include <vector>
#include <stdint.h>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only
    struct AnimationState
    {
        struct ScriptedAnimation
        {
            ScriptedAnimation()
                : mTime(0.f), mAbsolute(false), mLoopCount(0) {}

            std::string mGroup;
            float mTime;
            bool mAbsolute;
            uint64_t mLoopCount;
        };

        typedef std::vector<ScriptedAnimation> ScriptedAnimations;
        ScriptedAnimations mScriptedAnims;

        bool empty() const;

        void load(ESMReader& esm);
        void save(ESMWriter& esm) const;
    };
}

#endif
