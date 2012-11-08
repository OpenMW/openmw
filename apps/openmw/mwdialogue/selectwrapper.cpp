
#include "selectwrapper.hpp"

#include <cctype>

#include <stdexcept>
#include <algorithm>

namespace
{
    std::string toLower (const std::string& name)
    {
        std::string lowerCase;

        std::transform (name.begin(), name.end(), std::back_inserter (lowerCase),
            (int(*)(int)) std::tolower);

        return lowerCase;
    }

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
    char type = mSelect.mSelectRule[1];

    switch (type)
    {
        case '4': return Function_Journal;
        case '5': return Function_Item;
    }

    return Function_None;
}

MWDialogue::SelectWrapper::Type MWDialogue::SelectWrapper::getType() const
{
    static const Function integerFunctions[] =
    {
        Function_Journal, Function_Item,
        Function_None // end marker
    };
    
    static const Function numericFunctions[] =
    {
        Function_None // end marker
    };    
    
    static const Function booleanFunctions[] =
    {
        Function_None // end marker
    };   
    
    Function function = getFunction();
    
    if (function==Function_None)
        return Type_None;
        
    for (int i=0; integerFunctions[i]!=Function_None; ++i)
        if (integerFunctions[i]==function)
            return Type_Integer;
    
    for (int i=0; numericFunctions[i]!=Function_None; ++i)
        if (numericFunctions[i]==function)
            return Type_Numeric;

    for (int i=0; booleanFunctions[i]!=Function_None; ++i)
        if (booleanFunctions[i]==function)
            return Type_Boolean;
    
    throw std::runtime_error ("failed to determine type of select function");
}

bool MWDialogue::SelectWrapper::IsInverted() const
{
    char type = mSelect.mSelectRule[1];

    return type=='7' || type=='8' || type=='9' || type=='A' || type=='B' || type=='C';
}

bool MWDialogue::SelectWrapper::selectCompare (int value) const
{
    return selectCompareImp (mSelect, value)!=IsInverted(); // logic XOR
}

bool MWDialogue::SelectWrapper::selectCompare (float value) const
{
    return selectCompareImp (mSelect, value)!=IsInverted(); // logic XOR
}

bool MWDialogue::SelectWrapper::selectCompare (bool value) const
{
    return selectCompareImp (mSelect, static_cast<int> (value))!=IsInverted(); // logic XOR
}

std::string MWDialogue::SelectWrapper::getName() const
{
    return toLower (mSelect.mSelectRule.substr (5));
}
