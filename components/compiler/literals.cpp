
#include "literals.hpp"

#include <algorithm>

namespace Compiler
{
    int Literals::getIntegerSize() const
    {
        return mIntegers.size() * sizeof (Interpreter::Type_Integer);
    }

    int Literals::getFloatSize() const
    {
        return mFloats.size() * sizeof (Interpreter::Type_Float);
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
        for (std::vector<Interpreter::Type_Integer>::const_iterator iter (mIntegers.begin());
            iter!=mIntegers.end(); ++iter)
            code.push_back (*reinterpret_cast<const Interpreter::Type_Code *> (&*iter));
            
        for (std::vector<Interpreter::Type_Float>::const_iterator iter (mFloats.begin());
            iter!=mFloats.end(); ++iter)
            code.push_back (*reinterpret_cast<const Interpreter::Type_Code *> (&*iter));
            
        int stringBlockSize = getStringSize();
        int size = static_cast<int> (code.size());
        
        code.resize (size+stringBlockSize/4);
        
        int offset = 0;
        
        for (std::vector<std::string>::const_iterator iter (mStrings.begin());
            iter!=mStrings.end(); ++iter)        
        {
            int stringSize = iter->size()+1;
            
            std::copy (iter->c_str(), iter->c_str()+stringSize,
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

