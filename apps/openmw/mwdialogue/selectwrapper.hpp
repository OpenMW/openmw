#ifndef GAME_MWDIALOGUE_SELECTWRAPPER_H
#define GAME_MWDIALOGUE_SELECTWRAPPER_H

#include <components/esm/loadinfo.hpp>

namespace MWDialogue
{
    class SelectWrapper
    {
            const ESM::DialInfo::SelectStruct& mSelect;

        public:

            enum Function
            {
                Function_None, Function_False,
                Function_Journal,
                Function_Item,
                Function_Dead,
                Function_NotId,
                Function_NotFaction,
                Function_NotClass,
                Function_NotRace,
                Function_NotCell,
                Function_NotLocal,
                Function_Local,
                Function_Global,
                Function_SameGender, Function_SameRace, Function_SameFaction,
                Function_Choice,
                Function_PcCommonDisease, Function_PcBlightDisease, Function_PcCorprus,
                Function_AiSetting,
                Function_PcAttribute, Function_PcSkill,
                Function_PcExpelled,
                Function_PcVampire,
                Function_FriendlyHit,
                Function_TalkedToPc,
                Function_PcLevel, Function_PcHealthPercent, Function_PcDynamicStat,
                Function_PcGender, Function_PcClothingModifier, Function_PcCrimeLevel,
                Function_RankRequirement,
                Function_HealthPercent, Function_Level, Function_PCReputation,
                Function_Weather,
                Function_Reputation, Function_Alarmed, Function_FactionRankDiff, Function_Detected,
                Function_Attacked, Function_ShouldAttack,
                Function_CreatureTargetted,
                Function_Werewolf, Function_WerewolfKills,
                Function_RankLow, Function_RankHigh
            };

            enum Type
            {
                Type_None,
                Type_Integer,
                Type_Numeric,
                Type_Boolean,
                Type_Inverted
            };

        private:

            Function decodeFunction() const;

        public:

            SelectWrapper (const ESM::DialInfo::SelectStruct& select);

            Function getFunction() const;

            int getArgument() const;

            Type getType() const;

            bool isNpcOnly() const;
            ///< \attention Do not call any of the select functions for this select struct!

            bool selectCompare (int value) const;

            bool selectCompare (float value) const;

            bool selectCompare (bool value) const;

            std::string getName() const;
            ///< Return case-smashed name.
    };
}

#endif
