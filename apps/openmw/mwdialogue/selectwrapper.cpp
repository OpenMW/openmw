#include "selectwrapper.hpp"

#include <iterator>
#include <sstream>
#include <stdexcept>

#include <components/misc/strings/conversion.hpp>
#include <components/misc/strings/lower.hpp>

namespace
{
    template <typename T1, typename T2>
    bool selectCompareImp(ESM::DialogueCondition::Comparison comp, T1 value1, T2 value2)
    {
        switch (comp)
        {
            case ESM::DialogueCondition::Comp_Eq:
                return value1 == value2;
            case ESM::DialogueCondition::Comp_Ne:
                return value1 != value2;
            case ESM::DialogueCondition::Comp_Gt:
                return value1 > value2;
            case ESM::DialogueCondition::Comp_Ge:
                return value1 >= value2;
            case ESM::DialogueCondition::Comp_Ls:
                return value1 < value2;
            case ESM::DialogueCondition::Comp_Le:
                return value1 <= value2;
            default:
                throw std::runtime_error("unknown compare type in dialogue info select");
        }
    }

    template <typename T>
    bool selectCompareImp(const ESM::DialogueCondition& select, T value1)
    {
        return std::visit(
            [&](auto value) { return selectCompareImp(select.mComparison, value1, value); }, select.mValue);
    }
}

MWDialogue::SelectWrapper::SelectWrapper(const ESM::DialogueCondition& select)
    : mSelect(select)
{
}

ESM::DialogueCondition::Function MWDialogue::SelectWrapper::getFunction() const
{
    return mSelect.mFunction;
}

int MWDialogue::SelectWrapper::getArgument() const
{
    switch (mSelect.mFunction)
    {
        // AI settings
        case ESM::DialogueCondition::Function_Fight:
            return 1;
        case ESM::DialogueCondition::Function_Hello:
            return 0;
        case ESM::DialogueCondition::Function_Alarm:
            return 3;
        case ESM::DialogueCondition::Function_Flee:
            return 2;

        // attributes
        case ESM::DialogueCondition::Function_PcStrength:
            return 0;
        case ESM::DialogueCondition::Function_PcIntelligence:
            return 1;
        case ESM::DialogueCondition::Function_PcWillpower:
            return 2;
        case ESM::DialogueCondition::Function_PcAgility:
            return 3;
        case ESM::DialogueCondition::Function_PcSpeed:
            return 4;
        case ESM::DialogueCondition::Function_PcEndurance:
            return 5;
        case ESM::DialogueCondition::Function_PcPersonality:
            return 6;
        case ESM::DialogueCondition::Function_PcLuck:
            return 7;

        // skills
        case ESM::DialogueCondition::Function_PcBlock:
            return 0;
        case ESM::DialogueCondition::Function_PcArmorer:
            return 1;
        case ESM::DialogueCondition::Function_PcMediumArmor:
            return 2;
        case ESM::DialogueCondition::Function_PcHeavyArmor:
            return 3;
        case ESM::DialogueCondition::Function_PcBluntWeapon:
            return 4;
        case ESM::DialogueCondition::Function_PcLongBlade:
            return 5;
        case ESM::DialogueCondition::Function_PcAxe:
            return 6;
        case ESM::DialogueCondition::Function_PcSpear:
            return 7;
        case ESM::DialogueCondition::Function_PcAthletics:
            return 8;
        case ESM::DialogueCondition::Function_PcEnchant:
            return 9;
        case ESM::DialogueCondition::Function_PcDestruction:
            return 10;
        case ESM::DialogueCondition::Function_PcAlteration:
            return 11;
        case ESM::DialogueCondition::Function_PcIllusion:
            return 12;
        case ESM::DialogueCondition::Function_PcConjuration:
            return 13;
        case ESM::DialogueCondition::Function_PcMysticism:
            return 14;
        case ESM::DialogueCondition::Function_PcRestoration:
            return 15;
        case ESM::DialogueCondition::Function_PcAlchemy:
            return 16;
        case ESM::DialogueCondition::Function_PcUnarmored:
            return 17;
        case ESM::DialogueCondition::Function_PcSecurity:
            return 18;
        case ESM::DialogueCondition::Function_PcSneak:
            return 19;
        case ESM::DialogueCondition::Function_PcAcrobatics:
            return 20;
        case ESM::DialogueCondition::Function_PcLightArmor:
            return 21;
        case ESM::DialogueCondition::Function_PcShortBlade:
            return 22;
        case ESM::DialogueCondition::Function_PcMarksman:
            return 23;
        case ESM::DialogueCondition::Function_PcMercantile:
            return 24;
        case ESM::DialogueCondition::Function_PcSpeechcraft:
            return 25;
        case ESM::DialogueCondition::Function_PcHandToHand:
            return 26;

        // dynamic stats
        case ESM::DialogueCondition::Function_PcMagicka:
            return 1;
        case ESM::DialogueCondition::Function_PcFatigue:
            return 2;
        case ESM::DialogueCondition::Function_PcHealth:
            return 0;
        default:
            return 0;
    }
}

