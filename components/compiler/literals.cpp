#include "literals.hpp"

#include <algorithm>

namespace Compiler
{
    int Literals::getIntegerSize() const
    {
        return static_cast<int>(mIntegers.size() * sizeof (Interpreter::Type_Integer));
    }

    int Literals::getFloatSize() const
    {
        return static_cast<int>(mFloats.size() * sizeof (Interpreter::Type_Float));
    }

    int Literals::getStringSize() const
    {
        int size = 0;
        
        for (std::vector<std::string>::const_iterator iter (mStrings.begin());
            iter!=mStrings.end(); ++iter)
            size += static_cast<int> (iter->size()) + 1;
            
        if (size % 4) // padding
            size += 4 - size % 4;
            
        return size;
    }

    void Literals::append (std::vector<Interpreter::Type_Code>& code) const
    {
        for (const int & mInteger : mIntegers)
            code.push_back (*reinterpret_cast<const Interpreter::Type_Code *> (&mInteger));
            
        for (const float & mFloat : mFloats)
            code.push_back (*reinterpret_cast<const Interpreter::Type_Code *> (&mFloat));
            
        int stringBlockSize = getStringSize();
        int size = static_cast<int> (code.size());
        
        code.resize (size+stringBlockSize/4);
        
        size_t offset = 0;
        
        for (const auto & mString : mStrings)
        {
            size_t stringSize = mString.size()+1;
            
            std::copy (mString.c_str(), mString.c_str()+stringSize,
                reinterpret_cast<char *> (&code[size]) + offset);
            offset += stringSize;
        }
    }

    int Literals::addInteger (Interpreter::Type_Integer value)
    {
        int index = static_cast<int> (mIntegers.size());
        
        mIntegers.push_back (value);
        
        return index;    
    }

    int Literals::addFloat (Interpreter::Type_Float value)
    {
        int index = static_cast<int> (mFloats.size());
        
        mFloats.push_back (value);
        
        return index;    
    }

    int Literals::addString (const std::string& value)
    {
        int index = static_cast<int> (mStrings.size());
        
        mStrings.push_back (value);
        
        return index;
    }

    void Literals::clear()
    {
        mIntegers.clear();
        mFloats.clear();
        mStrings.clear();
    }
}

