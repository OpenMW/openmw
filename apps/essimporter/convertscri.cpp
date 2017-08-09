#include "convertscri.hpp"

#include <iostream>

namespace
{

    template <typename T, ESM::VarType VariantType>
    void storeVariables(const std::vector<T>& variables, ESM::Locals& locals, const std::string& scriptname)
    {
        for (typename std::vector<T>::const_iterator it = variables.begin(); it != variables.end(); ++it)
        {
            ESM::Variant val(*it);
            val.setType(VariantType);
            locals.mVariables.push_back(std::make_pair(std::string(), val));
        }
    }

}

namespace ESSImport
{

    void convertSCRI(const SCRI &scri, ESM::Locals &locals)
    {
        // order *is* important, as we do not have variable names available in this format
        storeVariables<short, ESM::VT_Short> (scri.mShorts, locals, scri.mScript);
        storeVariables<int, ESM::VT_Int> (scri.mLongs, locals, scri.mScript);
        storeVariables<float, ESM::VT_Float> (scri.mFloats, locals, scri.mScript);
    }

}
