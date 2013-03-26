
#include "globals.hpp"

#include <stdexcept>

#include "esmstore.hpp"

namespace MWWorld
{
    std::vector<std::string> Globals::getGlobals () const
    {
        std::vector<std::string> retval;
        Collection::const_iterator it;
        for(it = mVariables.begin(); it != mVariables.end(); ++it){
            retval.push_back(it->first);
        }

        return retval;
    }

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

    Globals::Globals (const MWWorld::ESMStore& store)
    {
        const MWWorld::Store<ESM::Global> &globals = store.get<ESM::Global>();
        MWWorld::Store<ESM::Global>::iterator iter = globals.begin();
        for (; iter != globals.end(); ++iter)
        {
            char type = ' ';
            Data value;

            switch (iter->mValue.getType())
            {
                case ESM::VT_Short:

                    type = 's';
                    value.mShort = iter->mValue.getInteger();
                    break;

                case ESM::VT_Long:

                    type = 'l';
                    value.mLong = iter->mValue.getInteger();
                    break;

                case ESM::VT_Float:

                    type = 'f';
                    value.mFloat = iter->mValue.getFloat();
                    break;

                default:

                    throw std::runtime_error ("unsupported global variable type");
            }

            mVariables.insert (std::make_pair (iter->mId, std::make_pair (type, value)));
        }

        if (mVariables.find ("dayspassed")==mVariables.end())
        {
            // vanilla Morrowind does not define dayspassed.
            Data value;
            value.mLong = 1; // but the addons start counting at 1 :(

            mVariables.insert (std::make_pair ("dayspassed", std::make_pair ('l', value)));
        }
    }

    const Globals::Data& Globals::operator[] (const std::string& name) const
    {
        Collection::const_iterator iter = find (name);

        return iter->second.second;
    }

    Globals::Data& Globals::operator[] (const std::string& name)
    {
        Collection::iterator iter = find (name);

        return iter->second.second;
    }

    void Globals::setInt (const std::string& name, int value)
    {
        Collection::iterator iter = find (name);

        switch (iter->second.first)
        {
            case 's': iter->second.second.mShort = value; break;
            case 'l': iter->second.second.mLong = value; break;
            case 'f': iter->second.second.mFloat = value; break;

            default: throw std::runtime_error ("unsupported global variable type");
        }
    }

    void Globals::setFloat (const std::string& name, float value)
    {
        Collection::iterator iter = find (name);

        switch (iter->second.first)
        {
            case 's': iter->second.second.mShort = value; break;
            case 'l': iter->second.second.mLong = value; break;
            case 'f': iter->second.second.mFloat = value; break;

            default: throw std::runtime_error ("unsupported global variable type");
        }
    }

    int Globals::getInt (const std::string& name) const
    {
        Collection::const_iterator iter = find (name);

        switch (iter->second.first)
        {
            case 's': return iter->second.second.mShort;
            case 'l': return iter->second.second.mLong;
            case 'f': return iter->second.second.mFloat;

            default: throw std::runtime_error ("unsupported global variable type");
        }
    }

    float Globals::getFloat (const std::string& name) const
    {
        Collection::const_iterator iter = find (name);

        switch (iter->second.first)
        {
            case 's': return iter->second.second.mShort;
            case 'l': return iter->second.second.mLong;
            case 'f': return iter->second.second.mFloat;

            default: throw std::runtime_error ("unsupported global variable type");
        }
    }

    char Globals::getType (const std::string& name) const
    {
        Collection::const_iterator iter = mVariables.find (name);

        if (iter==mVariables.end())
            return ' ';

        return iter->second.first;
    }
}

