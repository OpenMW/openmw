
#include "selectwrapper.hpp"

#include <stdexcept>

namespace
{
    template<typename T1, typename T2>
    bool selectCompareImp (char comp, T1 value1, T2 value2)
    {
        switch (comp)
        {
            case '0': return value1==value2;
            case '1': return value1!=value2;
            case '2': return value1>value2;
            case '3': return value1>=value2;
            case '4': return value1<value2;
            case '5': return value1<=value2;
        }

        throw std::runtime_error ("unknown compare type in dialogue info select");
    }
    
    template<typename T>
    bool selectCompareImp (const ESM::DialInfo::SelectStruct& select, T value1)
    {
        if (select.mType==ESM::VT_Short || select.mType==ESM::VT_Int ||
            select.mType==ESM::VT_Long)
        {
            return selectCompareImp (select.mSelectRule[4], value1, select.mI);
        }
        else if (select.mType==ESM::VT_Float)
        {
            return selectCompareImp (select.mSelectRule[4], value1, select.mF);
        }
        else
            throw std::runtime_error (
                "unsupported variable type in dialogue info select");        
    }
}

MWDialogue::SelectWrapper::SelectWrapper (const ESM::DialInfo::SelectStruct& select) : mSelect (select) {}

MWDialogue::SelectWrapper::Function MWDialogue::SelectWrapper::getFunction() const
{
    return Function_None;
}

MWDialogue::SelectWrapper::Type MWDialogue::SelectWrapper::getType() const
{
    return Type_None;
}

bool MWDialogue::SelectWrapper::IsInverted() const
{
    return false;
}

bool MWDialogue::SelectWrapper::selectCompare (int value) const
{
    return selectCompareImp (mSelect, value)!=IsInverted(); // logic XOR
}

bool MWDialogue::SelectWrapper::selectCompare (float value) const
{
    return selectCompareImp (mSelect, value)!=IsInverted(); // logic XOR
}
