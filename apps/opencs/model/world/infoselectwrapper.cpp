#include "infoselectwrapper.hpp"

#include <limits>
#include <sstream>
#include <stdexcept>

#include <components/esm3/variant.hpp>

const char* CSMWorld::ConstInfoSelectWrapper::FunctionEnumStrings[] = {
    "Faction Reaction Low",
    "Faction Reaction High",
    "Rank Requirement",
    "Reputation",
    "Health Percent",
    "PC Reputation",
    "PC Level",
    "PC Health Percent",
    "PC Magicka",
    "PC Fatigue",
    "PC Strength",
    "PC Block",
    "PC Armorer",
    "PC Medium Armor",
    "PC Heavy Armor",
    "PC Blunt Weapon",
    "PC Long Blade",
    "PC Axe",
    "PC Spear",
    "PC Athletics",
    "PC Enchant",
    "PC Detruction",
    "PC Alteration",
    "PC Illusion",
    "PC Conjuration",
    "PC Mysticism",
    "PC Restoration",
    "PC Alchemy",
    "PC Unarmored",
    "PC Security",
    "PC Sneak",
    "PC Acrobatics",
    "PC Light Armor",
    "PC Short Blade",
    "PC Marksman",
    "PC Merchantile",
    "PC Speechcraft",
    "PC Hand to Hand",
    "PC Sex",
    "PC Expelled",
    "PC Common Disease",
    "PC Blight Disease",
    "PC Clothing Modifier",
    "PC Crime Level",
    "Same Sex",
    "Same Race",
    "Same Faction",
    "Faction Rank Difference",
    "Detected",
    "Alarmed",
    "Choice",
    "PC Intelligence",
    "PC Willpower",
    "PC Agility",
    "PC Speed",
    "PC Endurance",
    "PC Personality",
    "PC Luck",
    "PC Corprus",
    "Weather",
    "PC Vampire",
    "Level",
    "Attacked",
    "Talked to PC",
    "PC Health",
    "Creature Target",
    "Friend Hit",
    "Fight",
    "Hello",
    "Alarm",
    "Flee",
    "Should Attack",
    "Werewolf",
    "PC Werewolf Kills",
    "Global",
    "Local",
    "Journal",
    "Item",
    "Dead",
    "Not Id",
    "Not Faction",
    "Not Class",
    "Not Race",
    "Not Cell",
    "Not Local",
    nullptr,
};

const char* CSMWorld::ConstInfoSelectWrapper::RelationEnumStrings[] = {
    "=",
    "!=",
    ">",
    ">=",
    "<",
    "<=",
    nullptr,
};

namespace
{
    std::string_view convertToString(ESM::DialogueCondition::Function name)
    {
        if (name < ESM::DialogueCondition::Function_None)
            return CSMWorld::ConstInfoSelectWrapper::FunctionEnumStrings[name];
        return "(Invalid Data: Function)";
    }

    std::string_view convertToString(ESM::DialogueCondition::Comparison type)
    {
        if (type != ESM::DialogueCondition::Comp_None)
            return CSMWorld::ConstInfoSelectWrapper::RelationEnumStrings[type - ESM::DialogueCondition::Comp_Eq];
        return "(Invalid Data: Relation)";
    }
}

// ConstInfoSelectWrapper

CSMWorld::ConstInfoSelectWrapper::ConstInfoSelectWrapper(const ESM::DialogueCondition& select)
    : mConstSelect(select)
{
    updateHasVariable();
    updateComparisonType();
}

ESM::DialogueCondition::Function CSMWorld::ConstInfoSelectWrapper::getFunctionName() const
{
    return mConstSelect.mFunction;
}

ESM::DialogueCondition::Comparison CSMWorld::ConstInfoSelectWrapper::getRelationType() const
{
    return mConstSelect.mComparison;
}

CSMWorld::ConstInfoSelectWrapper::ComparisonType CSMWorld::ConstInfoSelectWrapper::getComparisonType() const
{
    return mComparisonType;
}

