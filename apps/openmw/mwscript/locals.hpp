#ifndef GAME_SCRIPT_LOCALS_H
#define GAME_SCRIPT_LOCALS_H

#include <vector>

#include <components/esm/loadscpt.hpp>
#include <components/interpreter/types.hpp>

namespace MWScript
{
    struct Locals
    {
        std::vector<Interpreter::Type_Short> mShorts;
        std::vector<Interpreter::Type_Integer> mLongs;
        std::vector<Interpreter::Type_Float> mFloats;
        
        void configure (const ESM::Script& script)
        {
            mShorts.clear();
            mShorts.resize (script.data.numShorts, 0);
            mLongs.clear();
            mLongs.resize (script.data.numLongs, 0);
            mFloats.clear();
            mFloats.resize (script.data.numFloats, 0);
        }
    };
}

#endif

