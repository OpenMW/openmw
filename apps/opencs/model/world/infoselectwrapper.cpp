#include "infoselectwrapper.hpp"

#include <limits>
#include <sstream>
#include <stdexcept>

const size_t CSMWorld::ConstInfoSelectWrapper::RuleMinSize = 5;

const size_t CSMWorld::ConstInfoSelectWrapper::FunctionPrefixOffset = 1;
const size_t CSMWorld::ConstInfoSelectWrapper::FunctionIndexOffset = 2;
const size_t CSMWorld::ConstInfoSelectWrapper::RelationIndexOffset = 4;
const size_t CSMWorld::ConstInfoSelectWrapper::VarNameOffset = 5;

const char* CSMWorld::ConstInfoSelectWrapper::FunctionEnumStrings[] =
{
    "Rank Low",
    "Rank High",
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
    "PC Corpus",
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
    0
};

const char* CSMWorld::ConstInfoSelectWrapper::RelationEnumStrings[] =
{
    "=",
    "!=",
    ">",
    ">=",
    "<",
    "<=",
    0
};

const char* CSMWorld::ConstInfoSelectWrapper::ComparisonEnumStrings[] =
{
    "Boolean",
    "Integer",
    "Numeric",
    0
};

// static functions

std::string CSMWorld::ConstInfoSelectWrapper::convertToString(FunctionName name)
{
    if (name < Function_None)
        return FunctionEnumStrings[name];
    else
        return "(Invalid Data: Function)";
}

std::string CSMWorld::ConstInfoSelectWrapper::convertToString(RelationType type)
{
    if (type < Relation_None)
        return RelationEnumStrings[type];
    else
        return "(Invalid Data: Relation)";
}

std::string CSMWorld::ConstInfoSelectWrapper::convertToString(ComparisonType type)
{
    if (type < Comparison_None)
        return ComparisonEnumStrings[type];
    else
        return "(Invalid Data: Comparison)";
}

// ConstInfoSelectWrapper

CSMWorld::ConstInfoSelectWrapper::ConstInfoSelectWrapper(const ESM::DialInfo::SelectStruct& select)
    : mConstSelect(select)
{
    readRule();
}

CSMWorld::ConstInfoSelectWrapper::FunctionName CSMWorld::ConstInfoSelectWrapper::getFunctionName() const
{
    return mFunctionName;
}

CSMWorld::ConstInfoSelectWrapper::RelationType CSMWorld::ConstInfoSelectWrapper::getRelationType() const
{
    return mRelationType;
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
    return mVariableName;
}

bool CSMWorld::ConstInfoSelectWrapper::conditionIsAlwaysTrue() const
{
    if (!variantTypeIsValid())
        return false;

    if (mComparisonType == Comparison_Boolean || mComparisonType == Comparison_Integer)
    {
        if (mConstSelect.mValue.getType() == ESM::VT_Float)
            return conditionIsAlwaysTrue(getConditionFloatRange(), getValidIntRange());
        else
            return conditionIsAlwaysTrue(getConditionIntRange(), getValidIntRange());
    }
    else if (mComparisonType == Comparison_Numeric)
    {
        if (mConstSelect.mValue.getType() == ESM::VT_Float)
            return conditionIsAlwaysTrue(getConditionFloatRange(), getValidFloatRange());
        else
            return conditionIsAlwaysTrue(getConditionIntRange(), getValidFloatRange());
    }

    return false;
}

bool CSMWorld::ConstInfoSelectWrapper::conditionIsNeverTrue() const
{
    if (!variantTypeIsValid())
        return false;

    if (mComparisonType == Comparison_Boolean || mComparisonType == Comparison_Integer)
    {
        if (mConstSelect.mValue.getType() == ESM::VT_Float)
            return conditionIsNeverTrue(getConditionFloatRange(), getValidIntRange());
        else
            return conditionIsNeverTrue(getConditionIntRange(), getValidIntRange());
    }
    else if (mComparisonType == Comparison_Numeric)
    {
        if (mConstSelect.mValue.getType() == ESM::VT_Float)
            return conditionIsNeverTrue(getConditionFloatRange(), getValidFloatRange());
        else
            return conditionIsNeverTrue(getConditionIntRange(), getValidFloatRange());
    }

    return false;
}

bool CSMWorld::ConstInfoSelectWrapper::variantTypeIsValid() const
{
    return (mConstSelect.mValue.getType() == ESM::VT_Int || mConstSelect.mValue.getType() == ESM::VT_Float);
}

const ESM::Variant& CSMWorld::ConstInfoSelectWrapper::getVariant() const
{
    return mConstSelect.mValue;
}