bool CSMWorld::ConstInfoSelectWrapper::hasVariable() const
{
    return mHasVariable;
}

const std::string& CSMWorld::ConstInfoSelectWrapper::getVariableName() const
{
    return mConstSelect.mVariable;
}

bool CSMWorld::ConstInfoSelectWrapper::conditionIsAlwaysTrue() const
{
    if (mComparisonType == Comparison_Boolean || mComparisonType == Comparison_Integer)
    {
        if (std::holds_alternative<float>(mConstSelect.mValue))
            return conditionIsAlwaysTrue(getConditionFloatRange(), getValidIntRange());
        else
            return conditionIsAlwaysTrue(getConditionIntRange(), getValidIntRange());
    }
    else if (mComparisonType == Comparison_Numeric)
    {
        if (std::holds_alternative<float>(mConstSelect.mValue))
            return conditionIsAlwaysTrue(getConditionFloatRange(), getValidFloatRange());
        else
            return conditionIsAlwaysTrue(getConditionIntRange(), getValidFloatRange());
    }

    return false;
}

bool CSMWorld::ConstInfoSelectWrapper::conditionIsNeverTrue() const
{
    if (mComparisonType == Comparison_Boolean || mComparisonType == Comparison_Integer)
    {
        if (std::holds_alternative<float>(mConstSelect.mValue))
            return conditionIsNeverTrue(getConditionFloatRange(), getValidIntRange());
        else
            return conditionIsNeverTrue(getConditionIntRange(), getValidIntRange());
    }
    else if (mComparisonType == Comparison_Numeric)
    {
        if (std::holds_alternative<float>(mConstSelect.mValue))
            return conditionIsNeverTrue(getConditionFloatRange(), getValidFloatRange());
        else
            return conditionIsNeverTrue(getConditionIntRange(), getValidFloatRange());
    }

    return false;
}

std::string CSMWorld::ConstInfoSelectWrapper::toString() const
{
    std::ostringstream stream;
    stream << convertToString(getFunctionName()) << " ";

    if (mHasVariable)
        stream << getVariableName() << " ";

    stream << convertToString(getRelationType()) << " ";

    std::visit([&](auto value) { stream << value; }, mConstSelect.mValue);

    return stream.str();
}

void CSMWorld::ConstInfoSelectWrapper::updateHasVariable()
{
    switch (getFunctionName())
    {
        case ESM::DialogueCondition::Function_Global:
        case ESM::DialogueCondition::Function_Local:
        case ESM::DialogueCondition::Function_Journal:
        case ESM::DialogueCondition::Function_Item:
        case ESM::DialogueCondition::Function_Dead:
        case ESM::DialogueCondition::Function_NotId:
        case ESM::DialogueCondition::Function_NotFaction:
        case ESM::DialogueCondition::Function_NotClass:
        case ESM::DialogueCondition::Function_NotRace:
        case ESM::DialogueCondition::Function_NotCell:
        case ESM::DialogueCondition::Function_NotLocal:
            mHasVariable = true;
            break;

        default:
            mHasVariable = false;
            break;
    }
}

