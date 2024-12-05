#include "importproj.h"

#include <components/esm3/esmreader.hpp>

namespace ESSImport
{

    void ESSImport::PROJ::load(ESM::ESMReader& esm)
    {
        while (esm.isNextSub("PNAM"))
        {
            PNAM pnam;
            esm.getHT(pnam.mAttackStrength, pnam.mSpeed, pnam.mUnknown, pnam.mFlightTime, pnam.mSplmIndex,
                pnam.mUnknown2, pnam.mVelocity.mValues, pnam.mPosition.mValues, pnam.mUnknown3, pnam.mActorId.mData,
                pnam.mArrowId.mData, pnam.mBowId.mData);
            mProjectiles.push_back(pnam);
        }
    }

}
