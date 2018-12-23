#ifndef OPENMW_ESM_STATSTATE_H
#define OPENMW_ESM_STATSTATE_H

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only

    template<typename T>
    struct StatState
    {
        T mBase;
        T mMod; // Note: can either be the modifier, or the modified value.
                // A bit inconsistent, but we can't fix this without breaking compatibility.
        T mCurrent;
        float mDamage;
        float mProgress;

        StatState();

        void load (ESMReader &esm, bool intFallback = false);
        void save (ESMWriter &esm) const;
    };
}

#endif