void CSMWorld::ConstInfoSelectWrapper::updateComparisonType()
{
    switch (getFunctionName())
    {
        // Boolean
        case ESM::DialogueCondition::Function_NotId:
        case ESM::DialogueCondition::Function_NotFaction:
        case ESM::DialogueCondition::Function_NotClass:
        case ESM::DialogueCondition::Function_NotRace:
        case ESM::DialogueCondition::Function_NotCell:
        case ESM::DialogueCondition::Function_PcExpelled:
        case ESM::DialogueCondition::Function_PcCommonDisease:
        case ESM::DialogueCondition::Function_PcBlightDisease:
        case ESM::DialogueCondition::Function_SameSex:
        case ESM::DialogueCondition::Function_SameRace:
        case ESM::DialogueCondition::Function_SameFaction:
        case ESM::DialogueCondition::Function_Detected:
        case ESM::DialogueCondition::Function_Alarmed:
        case ESM::DialogueCondition::Function_PcCorprus:
        case ESM::DialogueCondition::Function_PcVampire:
        case ESM::DialogueCondition::Function_Attacked:
        case ESM::DialogueCondition::Function_TalkedToPc:
        case ESM::DialogueCondition::Function_ShouldAttack:
        case ESM::DialogueCondition::Function_Werewolf:
            mComparisonType = Comparison_Boolean;
            break;

        // Integer
        case ESM::DialogueCondition::Function_Journal:
        case ESM::DialogueCondition::Function_Item:
        case ESM::DialogueCondition::Function_Dead:
        case ESM::DialogueCondition::Function_FacReactionLowest:
        case ESM::DialogueCondition::Function_FacReactionHighest:
        case ESM::DialogueCondition::Function_RankRequirement:
        case ESM::DialogueCondition::Function_Reputation:
        case ESM::DialogueCondition::Function_PcReputation:
        case ESM::DialogueCondition::Function_PcLevel:
        case ESM::DialogueCondition::Function_PcStrength:
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
        case ESM::DialogueCondition::Function_PcMerchantile:
        case ESM::DialogueCondition::Function_PcSpeechcraft:
        case ESM::DialogueCondition::Function_PcHandToHand:
        case ESM::DialogueCondition::Function_PcGender:
        case ESM::DialogueCondition::Function_PcClothingModifier:
        case ESM::DialogueCondition::Function_PcCrimeLevel:
        case ESM::DialogueCondition::Function_FactionRankDifference:
        case ESM::DialogueCondition::Function_Choice:
        case ESM::DialogueCondition::Function_PcIntelligence:
        case ESM::DialogueCondition::Function_PcWillpower:
        case ESM::DialogueCondition::Function_PcAgility:
        case ESM::DialogueCondition::Function_PcSpeed:
        case ESM::DialogueCondition::Function_PcEndurance:
        case ESM::DialogueCondition::Function_PcPersonality:
        case ESM::DialogueCondition::Function_PcLuck:
        case ESM::DialogueCondition::Function_Weather:
        case ESM::DialogueCondition::Function_Level:
        case ESM::DialogueCondition::Function_CreatureTarget:
        case ESM::DialogueCondition::Function_FriendHit:
        case ESM::DialogueCondition::Function_Fight:
        case ESM::DialogueCondition::Function_Hello:
        case ESM::DialogueCondition::Function_Alarm:
        case ESM::DialogueCondition::Function_Flee:
        case ESM::DialogueCondition::Function_PcWerewolfKills:
            mComparisonType = Comparison_Integer;
            break;

        // Numeric
        case ESM::DialogueCondition::Function_Global:
        case ESM::DialogueCondition::Function_Local:
        case ESM::DialogueCondition::Function_NotLocal:

        case ESM::DialogueCondition::Function_Health_Percent:
        case ESM::DialogueCondition::Function_PcHealthPercent:
        case ESM::DialogueCondition::Function_PcMagicka:
        case ESM::DialogueCondition::Function_PcFatigue:
        case ESM::DialogueCondition::Function_PcHealth:
            mComparisonType = Comparison_Numeric;
            break;

        default:
            mComparisonType = Comparison_None;
            break;
    }
}

std::pair<int, int> CSMWorld::ConstInfoSelectWrapper::getConditionIntRange() const
{
    const int IntMax = std::numeric_limits<int>::max();
    const int IntMin = std::numeric_limits<int>::min();
    const std::pair<int, int> InvalidRange(IntMax, IntMin);

    int value = std::get<int>(mConstSelect.mValue);

    switch (getRelationType())
    {
        case ESM::DialogueCondition::Comp_Eq:
        case ESM::DialogueCondition::Comp_Ne:
            return std::pair<int, int>(value, value);

        case ESM::DialogueCondition::Comp_Gt:
            if (value == IntMax)
            {
                return InvalidRange;
            }
            else
            {
                return std::pair<int, int>(value + 1, IntMax);
            }
            break;

        case ESM::DialogueCondition::Comp_Ge:
            return std::pair<int, int>(value, IntMax);

        case ESM::DialogueCondition::Comp_Ls:
            if (value == IntMin)
            {
                return InvalidRange;
            }
            else
            {
                return std::pair<int, int>(IntMin, value - 1);
            }

        case ESM::DialogueCondition::Comp_Le:
            return std::pair<int, int>(IntMin, value);

        default:
            throw std::logic_error("InfoSelectWrapper: relation does not have a range");
    }
}

