#ifndef OPENMW_ESM_STATSTATE_H
#define OPENMW_ESM_STATSTATE_H

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
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

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };

    template<typename T>
    StatState<T>::StatState() : mBase (0), mMod (0), mCurrent (0), mDamage (0), mProgress (0) {}

    template<typename T>
    void StatState<T>::load (ESMReader &esm)
    {
        esm.getHNT (mBase, "STBA");

        mMod = 0;
        esm.getHNOT (mMod, "STMO");
        mCurrent = 0;
        esm.getHNOT (mCurrent, "STCU");

        // mDamage was changed to a float; ensure backwards compatibility
        T oldDamage = 0;
        esm.getHNOT(oldDamage, "STDA");
        mDamage = static_cast<float>(oldDamage);

        esm.getHNOT (mDamage, "STDF");

        mProgress = 0;
        esm.getHNOT (mProgress, "STPR");
    }

    template<typename T>
    void StatState<T>::save (ESMWriter &esm) const
    {
        esm.writeHNT ("STBA", mBase);

        if (mMod != 0)
            esm.writeHNT ("STMO", mMod);

        if (mCurrent)
            esm.writeHNT ("STCU", mCurrent);

        if (mDamage)
            esm.writeHNT ("STDF", mDamage);

        if (mProgress)
            esm.writeHNT ("STPR", mProgress);
    }
}

#endif
