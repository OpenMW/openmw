#ifndef INTERPRETER_TYPES_H_INCLUDED
#define INTERPRETER_TYPES_H_INCLUDED

#include <stdexcept>

namespace Interpreter
{
    typedef unsigned int Type_Code; // 32 bit

    typedef unsigned int Type_Data; // 32 bit
    
    typedef short Type_Short; // 16 bit
    
    typedef int Type_Integer; // 32 bit
    
    typedef float Type_Float; // 32 bit
    
    union Data
    {
        Type_Integer mInteger;
        Type_Float mFloat;
    };
    
    template<typename T>
    T& getData (Data& data)
    {
        throw std::runtime_error ("unsupported data type");
    }
    
    template<>
    inline Type_Integer& getData (Data& data)
    {
        return data.mInteger;
    }
    
    template<>
    inline Type_Float& getData (Data& data)
    {
        return data.mFloat;
    }    
}

#endif