std::pair<float, float> CSMWorld::ConstInfoSelectWrapper::getConditionFloatRange() const
{
    const float FloatMax = std::numeric_limits<float>::infinity();
    const float FloatMin = -std::numeric_limits<float>::infinity();
    const float Epsilon = std::numeric_limits<float>::epsilon();

    float value = std::get<float>(mConstSelect.mValue);

    switch (getRelationType())
    {
        case ESM::DialogueCondition::Comp_Eq:
        case ESM::DialogueCondition::Comp_Ne:
            return std::pair<float, float>(value, value);

        case ESM::DialogueCondition::Comp_Gt:
            return std::pair<float, float>(value + Epsilon, FloatMax);

        case ESM::DialogueCondition::Comp_Ge:
            return std::pair<float, float>(value, FloatMax);

        case ESM::DialogueCondition::Comp_Ls:
            return std::pair<float, float>(FloatMin, value - Epsilon);

        case ESM::DialogueCondition::Comp_Le:
            return std::pair<float, float>(FloatMin, value);

        default:
            throw std::logic_error("InfoSelectWrapper: given relation does not have a range");
    }
}

std::pair<int, int> CSMWorld::ConstInfoSelectWrapper::getValidIntRange() const
{
    const int IntMax = std::numeric_limits<int>::max();
    const int IntMin = std::numeric_limits<int>::min();

    switch (getFunctionName())
    {
        // Boolean
        case ESM::DialogueCondition::Function_NotId:
        case ESM::DialogueCondition::Function_NotFaction:
        case ESM::DialogueCondition::Function_NotClass:
        case ESM::DialogueCondition::Function_NotRace:
        case ESM::DialogueCondition::Function_NotCell:
        case ESM::DialogueCondition::Function_PcExpelled:
        case ESM::DialogueCondition::Function_PcCommonDisease:
        case ESM::DialogueCondition::Function_PcBlightDisease:
        case ESM::DialogueCondition::Function_SameSex:
        case ESM::DialogueCondition::Function_SameRace:
        case ESM::DialogueCondition::Function_SameFaction:
        case ESM::DialogueCondition::Function_Detected:
        case ESM::DialogueCondition::Function_Alarmed:
        case ESM::DialogueCondition::Function_PcCorprus:
        case ESM::DialogueCondition::Function_PcVampire:
        case ESM::DialogueCondition::Function_Attacked:
        case ESM::DialogueCondition::Function_TalkedToPc:
        case ESM::DialogueCondition::Function_ShouldAttack:
        case ESM::DialogueCondition::Function_Werewolf:
            return std::pair<int, int>(0, 1);

        // Integer
        case ESM::DialogueCondition::Function_FacReactionLowest:
        case ESM::DialogueCondition::Function_FacReactionHighest:
        case ESM::DialogueCondition::Function_Reputation:
        case ESM::DialogueCondition::Function_PcReputation:
        case ESM::DialogueCondition::Function_Journal:
            return std::pair<int, int>(IntMin, IntMax);

        case ESM::DialogueCondition::Function_Item:
        case ESM::DialogueCondition::Function_Dead:
        case ESM::DialogueCondition::Function_PcLevel:
        case ESM::DialogueCondition::Function_PcStrength:
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
        case ESM::DialogueCondition::Function_PcMerchantile:
        case ESM::DialogueCondition::Function_PcSpeechcraft:
        case ESM::DialogueCondition::Function_PcHandToHand:
        case ESM::DialogueCondition::Function_PcClothingModifier:
        case ESM::DialogueCondition::Function_PcCrimeLevel:
        case ESM::DialogueCondition::Function_Choice:
        case ESM::DialogueCondition::Function_PcIntelligence:
        case ESM::DialogueCondition::Function_PcWillpower:
        case ESM::DialogueCondition::Function_PcAgility:
        case ESM::DialogueCondition::Function_PcSpeed:
        case ESM::DialogueCondition::Function_PcEndurance:
        case ESM::DialogueCondition::Function_PcPersonality:
        case ESM::DialogueCondition::Function_PcLuck:
        case ESM::DialogueCondition::Function_Level:
        case ESM::DialogueCondition::Function_PcWerewolfKills:
            return std::pair<int, int>(0, IntMax);

        case ESM::DialogueCondition::Function_Fight:
        case ESM::DialogueCondition::Function_Hello:
        case ESM::DialogueCondition::Function_Alarm:
        case ESM::DialogueCondition::Function_Flee:
            return std::pair<int, int>(0, 100);

        case ESM::DialogueCondition::Function_Weather:
            return std::pair<int, int>(0, 9);

        case ESM::DialogueCondition::Function_FriendHit:
            return std::pair<int, int>(0, 4);

        case ESM::DialogueCondition::Function_RankRequirement:
            return std::pair<int, int>(0, 3);

        case ESM::DialogueCondition::Function_CreatureTarget:
            return std::pair<int, int>(0, 2);

        case ESM::DialogueCondition::Function_PcGender:
            return std::pair<int, int>(0, 1);

        case ESM::DialogueCondition::Function_FactionRankDifference:
            return std::pair<int, int>(-9, 9);

        // Numeric
        case ESM::DialogueCondition::Function_Global:
        case ESM::DialogueCondition::Function_Local:
        case ESM::DialogueCondition::Function_NotLocal:
            return std::pair<int, int>(IntMin, IntMax);

        case ESM::DialogueCondition::Function_PcMagicka:
        case ESM::DialogueCondition::Function_PcFatigue:
        case ESM::DialogueCondition::Function_PcHealth:
            return std::pair<int, int>(0, IntMax);

        case ESM::DialogueCondition::Function_Health_Percent:
        case ESM::DialogueCondition::Function_PcHealthPercent:
            return std::pair<int, int>(0, 100);

        default:
            throw std::runtime_error("InfoSelectWrapper: function does not exist");
    }
}

