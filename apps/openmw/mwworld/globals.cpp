
#include "globals.hpp"

#include <stdexcept>

#include "esmstore.hpp"

namespace MWWorld
{
    Globals::Collection::const_iterator Globals::find (const std::string& name) const
    {
        Collection::const_iterator iter = mVariables.find (name);

        if (iter==mVariables.end())
            throw std::runtime_error ("unknown global variable: " + name);

        return iter;
    }

    Globals::Collection::iterator Globals::find (const std::string& name)
    {
        Collection::iterator iter = mVariables.find (name);

        if (iter==mVariables.end())
            throw std::runtime_error ("unknown global variable: " + name);

        return iter;
    }

    void Globals::fill (const MWWorld::ESMStore& store)
    {
        mVariables.clear();

        const MWWorld::Store<ESM::Global>& globals = store.get<ESM::Global>();

        for (MWWorld::Store<ESM::Global>::iterator iter = globals.begin(); iter!=globals.end();
            ++iter)
        {
            mVariables.insert (std::make_pair (iter->mId, iter->mValue));
        }
    }

    const ESM::Variant& Globals::operator[] (const std::string& name) const
    {
        return find (name)->second;
    }

    ESM::Variant& Globals::operator[] (const std::string& name)
    {
        return find (name)->second;
    }

    char Globals::getType (const std::string& name) const
    {
        Collection::const_iterator iter = mVariables.find (name);

        if (iter==mVariables.end())
            return ' ';

        switch (iter->second.getType())
        {
            case ESM::VT_Short: return 's';
            case ESM::VT_Long: return 'l';
            case ESM::VT_Float: return 'f';

            default: return ' ';
        }
    }
}
