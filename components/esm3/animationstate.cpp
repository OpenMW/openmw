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
            esm.getHNT(anim.mLoopCount, "COUN");

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