std::pair<float, float> CSMWorld::ConstInfoSelectWrapper::getValidFloatRange() const
{
    const float FloatMax = std::numeric_limits<float>::infinity();
    const float FloatMin = -std::numeric_limits<float>::infinity();

    switch (getFunctionName())
    {
        // Numeric
        case ESM::DialogueCondition::Function_Global:
        case ESM::DialogueCondition::Function_Local:
        case ESM::DialogueCondition::Function_NotLocal:
            return std::pair<float, float>(FloatMin, FloatMax);

        case ESM::DialogueCondition::Function_PcMagicka:
        case ESM::DialogueCondition::Function_PcFatigue:
        case ESM::DialogueCondition::Function_PcHealth:
            return std::pair<float, float>(0, FloatMax);

        case ESM::DialogueCondition::Function_Health_Percent:
        case ESM::DialogueCondition::Function_PcHealthPercent:
            return std::pair<float, float>(0, 100);

        default:
            throw std::runtime_error("InfoSelectWrapper: function does not exist or is not numeric");
    }
}

template <typename T1, typename T2>
bool CSMWorld::ConstInfoSelectWrapper::rangeContains(T1 value, std::pair<T2, T2> range) const
{
    return (value >= range.first && value <= range.second);
}

template <typename T1, typename T2>
bool CSMWorld::ConstInfoSelectWrapper::rangeFullyContains(
    std::pair<T1, T1> containingRange, std::pair<T2, T2> testRange) const
{
    return (containingRange.first <= testRange.first) && (testRange.second <= containingRange.second);
}

