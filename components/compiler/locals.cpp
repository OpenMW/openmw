
#include "locals.hpp"

#include <cassert>
#include <stdexcept>
#include <algorithm>

namespace Compiler
{
    const std::vector<std::string>& Locals::get (char type) const
    {
        switch (type)
        {
            case 's': return mShorts;
            case 'l': return mLongs;
            case 'f': return mFloats;        
        }
        
        throw std::logic_error ("unknown variable type");
    }
    
    bool Locals::search (char type, const std::string& name) const
    {
        const std::vector<std::string>& collection = get (type);
        
        return std::find (collection.begin(), collection.end(), name)!=collection.end();
    }

    std::vector<std::string>& Locals::get (char type)
    {
        switch (type)
        {
            case 's': return mShorts;
            case 'l': return mLongs;
            case 'f': return mFloats;        
        }
        
        throw std::logic_error ("unknown variable type");    
    }

    char Locals::getType (const std::string& name) const
    {
        if (search ('s', name))
            return 's';

        if (search ('l', name))
            return 'l';
        
        if (search ('f', name))
            return 'f';
        
        return ' ';
    }
    
    void Locals::declare (char type, const std::string& name)
    {
        get (type).push_back (name);
    }
    
    void Locals::clear()
    {
        get ('s').clear();
        get ('l').clear();
        get ('f').clear();
    }
}

