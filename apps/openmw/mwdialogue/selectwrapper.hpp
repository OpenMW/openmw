#ifndef GAME_MWDIALOGUE_SELECTWRAPPER_H
#define GAME_MWDIALOGUE_SELECTWRAPPER_H

#include <components/esm3/loadinfo.hpp>

namespace MWDialogue
{
    class SelectWrapper
    {
        const ESM::DialogueCondition& mSelect;

    public:
        enum Type
        {
            Type_None,
            Type_Integer,
            Type_Numeric,
            Type_Boolean,
            Type_Inverted
        };

    public:
        SelectWrapper(const ESM::DialogueCondition& select);

        ESM::DialogueCondition::Function getFunction() const;

        int getArgument() const;

        Type getType() const;

        bool selectCompare(int value) const;

        bool selectCompare(float value) const;

        bool selectCompare(bool value) const;

        std::string getName() const;
        ///< Return case-smashed name.

        std::string_view getCellName() const;

        ESM::RefId getId() const;
    };
}

#endif
