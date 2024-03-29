#ifndef OPENMW_ESSIMPORT_IMPORTSPLM_H
#define OPENMW_ESSIMPORT_IMPORTSPLM_H

#include <components/esm/esmcommon.hpp>
#include <cstdint>
#include <vector>

namespace ESM
{
    class ESMReader;
}

namespace ESSImport
{

    struct SPLM
    {

        struct SPDT // 160 bytes
        {
            int32_t mType; // 1 = spell, 2 = enchantment, 3 = potion
            ESM::NAME32 mId; // base ID of a spell/enchantment/potion
            unsigned char mUnknown[4 * 4];
            ESM::NAME32 mCasterId;
            ESM::NAME32 mSourceId; // empty for spells
            unsigned char mUnknown2[4 * 11];
        };

        struct NPDT // 56 bytes
        {
            ESM::NAME32 mAffectedActorId;
            unsigned char mUnknown[4 * 2];
            int32_t mMagnitude;
            float mSecondsActive;
            unsigned char mUnknown2[4 * 2];
        };

        struct INAM // 40 bytes
        {
            int32_t mUnknown;
            unsigned char mUnknown2;
            ESM::FixedString<35> mItemId; // disintegrated item / bound item / item to re-equip after expiration
        };

        struct CNAM // 36 bytes
        {
            int32_t mUnknown; // seems to always be 0
            ESM::NAME32 mSummonedOrCommandedActor[32];
        };

        struct VNAM // 4 bytes
        {
            int32_t mUnknown;
        };

        struct ActiveEffect
        {
            NPDT mNPDT;
        };

        struct ActiveSpell
        {
            int32_t mIndex;
            SPDT mSPDT;
            std::string mTarget;
            std::vector<ActiveEffect> mActiveEffects;
        };

        std::vector<ActiveSpell> mActiveSpells;

        void load(ESM::ESMReader& esm);
    };

}

#endif
