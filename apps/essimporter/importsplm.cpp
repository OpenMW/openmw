#include "importsplm.h"

#include <components/esm3/esmreader.hpp>

namespace ESSImport
{

    void SPLM::load(ESM::ESMReader& esm)
    {
        while (esm.isNextSub("NAME"))
        {
            ActiveSpell spell;
            esm.getHT(spell.mIndex);
            esm.getHNT("SPDT", spell.mSPDT.mType, spell.mSPDT.mId.mData, spell.mSPDT.mUnknown,
                spell.mSPDT.mCasterId.mData, spell.mSPDT.mSourceId.mData, spell.mSPDT.mUnknown2);
            spell.mTarget = esm.getHNOString("TNAM");

            while (esm.isNextSub("NPDT"))
            {
                ActiveEffect effect;
                esm.getHT(effect.mNPDT.mAffectedActorId.mData, effect.mNPDT.mUnknown, effect.mNPDT.mMagnitude,
                    effect.mNPDT.mSecondsActive, effect.mNPDT.mUnknown2);

                // Effect-specific subrecords can follow:
                // - INAM for disintegration and bound effects
                // - CNAM for summoning and command effects
                // - VNAM for vampirism
                // NOTE: There can be multiple INAMs per effect.
                // TODO: Needs more research.

                esm.skipHSubUntil("NAM0"); // sentinel
                esm.getSubName();
                esm.skipHSub();

                spell.mActiveEffects.push_back(effect);
            }

            unsigned char xnam; // sentinel
            esm.getHNT(xnam, "XNAM");

            mActiveSpells.push_back(std::move(spell));
        }
    }

}
