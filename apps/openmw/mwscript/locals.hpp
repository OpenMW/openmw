#ifndef GAME_SCRIPT_LOCALS_H
#define GAME_SCRIPT_LOCALS_H

#include <vector>

#include <components/esm/loadscpt.hpp>
#include <components/interpreter/types.hpp>

namespace MWScript
{
    class Locals
    {
        public:
            std::vector<Interpreter::Type_Short> mShorts;
            std::vector<Interpreter::Type_Integer> mLongs;
            std::vector<Interpreter::Type_Float> mFloats;
            
            void configure (const ESM::Script& script);
            bool setVarByInt(const std::string& script, const std::string& var, int val);
        
    };
}

#endif

