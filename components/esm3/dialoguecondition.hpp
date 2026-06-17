#ifndef OPENMW_ESM3_DIALOGUECONDITION_H
#define OPENMW_ESM3_DIALOGUECONDITION_H

#include <cstdint>
#include <optional>
#include <string>
#include <variant>

#include <components/esm/refid.hpp>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    struct DialogueCondition
    {
        enum Function : std::int8_t
        {
            Function_FacReactionLowest = 0,
            Function_FacReactionHighest,
            Function_RankRequirement,
            Function_Reputation,
            Function_Health_Percent,
            Function_PcReputation,
            Function_PcLevel,
            Function_PcHealthPercent,
            Function_PcMagicka,
            Function_PcFatigue,
            Function_PcStrength,
            Function_PcBlock,
            Function_PcArmorer,
            Function_PcMediumArmor,
            Function_PcHeavyArmor,
            Function_PcBluntWeapon,
            Function_PcLongBlade,
            Function_PcAxe,
            Function_PcSpear,
            Function_PcAthletics,
            Function_PcEnchant,
            Function_PcDestruction,
            Function_PcAlteration,
            Function_PcIllusion,
            Function_PcConjuration,
            Function_PcMysticism,
            Function_PcRestoration,
            Function_PcAlchemy,
            Function_PcUnarmored,
            Function_PcSecurity,
            Function_PcSneak,
            Function_PcAcrobatics,
            Function_PcLightArmor,
            Function_PcShortBlade,
            Function_PcMarksman,
            Function_PcMercantile,
            Function_PcSpeechcraft,
            Function_PcHandToHand,
            Function_PcGender,
            Function_PcExpelled,
            Function_PcCommonDisease,
            Function_PcBlightDisease,
            Function_PcClothingModifier,
            Function_PcCrimeLevel,
            Function_SameSex,
            Function_SameRace,
            Function_SameFaction,
            Function_FactionRankDifference,
            Function_Detected,
            Function_Alarmed,
            Function_Choice,
            Function_PcIntelligence,
            Function_PcWillpower,
            Function_PcAgility,
            Function_PcSpeed,
            Function_PcEndurance,
            Function_PcPersonality,
            Function_PcLuck,
            Function_PcCorprus,
            Function_Weather,
            Function_PcVampire,
            Function_Level,
            Function_Attacked,
            Function_TalkedToPc,
            Function_PcHealth,
            Function_CreatureTarget,
            Function_FriendHit,
            Function_Fight,
            Function_Hello,
            Function_Alarm,
            Function_Flee,
            Function_ShouldAttack,
            Function_Werewolf,
            Function_PcWerewolfKills = 73,

            Function_Global,
            Function_Local,
            Function_Journal,
            Function_Item,
            Function_Dead,
            Function_NotId,
            Function_NotFaction,
            Function_NotClass,
            Function_NotRace,
            Function_NotCell,
            Function_NotLocal,

            Function_None, // Editor only
        };

        enum Comparison : char
        {
            Comp_Eq = '0',
            Comp_Ne = '1',
            Comp_Gt = '2',
            Comp_Ge = '3',
            Comp_Ls = '4',
            Comp_Le = '5',

            Comp_None = ' ', // Editor only
        };

        std::string mVariable;
        std::variant<int32_t, float> mValue = 0;
        std::uint8_t mIndex = 0;
        Function mFunction = Function_None;
        Comparison mComparison = Comp_None;

        static std::optional<DialogueCondition> load(ESMReader& esm, ESM::RefId context);

        void save(ESMWriter& esm) const;
    };
}

#endif
