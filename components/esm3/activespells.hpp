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
    class ActorIdConverter;

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
            Flag_Ignore_SpellAbsorption = 1 << 4,
            Flag_Invalid = 1 << 5
        };

        RefId mEffectId;
        float mMagnitude;
        float mMinMagnitude;
        float mMaxMagnitude;
        std::variant<RefId, RefNum> mArg; // skill, attribute, or summon
        float mDuration;
        float mTimeLeft;
        int32_t mEffectIndex;
        int32_t mFlags;

        RefId getSkillOrAttribute() const;
        RefNum getActor() const;
    };

    // format 0, saved games only
    struct ActiveSpells
    {
        enum Flags : uint32_t
        {
            Flag_Temporary = 1 << 0, //!< Effect will end automatically once its duration ends.
            Flag_Equipment = 1 << 1, //!< Effect will end automatically if item is unequipped.
            Flag_SpellStore = 1 << 2, //!< Effect will end automatically if removed from the actor's spell store.
            Flag_AffectsBaseValues = 1 << 3, //!< Effects will affect base values instead of current values.
            Flag_Stackable
            = 1 << 4, //!< Effect can stack. If this flag is not set, spells from the same caster and item cannot stack.
            Flag_Lua
            = 1 << 5, //!< Effect was added via Lua. Should not do any vfx/sound as this is handled by Lua scripts.
        };

        struct ActiveSpellParams
        {
            RefId mActiveSpellId;
            RefId mSourceSpellId;
            std::vector<ActiveEffect> mEffects;
            std::string mDisplayName;
            RefNum mCaster;
            RefNum mItem;
            Flags mFlags;
            int32_t mWorsenings;
            TimeStamp mNextWorsening;
        };

        std::vector<ActiveSpellParams> mSpells;
        std::vector<ActiveSpellParams> mQueue;
        ActorIdConverter* mActorIdConverter = nullptr;

        void load(ESMReader& esm);
        void save(ESMWriter& esm) const;
    };

    namespace Compatibility
    {
        namespace ActiveSpells
        {
            enum EffectType
            {
                Type_Temporary,
                Type_Ability,
                Type_Enchantment,
                Type_Permanent,
                Type_Consumable,
            };

            using Flags = ESM::ActiveSpells::Flags;
            constexpr Flags Type_Temporary_Flags = Flags::Flag_Temporary;
            constexpr Flags Type_Consumable_Flags = static_cast<Flags>(Flags::Flag_Temporary | Flags::Flag_Stackable);
            constexpr Flags Type_Permanent_Flags = Flags::Flag_SpellStore;
            constexpr Flags Type_Ability_Flags
                = static_cast<Flags>(Flags::Flag_SpellStore | Flags::Flag_AffectsBaseValues);
            constexpr Flags Type_Enchantment_Flags = Flags::Flag_Equipment;
        }
    }
}

#endif
