#include "locals.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/scriptmanager.hpp"
#include <components/compiler/locals.hpp>

namespace MWScript
{
    void Locals::configure (const ESM::Script& script)
    {
        mShorts.clear();
        mShorts.resize (script.mData.mNumShorts, 0);
        mLongs.clear();
        mLongs.resize (script.mData.mNumLongs, 0);
        mFloats.clear();
        mFloats.resize (script.mData.mNumFloats, 0);
    }
        
    bool Locals::setVarByInt(const std::string& script, const std::string& var, int val)
    {    
        Compiler::Locals locals = MWBase::Environment::get().getScriptManager()->getLocals(script);
        int index = locals.getIndex(var);
        char type = locals.getType(var);
        if(index != -1)
        {
            switch(type)
            {
                case 's':
                    mShorts.at (index) = val; break;
                
                case 'l':
                    mLongs.at (index) = val; break;
                
                case 'f':
                    mFloats.at (index) = val; break;
            }
            return true;
        }
        return false;
    }
}
