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
        T mMod;
        T mDamage;
        float mProgress;

        StatState();

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };

    template<typename T>
    StatState<T>::StatState() : mBase (0), mMod (0), mDamage (0), mProgress (0) {}

    template<typename T>
    void StatState<T>::load (ESMReader &esm)
    {
        esm.getHNT (mBase, "STBA");
        esm.getHNT (mMod, "STMO");
        mDamage = 0;
        esm.getHNOT (mDamage, "STDA");
        mProgress = 0;
        esm.getHNOT (mProgress, "STPR");
    }

    template<typename T>
    void StatState<T>::save (ESMWriter &esm) const
    {
        esm.writeHNT ("STBA", mBase);
        esm.writeHNT ("STMO", mMod);

        if (mDamage)
            esm.writeHNT ("STDA", mDamage);

        if (mProgress)
            esm.writeHNT ("STPR", mProgress);
    }
}

#endif