MWDialogue::SelectWrapper::Type MWDialogue::SelectWrapper::getType() const
{
    switch (mSelect.mFunction)
    {
        case ESM::DialogueCondition::Function_Journal:
        case ESM::DialogueCondition::Function_Item:
        case ESM::DialogueCondition::Function_Dead:
        case ESM::DialogueCondition::Function_Choice:
        case ESM::DialogueCondition::Function_Fight:
        case ESM::DialogueCondition::Function_Hello:
        case ESM::DialogueCondition::Function_Alarm:
        case ESM::DialogueCondition::Function_Flee:
        case ESM::DialogueCondition::Function_PcStrength:
        case ESM::DialogueCondition::Function_PcIntelligence:
        case ESM::DialogueCondition::Function_PcWillpower:
        case ESM::DialogueCondition::Function_PcAgility:
        case ESM::DialogueCondition::Function_PcSpeed:
        case ESM::DialogueCondition::Function_PcEndurance:
        case ESM::DialogueCondition::Function_PcPersonality:
        case ESM::DialogueCondition::Function_PcLuck:
        case ESM::DialogueCondition::Function_PcBlock:
        case ESM::DialogueCondition::Function_PcArmorer:
        case ESM::DialogueCondition::Function_PcMediumArmor:
        case ESM::DialogueCondition::Function_PcHeavyArmor:
        case ESM::DialogueCondition::Function_PcBluntWeapon:
        case ESM::DialogueCondition::Function_PcLongBlade:
        case ESM::DialogueCondition::Function_PcAxe:
        case ESM::DialogueCondition::Function_PcSpear:
        case ESM::DialogueCondition::Function_PcAthletics:
        case ESM::DialogueCondition::Function_PcEnchant:
        case ESM::DialogueCondition::Function_PcDestruction:
        case ESM::DialogueCondition::Function_PcAlteration:
        case ESM::DialogueCondition::Function_PcIllusion:
        case ESM::DialogueCondition::Function_PcConjuration:
        case ESM::DialogueCondition::Function_PcMysticism:
        case ESM::DialogueCondition::Function_PcRestoration:
        case ESM::DialogueCondition::Function_PcAlchemy:
        case ESM::DialogueCondition::Function_PcUnarmored:
        case ESM::DialogueCondition::Function_PcSecurity:
        case ESM::DialogueCondition::Function_PcSneak:
        case ESM::DialogueCondition::Function_PcAcrobatics:
        case ESM::DialogueCondition::Function_PcLightArmor:
        case ESM::DialogueCondition::Function_PcShortBlade:
        case ESM::DialogueCondition::Function_PcMarksman:
        case ESM::DialogueCondition::Function_PcMercantile:
        case ESM::DialogueCondition::Function_PcSpeechcraft:
        case ESM::DialogueCondition::Function_PcHandToHand:
        case ESM::DialogueCondition::Function_FriendHit:
        case ESM::DialogueCondition::Function_PcLevel:
        case ESM::DialogueCondition::Function_PcGender:
        case ESM::DialogueCondition::Function_PcClothingModifier:
        case ESM::DialogueCondition::Function_PcCrimeLevel:
        case ESM::DialogueCondition::Function_RankRequirement:
        case ESM::DialogueCondition::Function_Level:
        case ESM::DialogueCondition::Function_PcReputation:
        case ESM::DialogueCondition::Function_Weather:
        case ESM::DialogueCondition::Function_Reputation:
        case ESM::DialogueCondition::Function_FactionRankDifference:
        case ESM::DialogueCondition::Function_PcWerewolfKills:
        case ESM::DialogueCondition::Function_FacReactionLowest:
        case ESM::DialogueCondition::Function_FacReactionHighest:
        case ESM::DialogueCondition::Function_CreatureTarget:
            return Type_Integer;
        case ESM::DialogueCondition::Function_Global:
        case ESM::DialogueCondition::Function_Local:
        case ESM::DialogueCondition::Function_NotLocal:
        case ESM::DialogueCondition::Function_PcHealth:
        case ESM::DialogueCondition::Function_PcMagicka:
        case ESM::DialogueCondition::Function_PcFatigue:
        case ESM::DialogueCondition::Function_PcHealthPercent:
        case ESM::DialogueCondition::Function_Health_Percent:
            return Type_Numeric;
        case ESM::DialogueCondition::Function_SameSex:
        case ESM::DialogueCondition::Function_SameRace:
        case ESM::DialogueCondition::Function_SameFaction:
        case ESM::DialogueCondition::Function_PcCommonDisease:
        case ESM::DialogueCondition::Function_PcBlightDisease:
        case ESM::DialogueCondition::Function_PcCorprus:
        case ESM::DialogueCondition::Function_PcExpelled:
        case ESM::DialogueCondition::Function_PcVampire:
        case ESM::DialogueCondition::Function_TalkedToPc:
        case ESM::DialogueCondition::Function_Alarmed:
        case ESM::DialogueCondition::Function_Detected:
        case ESM::DialogueCondition::Function_Attacked:
        case ESM::DialogueCondition::Function_ShouldAttack:
        case ESM::DialogueCondition::Function_Werewolf:
            return Type_Boolean;
        case ESM::DialogueCondition::Function_NotId:
        case ESM::DialogueCondition::Function_NotFaction:
        case ESM::DialogueCondition::Function_NotClass:
        case ESM::DialogueCondition::Function_NotRace:
        case ESM::DialogueCondition::Function_NotCell:
            return Type_Inverted;
        default:
            return Type_None;
    };
}

bool MWDialogue::SelectWrapper::selectCompare(int value) const
{
    return selectCompareImp(mSelect, value);
}

bool MWDialogue::SelectWrapper::selectCompare(float value) const
{
    return selectCompareImp(mSelect, value);
}

bool MWDialogue::SelectWrapper::selectCompare(bool value) const
{
    return selectCompareImp(mSelect, static_cast<int>(value));
}

std::string MWDialogue::SelectWrapper::getName() const
{
    return Misc::StringUtils::lowerCase(mSelect.mVariable);
}

std::string_view MWDialogue::SelectWrapper::getCellName() const
{
    return mSelect.mVariable;
}

ESM::RefId MWDialogue::SelectWrapper::getId() const
{
    return ESM::RefId::stringRefId(mSelect.mVariable);
}
