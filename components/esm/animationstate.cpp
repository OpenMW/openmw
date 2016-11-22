#include "animationstate.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
    bool AnimationState::empty() const
    {
        return mScriptedAnims.empty();
    }

    void AnimationState::load(ESMReader& esm)
    {
        mScriptedAnims.clear();

        while (esm.isNextSub("ANIS"))
        {
            ScriptedAnimation anim;

            anim.mGroup = esm.getHString();
            esm.getHNOT(anim.mTime, "TIME");
            esm.getHNOT(anim.mAbsolute, "ABST");

            esm.getSubNameIs("COUN");
            // workaround bug in earlier version where size_t was used
            esm.getSubHeader();
            if (esm.getSubSize() == 8)
                esm.getT(anim.mLoopCount);
            else
            {
                uint32_t loopcount;
                esm.getT(loopcount);
                anim.mLoopCount = (uint64_t) loopcount;
            }

            mScriptedAnims.push_back(anim);
        }
    }

    void AnimationState::save(ESMWriter& esm) const
    {
        for (ScriptedAnimations::const_iterator iter = mScriptedAnims.begin(); iter != mScriptedAnims.end(); ++iter)
        {
            esm.writeHNString("ANIS", iter->mGroup);
            if (iter->mTime > 0)
                esm.writeHNT("TIME", iter->mTime);
            if (iter->mAbsolute)
                esm.writeHNT("ABST", iter->mAbsolute);
            esm.writeHNT("COUN", iter->mLoopCount);
        }
    }
}