template <typename T1, typename T2>
bool CSMWorld::ConstInfoSelectWrapper::rangesOverlap(std::pair<T1, T1> range1, std::pair<T2, T2> range2) const
{
    // One of the bounds of either range should fall within the other range
    return (range1.first <= range2.first && range2.first <= range1.second)
        || (range1.first <= range2.second && range2.second <= range1.second)
        || (range2.first <= range1.first && range1.first <= range2.second)
        || (range2.first <= range1.second && range1.second <= range2.second);
}

template <typename T1, typename T2>
bool CSMWorld::ConstInfoSelectWrapper::rangesMatch(std::pair<T1, T1> range1, std::pair<T2, T2> range2) const
{
    return (range1.first == range2.first && range1.second == range2.second);
}

template <typename T1, typename T2>
bool CSMWorld::ConstInfoSelectWrapper::conditionIsAlwaysTrue(
    std::pair<T1, T1> conditionRange, std::pair<T2, T2> validRange) const
{
    switch (getRelationType())
    {
        case ESM::DialogueCondition::Comp_Eq:
            return false;

        case ESM::DialogueCondition::Comp_Ne:
            // If value is not within range, it will always be true
            return !rangeContains(conditionRange.first, validRange);

        case ESM::DialogueCondition::Comp_Gt:
        case ESM::DialogueCondition::Comp_Ge:
        case ESM::DialogueCondition::Comp_Ls:
        case ESM::DialogueCondition::Comp_Le:
            // If the valid range is completely within the condition range, it will always be true
            return rangeFullyContains(conditionRange, validRange);

        default:
            throw std::logic_error("InfoCondition: operator can not be used to compare");
    }

    return false;
}

template <typename T1, typename T2>
bool CSMWorld::ConstInfoSelectWrapper::conditionIsNeverTrue(
    std::pair<T1, T1> conditionRange, std::pair<T2, T2> validRange) const
{
    switch (getRelationType())
    {
        case ESM::DialogueCondition::Comp_Eq:
            return !rangeContains(conditionRange.first, validRange);

        case ESM::DialogueCondition::Comp_Ne:
            return false;

        case ESM::DialogueCondition::Comp_Gt:
        case ESM::DialogueCondition::Comp_Ge:
        case ESM::DialogueCondition::Comp_Ls:
        case ESM::DialogueCondition::Comp_Le:
            // If ranges do not overlap, it will never be true
            return !rangesOverlap(conditionRange, validRange);

        default:
            throw std::logic_error("InfoCondition: operator can not be used to compare");
    }

    return false;
}

QVariant CSMWorld::ConstInfoSelectWrapper::getValue() const
{
    return std::visit([](auto value) { return QVariant(value); }, mConstSelect.mValue);
}

// InfoSelectWrapper

CSMWorld::InfoSelectWrapper::InfoSelectWrapper(ESM::DialogueCondition& select)
    : CSMWorld::ConstInfoSelectWrapper(select)
    , mSelect(select)
{
}

void CSMWorld::InfoSelectWrapper::setFunctionName(ESM::DialogueCondition::Function name)
{
    mSelect.mFunction = name;
    updateHasVariable();
    updateComparisonType();
    if (getComparisonType() != ConstInfoSelectWrapper::Comparison_Numeric
        && std::holds_alternative<float>(mSelect.mValue))
    {
        mSelect.mValue = std::visit([](auto value) { return static_cast<int>(value); }, mSelect.mValue);
    }
}

void CSMWorld::InfoSelectWrapper::setRelationType(ESM::DialogueCondition::Comparison type)
{
    mSelect.mComparison = type;
}

void CSMWorld::InfoSelectWrapper::setVariableName(const std::string& name)
{
    mSelect.mVariable = name;
}

void CSMWorld::InfoSelectWrapper::setValue(int value)
{
    mSelect.mValue = value;
}

void CSMWorld::InfoSelectWrapper::setValue(float value)
{
    mSelect.mValue = value;
}