std::string CSMWorld::ConstInfoSelectWrapper::toString() const
{
    std::ostringstream stream;
    stream << convertToString(mFunctionName) << " ";

    if (mHasVariable)
        stream << mVariableName << " ";

    stream << convertToString(mRelationType) << " ";

    switch (mConstSelect.mValue.getType())
    {
        case ESM::VT_Int:
            stream << mConstSelect.mValue.getInteger();
            break;

        case ESM::VT_Float:
            stream << mConstSelect.mValue.getFloat();
            break;

        default:
            stream << "(Invalid value type)";
            break;
    }

    return stream.str();
}

void CSMWorld::ConstInfoSelectWrapper::readRule()
{
    if (mConstSelect.mSelectRule.size() < RuleMinSize)
        throw std::runtime_error("InfoSelectWrapper: rule is to small");

    readFunctionName();
    readRelationType();
    readVariableName();
    updateHasVariable();
    updateComparisonType();
}

void CSMWorld::ConstInfoSelectWrapper::readFunctionName()
{
    char functionPrefix = mConstSelect.mSelectRule[FunctionPrefixOffset];
    std::string functionIndex = mConstSelect.mSelectRule.substr(FunctionIndexOffset, 2);
    int convertedIndex = -1;

    // Read in function index, form ## from 00 .. 73, skip leading zero
    if (functionIndex[0] == '0')
        functionIndex = functionIndex[1];

    std::stringstream stream;
    stream << functionIndex;
    stream >> convertedIndex;

    switch (functionPrefix)
    {
        case '1':
            if (convertedIndex >= 0 && convertedIndex <= 73)
                mFunctionName = static_cast<FunctionName>(convertedIndex);
            else
                mFunctionName = Function_None;
            break;

        case '2': mFunctionName = Function_Global; break;
        case '3': mFunctionName = Function_Local; break;
        case '4': mFunctionName = Function_Journal; break;
        case '5': mFunctionName = Function_Item; break;
        case '6': mFunctionName = Function_Dead; break;
        case '7': mFunctionName = Function_NotId; break;
        case '8': mFunctionName = Function_NotFaction; break;
        case '9': mFunctionName = Function_NotClass; break;
        case 'A': mFunctionName = Function_NotRace; break;
        case 'B': mFunctionName = Function_NotCell; break;
        case 'C': mFunctionName = Function_NotLocal; break;
        default:  mFunctionName = Function_None; break;
    }
}

void CSMWorld::ConstInfoSelectWrapper::readRelationType()
{
    char relationIndex = mConstSelect.mSelectRule[RelationIndexOffset];

    switch (relationIndex)
    {
        case '0': mRelationType = Relation_Equal; break;
        case '1': mRelationType = Relation_NotEqual; break;
        case '2': mRelationType = Relation_Greater; break;
        case '3': mRelationType = Relation_GreaterOrEqual; break;
        case '4': mRelationType = Relation_Less; break;
        case '5': mRelationType = Relation_LessOrEqual; break;
        default:  mRelationType = Relation_None;
    }
}

void CSMWorld::ConstInfoSelectWrapper::readVariableName()
{
    if (mConstSelect.mSelectRule.size() >= VarNameOffset)
        mVariableName = mConstSelect.mSelectRule.substr(VarNameOffset);
    else
        mVariableName.clear();
}

void CSMWorld::ConstInfoSelectWrapper::updateHasVariable()
{
    switch (mFunctionName)
    {
        case Function_Global:
        case Function_Local:
        case Function_Journal:
        case Function_Item:
        case Function_Dead:
        case Function_NotId:
        case Function_NotFaction:
        case Function_NotClass:
        case Function_NotRace:
        case Function_NotCell:
        case Function_NotLocal:
            mHasVariable = true;
            break;

        default:
            mHasVariable = false;
            break;
    }
}

