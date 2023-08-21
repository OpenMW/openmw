#ifndef OPENMW_ESM_ACTIVESPELLS_H
#define OPENMW_ESM_ACTIVESPELLS_H

#include "cellref.hpp"
#include "components/esm/defs.hpp"
#include "components/esm/refid.hpp"
#include "timestamp.hpp"

#include <string>
#include <variant>
#include <vector>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // Parameters of an effect concerning lasting effects.
    // Note we are not using ENAMstruct since the magnitude may be modified by magic resistance, etc.
    struct ActiveEffect
    {
        enum Flags
        {
            Flag_None = 0,
            Flag_Applied = 1 << 0,
            Flag_Remove = 1 << 1,
            Flag_Ignore_Resistances = 1 << 2,
            Flag_Ignore_Reflect = 1 << 3,
            Flag_Ignore_SpellAbsorption = 1 << 4
        };

        int32_t mEffectId;
        float mMagnitude;
        float mMinMagnitude;
        float mMaxMagnitude;
        std::variant<RefId, int> mArg; // skill, attribute, or summon
        float mDuration;
        float mTimeLeft;
        int32_t mEffectIndex;
        int32_t mFlags;

        RefId getSkillOrAttribute() const;
        int getActorId() const;
    };

    // format 0, saved games only
    struct ActiveSpells
    {
        enum EffectType
        {
            Type_Temporary,
            Type_Ability,
            Type_Enchantment,
            Type_Permanent,
            Type_Consumable
        };

        struct ActiveSpellParams
        {
            RefId mId;
            std::vector<ActiveEffect> mEffects;
            std::string mDisplayName;
            int32_t mCasterActorId;
            RefNum mItem;
            EffectType mType;
            int32_t mWorsenings;
            TimeStamp mNextWorsening;
        };

        std::vector<ActiveSpellParams> mSpells;
        std::vector<ActiveSpellParams> mQueue;

        void load(ESMReader& esm);
        void save(ESMWriter& esm) const;
    };
}

#endif
