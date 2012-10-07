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
            mShorts.resize (script.mData.mNumShorts, 0);
            mLongs.clear();
            mLongs.resize (script.mData.mNumLongs, 0);
            mFloats.clear();
            mFloats.resize (script.mData.mNumFloats, 0);
        }
    };
}

#endif