void CSMWorld::ConstInfoSelectWrapper::updateComparisonType()
{
    switch (mFunctionName)
    {
        // Boolean
        case Function_NotId:
        case Function_NotFaction:
        case Function_NotClass:
        case Function_NotRace:
        case Function_NotCell:
        case Function_PcExpelled:
        case Function_PcCommonDisease:
        case Function_PcBlightDisease:
        case Function_SameSex:
        case Function_SameRace:
        case Function_SameFaction:
        case Function_Detected:
        case Function_Alarmed:
        case Function_PcCorpus:
        case Function_PcVampire:
        case Function_Attacked:
        case Function_TalkedToPc:
        case Function_ShouldAttack:
        case Function_Werewolf:
            mComparisonType = Comparison_Boolean;
            break;

        // Integer
        case Function_Journal:
        case Function_Item:
        case Function_Dead:
        case Function_RankLow:
        case Function_RankHigh:
        case Function_RankRequirement:
        case Function_Reputation:
        case Function_PcReputation:
        case Function_PcLevel:
        case Function_PcStrength:
        case Function_PcBlock:
        case Function_PcArmorer:
        case Function_PcMediumArmor:
        case Function_PcHeavyArmor:
        case Function_PcBluntWeapon:
        case Function_PcLongBlade:
        case Function_PcAxe:
        case Function_PcSpear:
        case Function_PcAthletics:
        case Function_PcEnchant:
        case Function_PcDestruction:
        case Function_PcAlteration:
        case Function_PcIllusion:
        case Function_PcConjuration:
        case Function_PcMysticism:
        case Function_PcRestoration:
        case Function_PcAlchemy:
        case Function_PcUnarmored:
        case Function_PcSecurity:
        case Function_PcSneak:
        case Function_PcAcrobatics:
        case Function_PcLightArmor:
        case Function_PcShortBlade:
        case Function_PcMarksman:
        case Function_PcMerchantile:
        case Function_PcSpeechcraft:
        case Function_PcHandToHand:
        case Function_PcGender:
        case Function_PcClothingModifier:
        case Function_PcCrimeLevel:
        case Function_FactionRankDifference:
        case Function_Choice:
        case Function_PcIntelligence:
        case Function_PcWillpower:
        case Function_PcAgility:
        case Function_PcSpeed:
        case Function_PcEndurance:
        case Function_PcPersonality:
        case Function_PcLuck:
        case Function_Weather:
        case Function_Level:
        case Function_CreatureTarget:
        case Function_FriendHit:
        case Function_Fight:
        case Function_Hello:
        case Function_Alarm:
        case Function_Flee:
        case Function_PcWerewolfKills:
            mComparisonType = Comparison_Integer;
            break;

        // Numeric
        case Function_Global:
        case Function_Local:
        case Function_NotLocal:

        case Function_Health_Percent:
        case Function_PcHealthPercent:
        case Function_PcMagicka:
        case Function_PcFatigue:
        case Function_PcHealth:
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

    int value = mConstSelect.mValue.getInteger();

    switch (mRelationType)
    {
        case Relation_Equal:
        case Relation_NotEqual:
            return std::pair<int, int>(value, value);

        case Relation_Greater:
            if (value == IntMax)
            {
                return InvalidRange;
            }
            else
            {
                return std::pair<int, int>(value + 1, IntMax);
            }
            break;

        case Relation_GreaterOrEqual:
            return std::pair<int, int>(value, IntMax);

        case Relation_Less:
            if (value == IntMin)
            {
                return InvalidRange;
            }
            else
            {
                return std::pair<int, int>(IntMin, value - 1);
            }

        case Relation_LessOrEqual:
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

    float value = mConstSelect.mValue.getFloat();

    switch (mRelationType)
    {
        case Relation_Equal:
        case Relation_NotEqual:
            return std::pair<float, float>(value, value);

        case Relation_Greater:
            return std::pair<float, float>(value + Epsilon, FloatMax);

        case Relation_GreaterOrEqual:
            return std::pair<float, float>(value, FloatMax);

        case Relation_Less:
            return std::pair<float, float>(FloatMin, value - Epsilon);

        case Relation_LessOrEqual:
            return std::pair<float, float>(FloatMin, value);

        default:
            throw std::logic_error("InfoSelectWrapper: given relation does not have a range");
    }
}

std::pair<int, int> CSMWorld::ConstInfoSelectWrapper::getValidIntRange() const
{
    const int IntMax = std::numeric_limits<int>::max();
    const int IntMin = std::numeric_limits<int>::min();

    switch (mFunctionName)
    {
        // Boolean
        case Function_NotId:
        case Function_NotFaction:
        case Function_NotClass:
        case Function_NotRace:
        case Function_NotCell:
        case Function_PcExpelled:
        case Function_PcCommonDisease:
        case Function_PcBlightDisease:
        case Function_SameSex:
        case Function_SameRace:
        case Function_SameFaction:
        case Function_Detected:
        case Function_Alarmed:
        case Function_PcCorpus:
        case Function_PcVampire:
        case Function_Attacked:
        case Function_TalkedToPc:
        case Function_ShouldAttack:
        case Function_Werewolf:
            return std::pair<int, int>(0, 1);

        // Integer
        case Function_RankLow:
        case Function_RankHigh:
        case Function_Reputation:
        case Function_PcReputation:
        case Function_Journal:
            return std::pair<int, int>(IntMin, IntMax);

        case Function_Item:
        case Function_Dead:
        case Function_PcLevel:
        case Function_PcStrength:
        case Function_PcBlock:
        case Function_PcArmorer:
        case Function_PcMediumArmor:
        case Function_PcHeavyArmor:
        case Function_PcBluntWeapon:
        case Function_PcLongBlade:
        case Function_PcAxe:
        case Function_PcSpear:
        case Function_PcAthletics:
        case Function_PcEnchant:
        case Function_PcDestruction:
        case Function_PcAlteration:
        case Function_PcIllusion:
        case Function_PcConjuration:
        case Function_PcMysticism:
        case Function_PcRestoration:
        case Function_PcAlchemy:
        case Function_PcUnarmored:
        case Function_PcSecurity:
        case Function_PcSneak:
        case Function_PcAcrobatics:
        case Function_PcLightArmor:
        case Function_PcShortBlade:
        case Function_PcMarksman:
        case Function_PcMerchantile:
        case Function_PcSpeechcraft:
        case Function_PcHandToHand:
        case Function_PcClothingModifier:
        case Function_PcCrimeLevel:
        case Function_Choice:
        case Function_PcIntelligence:
        case Function_PcWillpower:
        case Function_PcAgility:
        case Function_PcSpeed:
        case Function_PcEndurance:
        case Function_PcPersonality:
        case Function_PcLuck:
        case Function_Level:
        case Function_PcWerewolfKills:
            return std::pair<int, int>(0, IntMax);

        case Function_Fight:
        case Function_Hello:
        case Function_Alarm:
        case Function_Flee:
            return std::pair<int, int>(0, 100);

        case Function_Weather:
            return std::pair<int, int>(0, 9);

        case Function_FriendHit:
            return std::pair<int, int>(0, 4);

        case Function_RankRequirement:
            return std::pair<int, int>(0, 3);

        case Function_CreatureTarget:
            return std::pair<int, int>(0, 2);

        case Function_PcGender:
            return std::pair<int, int>(0, 1);

        case Function_FactionRankDifference:
            return std::pair<int, int>(-9, 9);

        // Numeric
        case Function_Global:
        case Function_Local:
        case Function_NotLocal:
            return std::pair<int, int>(IntMin, IntMax);

        case Function_PcMagicka:
        case Function_PcFatigue:
        case Function_PcHealth:
            return std::pair<int, int>(0, IntMax);

        case Function_Health_Percent:
        case Function_PcHealthPercent:
            return std::pair<int, int>(0, 100);

        default:
            throw std::runtime_error("InfoSelectWrapper: function does not exist");
    }
}

std::pair<float, float> CSMWorld::ConstInfoSelectWrapper::getValidFloatRange() const
{
    const float FloatMax = std::numeric_limits<float>::infinity();
    const float FloatMin = -std::numeric_limits<float>::infinity();

    switch (mFunctionName)
    {
        // Numeric
        case Function_Global:
        case Function_Local:
        case Function_NotLocal:
            return std::pair<float, float>(FloatMin, FloatMax);

        case Function_PcMagicka:
        case Function_PcFatigue:
        case Function_PcHealth:
            return std::pair<float, float>(0, FloatMax);

        case Function_Health_Percent:
        case Function_PcHealthPercent:
            return std::pair<float, float>(0, 100);

        default:
            throw std::runtime_error("InfoSelectWrapper: function does not exist or is not numeric");
    }
}

template <typename T1, typename T2>
bool CSMWorld::ConstInfoSelectWrapper::rangeContains(T1 value, std::pair<T2,T2> range) const
{
    return (value >= range.first && value <= range.second);
}

template <typename T1, typename T2>
bool CSMWorld::ConstInfoSelectWrapper::rangeFullyContains(std::pair<T1,T1> containingRange,
    std::pair<T2,T2> testRange) const
{
    return (containingRange.first <= testRange.first) && (testRange.second <= containingRange.second);
}

template <typename T1, typename T2>
bool CSMWorld::ConstInfoSelectWrapper::rangesOverlap(std::pair<T1,T1> range1, std::pair<T2,T2> range2) const
{
    // One of the bounds of either range should fall within the other range
    return
        (range1.first <= range2.first  && range2.first  <= range1.second) ||
        (range1.first <= range2.second && range2.second <= range1.second) ||
        (range2.first <= range1.first  && range1.first  <= range2.second) ||
        (range2.first <= range1.second && range1.second <= range2.second);
}

template <typename T1, typename T2>
bool CSMWorld::ConstInfoSelectWrapper::rangesMatch(std::pair<T1,T1> range1, std::pair<T2,T2> range2) const
{
    return (range1.first == range2.first  && range1.second == range2.second);
}

template <typename T1, typename T2>
bool CSMWorld::ConstInfoSelectWrapper::conditionIsAlwaysTrue(std::pair<T1,T1> conditionRange,
    std::pair<T2,T2> validRange) const
{
    switch (mRelationType)
    {
        case Relation_Equal:
            return false;

        case Relation_NotEqual:
            // If value is not within range, it will always be true
            return !rangeContains(conditionRange.first, validRange);

        case Relation_Greater:
        case Relation_GreaterOrEqual:
        case Relation_Less:
        case Relation_LessOrEqual:
            // If the valid range is completely within the condition range, it will always be true
            return rangeFullyContains(conditionRange, validRange);

        default:
            throw std::logic_error("InfoCondition: operator can not be used to compare");
    }

    return false;
}

template <typename T1, typename T2>
bool CSMWorld::ConstInfoSelectWrapper::conditionIsNeverTrue(std::pair<T1,T1> conditionRange,
    std::pair<T2,T2> validRange) const
{
    switch (mRelationType)
    {
        case Relation_Equal:
            return !rangeContains(conditionRange.first, validRange);

        case Relation_NotEqual:
            return false;

        case Relation_Greater:
        case Relation_GreaterOrEqual:
        case Relation_Less:
        case Relation_LessOrEqual:
            // If ranges do not overlap, it will never be true
            return !rangesOverlap(conditionRange, validRange);

        default:
            throw std::logic_error("InfoCondition: operator can not be used to compare");
    }

    return false;
}

// InfoSelectWrapper

CSMWorld::InfoSelectWrapper::InfoSelectWrapper(ESM::DialInfo::SelectStruct& select)
    : CSMWorld::ConstInfoSelectWrapper(select), mSelect(select)
{
}

void CSMWorld::InfoSelectWrapper::setFunctionName(FunctionName name)
{
    mFunctionName = name;
    updateHasVariable();
    updateComparisonType();
}

void CSMWorld::InfoSelectWrapper::setRelationType(RelationType type)
{
    mRelationType = type;
}

void CSMWorld::InfoSelectWrapper::setVariableName(const std::string& name)
{
    mVariableName = name;
}

void CSMWorld::InfoSelectWrapper::setDefaults()
{
    if (!variantTypeIsValid())
        mSelect.mValue.setType(ESM::VT_Int);

    switch (mComparisonType)
    {
        case Comparison_Boolean:
            setRelationType(Relation_Equal);
            mSelect.mValue.setInteger(1);
            break;

        case Comparison_Integer:
        case Comparison_Numeric:
            setRelationType(Relation_Greater);
            mSelect.mValue.setInteger(0);
            break;

        default:
            // Do nothing
            break;
    }

    update();
}

void CSMWorld::InfoSelectWrapper::update()
{
    std::ostringstream stream;

    // Leading 0
    stream << '0';

    // Write Function

    bool writeIndex = false;
    size_t functionIndex = static_cast<size_t>(mFunctionName);

    switch (mFunctionName)
    {
        case Function_None:         stream << '0'; break;
        case Function_Global:       stream << '2'; break;
        case Function_Local:        stream << '3'; break;
        case Function_Journal:      stream << '4'; break;
        case Function_Item:         stream << '5'; break;
        case Function_Dead:         stream << '6'; break;
        case Function_NotId:        stream << '7'; break;
        case Function_NotFaction:   stream << '8'; break;
        case Function_NotClass:     stream << '9'; break;
        case Function_NotRace:      stream << 'A'; break;
        case Function_NotCell:      stream << 'B'; break;
        case Function_NotLocal:     stream << 'C'; break;
        default:                    stream << '1'; writeIndex = true; break;
    }

    if (writeIndex && functionIndex < 10) // leading 0
        stream << '0' << functionIndex;
    else if (writeIndex)
        stream << functionIndex;
    else
        stream << "00";

    // Write Relation
    switch (mRelationType)
    {
        case Relation_Equal:            stream << '0'; break;
        case Relation_NotEqual:         stream << '1'; break;
        case Relation_Greater:          stream << '2'; break;
        case Relation_GreaterOrEqual:   stream << '3'; break;
        case Relation_Less:             stream << '4'; break;
        case Relation_LessOrEqual:      stream << '5'; break;
        default:                        stream << '0'; break;
    }

    if (mHasVariable)
        stream << mVariableName;

    mSelect.mSelectRule = stream.str();
}

ESM::Variant& CSMWorld::InfoSelectWrapper::getVariant()
{
    return mSelect.mValue;
}
