#include "importcellref.hpp"

#include <components/esm3/esmreader.hpp>
#include <components/misc/concepts.hpp>

#include <cstdint>

namespace ESSImport
{
    template <Misc::SameAsWithoutCvref<ACDT> T>
    void decompose(T&& v, const auto& f)
    {
        f(v.mUnknown, v.mFlags, v.mBreathMeter, v.mUnknown2, v.mDynamic, v.mUnknown3, v.mAttributes, v.mMagicEffects,
            v.mUnknown4, v.mGoldPool, v.mCountDown, v.mUnknown5);
    }

    template <Misc::SameAsWithoutCvref<ACSC> T>
    void decompose(T&& v, const auto& f)
    {
        f(v.mUnknown1, v.mFlags, v.mUnknown2, v.mCorpseClearCountdown, v.mUnknown3);
    }

    template <Misc::SameAsWithoutCvref<ANIS> T>
    void decompose(T&& v, const auto& f)
    {
        f(v.mGroupIndex, v.mUnknown, v.mTime);
    }

    void CellRef::load(ESM::ESMReader& esm)
    {
        blank();

        esm.getHNT(mRefNum.mIndex, "FRMR");

        // this is required since openmw supports more than 255 content files
        int pluginIndex = (mRefNum.mIndex & 0xff000000) >> 24;
        mRefNum.mContentFile = pluginIndex - 1;
        mRefNum.mIndex &= 0x00ffffff;

        mIndexedRefId = esm.getHNString("NAME");

        if (esm.isNextSub("ACTN"))
        {
            /*
            Activation flags:
            ActivationFlag_UseEnabled  = 1
            ActivationFlag_OnActivate  = 2
            ActivationFlag_OnDeath  = 10h
            ActivationFlag_OnKnockout  = 20h
            ActivationFlag_OnMurder  = 40h
            ActivationFlag_DoorOpening  = 100h
            ActivationFlag_DoorClosing  = 200h
            ActivationFlag_DoorJammedOpening  = 400h
            ActivationFlag_DoorJammedClosing  = 800h
            */
            esm.skipHSub();
        }

        if (esm.isNextSub("STPR"))
            esm.skipHSub();

        if (esm.isNextSub("MNAM"))
            esm.skipHSub();

        bool isDeleted = false;
        ESM::CellRef::loadData(esm, isDeleted);

        mActorData.mHasACDT = esm.getOptionalComposite("ACDT", mActorData.mACDT);

        mActorData.mHasACSC = esm.getOptionalComposite("ACSC", mActorData.mACSC);

        if (esm.isNextSub("ACSL"))
            esm.skipHSubSize(112);

        if (esm.isNextSub("CSTN"))
            esm.skipHSub(); // "PlayerSaveGame", link to some object?

        if (esm.isNextSub("LSTN"))
            esm.skipHSub(); // "PlayerSaveGame", link to some object?

        // unsure at which point between LSTN and TGTN
        if (esm.isNextSub("CSHN"))
            esm.skipHSub(); // "PlayerSaveGame", link to some object?

        // unsure if before or after CSTN/LSTN
        if (esm.isNextSub("LSHN"))
            esm.skipHSub(); // "PlayerSaveGame", link to some object?

        while (esm.isNextSub("TGTN"))
            esm.skipHSub(); // "PlayerSaveGame", link to some object?

        while (esm.isNextSub("FGTN"))
            esm.getHString(); // fight target?

        // unsure at which point between TGTN and CRED
        if (esm.isNextSub("AADT"))
        {
            // occurred when a creature was in the middle of its attack, 44 bytes
            esm.skipHSub();
        }

        // unsure at which point between FGTN and CHRD
        if (esm.isNextSub("PWPC"))
            esm.skipHSub();
        if (esm.isNextSub("PWPS"))
            esm.skipHSub();

        if (esm.isNextSub("WNAM"))
        {
            std::string id = esm.getHString();

            if (esm.isNextSub("XNAM"))
                mActorData.mSelectedEnchantItem = esm.getHString();
            else
                mActorData.mSelectedSpell = std::move(id);

            if (esm.isNextSub("YNAM"))
                esm.skipHSub(); // 4 byte, 0
        }

        while (esm.isNextSub("APUD"))
        {
            // used power
            esm.getSubHeader();
            std::string id = esm.getMaybeFixedStringSize(32);
            (void)id;
            // timestamp can't be used: this is the total hours passed, calculated by
            // timestamp = 24 * (365 * year + cumulativeDays[month] + day)
            // unfortunately cumulativeDays[month] is not clearly defined,
            // in the (non-MCP) vanilla version the first month was missing, but MCP added it.
            double timestamp;
            esm.getT(timestamp);
        }

        // FIXME: not all actors have this, add flag
        esm.getHNOT("CHRD", mActorData.mSkills); // npc only

        esm.getHNOT("CRED", mActorData.mCombatStats); // creature only

        mActorData.mSCRI.load(esm);

        if (esm.isNextSub("ND3D"))
            esm.skipHSub();

        mActorData.mHasANIS = esm.getOptionalComposite("ANIS", mActorData.mANIS);

        if (esm.isNextSub("LVCR"))
        {
            // occurs on levelled creature spawner references
            // probably some identifier for the creature that has been spawned?
            unsigned char lvcr;
            esm.getHT(lvcr);
            // std::cout << "LVCR: " << (int)lvcr << std::endl;
        }

        mEnabled = true;
        esm.getHNOT(mEnabled, "ZNAM");

        // DATA should occur for all references, except levelled creature spawners
        // I've seen DATA *twice* on a creature record, and with the exact same content too! weird
        // alarmvoi0000.ess
        for (int i = 0; i < 2; ++i)
            esm.getOptionalComposite("DATA", mPos);

        mDeleted = 0;
        if (esm.isNextSub("DELE"))
        {
            uint32_t deleted;
            esm.getHT(deleted);
            mDeleted = ((deleted >> 24) & 0x2) != 0; // the other 3 bytes seem to be uninitialized garbage
        }

        if (esm.isNextSub("MVRF"))
        {
            esm.skipHSub();
            esm.getSubName();
            esm.skipHSub();
        }
    }

}
