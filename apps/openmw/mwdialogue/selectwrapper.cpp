
#include "selectwrapper.hpp"

#include <cctype>

#include <stdexcept>
#include <algorithm>
#include <sstream>

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

MWDialogue::SelectWrapper::Function MWDialogue::SelectWrapper::decodeFunction() const
{
    int index = 0;
    
    std::istringstream (mSelect.mSelectRule.substr(2,2)) >> index;
    
    return static_cast<Function> (index);
}

MWDialogue::SelectWrapper::SelectWrapper (const ESM::DialInfo::SelectStruct& select) : mSelect (select) {}

MWDialogue::SelectWrapper::Function MWDialogue::SelectWrapper::getFunction() const
{
    char type = mSelect.mSelectRule[1];

    switch (type)
    {
        case '1': return decodeFunction();
        case '2': return Function_Global;
        case '3': return Function_Local;
        case '4': return Function_Journal;
        case '5': return Function_Item;
        case '6': return Function_Dead;
        case '7': return Function_Id;
        case '8': return Function_Faction;
        case '9': return Function_Class;
        case 'A': return Function_Race;
        case 'B': return Function_Cell;
        case 'C': return Function_Local;
    }

    return Function_None;
}

MWDialogue::SelectWrapper::Type MWDialogue::SelectWrapper::getType() const
{
    static const Function integerFunctions[] =
    {
        Function_Journal, Function_Item, Function_Dead,
        Function_None // end marker
    };
    
    static const Function numericFunctions[] =
    {
        Function_Global, Function_Local,
        Function_None // end marker
    };    
    
    static const Function booleanFunctions[] =
    {
        Function_Id, Function_Faction, Function_Class, Function_Race, Function_Cell,
        Function_SameFaction,
        Function_None // end marker
    };   
    
    Function function = getFunction();

    for (int i=0; integerFunctions[i]!=Function_None; ++i)
        if (integerFunctions[i]==function)
            return Type_Integer;
    
    for (int i=0; numericFunctions[i]!=Function_None; ++i)
        if (numericFunctions[i]==function)
            return Type_Numeric;

    for (int i=0; booleanFunctions[i]!=Function_None; ++i)
        if (booleanFunctions[i]==function)
            return Type_Boolean;
    
    return Type_None;
}

bool MWDialogue::SelectWrapper::isInverted() const
{
    char type = mSelect.mSelectRule[1];

    return type=='7' || type=='8' || type=='9' || type=='A' || type=='B' || type=='C';
}

bool MWDialogue::SelectWrapper::isNpcOnly() const
{
    static const Function functions[] =
    {
        Function_Faction, SelectWrapper::Function_Class, SelectWrapper::Function_Race,
        Function_SameFaction,
        Function_None // end marker
    };

    Function function = getFunction();
    
    for (int i=0; functions[i]!=Function_None; ++i)
        if (functions[i]==function)
            return true;
    
    return false;
}

bool MWDialogue::SelectWrapper::selectCompare (int value) const
{
    return selectCompareImp (mSelect, value)!=isInverted(); // logic XOR
}

bool MWDialogue::SelectWrapper::selectCompare (float value) const
{
    return selectCompareImp (mSelect, value)!=isInverted(); // logic XOR
}

bool MWDialogue::SelectWrapper::selectCompare (bool value) const
{
    return selectCompareImp (mSelect, static_cast<int> (value))!=isInverted(); // logic XOR
}

std::string MWDialogue::SelectWrapper::getName() const
{
    return toLower (mSelect.mSelectRule.substr (5));
}
