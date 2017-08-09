#ifndef CSM_WORLD_INFOSELECTWRAPPER_H
#define CSM_WORLD_INFOSELECTWRAPPER_H

#include <components/esm/loadinfo.hpp>

namespace CSMWorld
{
    // ESM::DialInfo::SelectStruct.mSelectRule
    // 012345...
    // ^^^ ^^
    // ||| ||
    // ||| |+------------- condition variable string
    // ||| +-------------- comparison type, ['0'..'5']; e.g. !=, <, >=, etc
    // ||+---------------- function index (encoded, where function == '1')
    // |+----------------- function, ['1'..'C']; e.g. Global, Local, Not ID, etc
    // +------------------ unknown
    //

    // Wrapper for DialInfo::SelectStruct
    class ConstInfoSelectWrapper
    {
    public:

        // Order matters
        enum FunctionName
        {
            Function_RankLow=0,
            Function_RankHigh,
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
            Function_PcMerchantile,
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
            Function_PcCorpus,
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
            Function_PcWerewolfKills=73,

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

            Function_None
        };

        enum RelationType
        {
            Relation_Equal,
            Relation_NotEqual,
            Relation_Greater,
            Relation_GreaterOrEqual,
            Relation_Less,
            Relation_LessOrEqual,

            Relation_None
        };

        enum ComparisonType
        {
            Comparison_Boolean,
            Comparison_Integer,
            Comparison_Numeric,

            Comparison_None
        };

        static const size_t RuleMinSize;

        static const size_t FunctionPrefixOffset;
        static const size_t FunctionIndexOffset;
        static const size_t RelationIndexOffset;
        static const size_t VarNameOffset;

        static const char* FunctionEnumStrings[];
        static const char* RelationEnumStrings[];
        static const char* ComparisonEnumStrings[];

        static std::string convertToString(FunctionName name);
        static std::string convertToString(RelationType type);
        static std::string convertToString(ComparisonType type);

        ConstInfoSelectWrapper(const ESM::DialInfo::SelectStruct& select);

        FunctionName getFunctionName() const;
        RelationType getRelationType() const;
        ComparisonType getComparisonType() const;

        bool hasVariable() const;
        const std::string& getVariableName() const;

        bool conditionIsAlwaysTrue() const;
        bool conditionIsNeverTrue() const;
        bool variantTypeIsValid() const;

        const ESM::Variant& getVariant() const;

        std::string toString() const;

    protected:

        void readRule();
        void readFunctionName();
        void readRelationType();
        void readVariableName();
        void updateHasVariable();
        void updateComparisonType();

        std::pair<int, int> getConditionIntRange() const;
        std::pair<float, float> getConditionFloatRange() const;

        std::pair<int, int> getValidIntRange() const;
        std::pair<float, float> getValidFloatRange() const;

        template <typename Type1, typename Type2>
        bool rangeContains(Type1 value, std::pair<Type2,Type2> range) const;

        template <typename Type1, typename Type2>
        bool rangesOverlap(std::pair<Type1,Type1> range1, std::pair<Type2,Type2> range2) const;

        template <typename Type1, typename Type2>
        bool rangeFullyContains(std::pair<Type1,Type1> containing, std::pair<Type2,Type2> test) const;

        template <typename Type1, typename Type2>
        bool rangesMatch(std::pair<Type1,Type1> range1, std::pair<Type2,Type2> range2) const;

        template <typename Type1, typename Type2>
        bool conditionIsAlwaysTrue(std::pair<Type1,Type1> conditionRange, std::pair<Type2,Type2> validRange) const;

        template <typename Type1, typename Type2>
        bool conditionIsNeverTrue(std::pair<Type1,Type1> conditionRange, std::pair<Type2,Type2> validRange) const;

        FunctionName mFunctionName;
        RelationType mRelationType;
        ComparisonType mComparisonType;

        bool mHasVariable;
        std::string mVariableName;

    private:

        const ESM::DialInfo::SelectStruct& mConstSelect;
    };

    // Wrapper for DialInfo::SelectStruct that can modify the wrapped select struct
    class InfoSelectWrapper : public ConstInfoSelectWrapper
    {
    public:

        InfoSelectWrapper(ESM::DialInfo::SelectStruct& select);

        // Wrapped SelectStruct will not be modified until update() is called
        void setFunctionName(FunctionName name);
        void setRelationType(RelationType type);
        void setVariableName(const std::string& name);

        // Modified wrapped SelectStruct
        void update();

        // This sets properties based on the function name to its defaults and updates the wrapped object
        void setDefaults();

        ESM::Variant& getVariant();

    private:

        ESM::DialInfo::SelectStruct& mSelect;

        void writeRule();
    };
}

#endif
