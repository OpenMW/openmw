#ifndef CSM_WORLD_INFOSELECTWRAPPER_H
#define CSM_WORLD_INFOSELECTWRAPPER_H

#include <stddef.h>
#include <string>
#include <utility>

#include <components/esm3/loadinfo.hpp>

#include <QVariant>

namespace CSMWorld
{
    class ConstInfoSelectWrapper
    {
    public:
        enum ComparisonType
        {
            Comparison_Boolean,
            Comparison_Integer,
            Comparison_Numeric,

            Comparison_None
        };

        static const char* FunctionEnumStrings[];
        static const char* RelationEnumStrings[];

        ConstInfoSelectWrapper(const ESM::DialogueCondition& select);

        ESM::DialogueCondition::Function getFunctionName() const;
        ESM::DialogueCondition::Comparison getRelationType() const;
        ComparisonType getComparisonType() const;

        bool hasVariable() const;
        const std::string& getVariableName() const;

        bool conditionIsAlwaysTrue() const;
        bool conditionIsNeverTrue() const;

        QVariant getValue() const;

        std::string toString() const;

    protected:
        void updateHasVariable();
        void updateComparisonType();

        std::pair<int, int> getConditionIntRange() const;
        std::pair<float, float> getConditionFloatRange() const;

        std::pair<int, int> getValidIntRange() const;
        std::pair<float, float> getValidFloatRange() const;

        template <typename Type1, typename Type2>
        bool rangeContains(Type1 value, std::pair<Type2, Type2> range) const;

        template <typename Type1, typename Type2>
        bool rangesOverlap(std::pair<Type1, Type1> range1, std::pair<Type2, Type2> range2) const;

        template <typename Type1, typename Type2>
        bool rangeFullyContains(std::pair<Type1, Type1> containing, std::pair<Type2, Type2> test) const;

        template <typename Type1, typename Type2>
        bool rangesMatch(std::pair<Type1, Type1> range1, std::pair<Type2, Type2> range2) const;

        template <typename Type1, typename Type2>
        bool conditionIsAlwaysTrue(std::pair<Type1, Type1> conditionRange, std::pair<Type2, Type2> validRange) const;

        template <typename Type1, typename Type2>
        bool conditionIsNeverTrue(std::pair<Type1, Type1> conditionRange, std::pair<Type2, Type2> validRange) const;

        ComparisonType mComparisonType;

        bool mHasVariable;

    private:
        const ESM::DialogueCondition& mConstSelect;
    };

    // Wrapper for DialogueCondition that can modify the wrapped select struct
    class InfoSelectWrapper : public ConstInfoSelectWrapper
    {
    public:
        InfoSelectWrapper(ESM::DialogueCondition& select);

        // Wrapped SelectStruct will not be modified until update() is called
        void setFunctionName(ESM::DialogueCondition::Function name);
        void setRelationType(ESM::DialogueCondition::Comparison type);
        void setVariableName(const std::string& name);
        void setValue(int value);
        void setValue(float value);

    private:
        ESM::DialogueCondition& mSelect;

        void writeRule();
    };
}

#endif
