#ifndef GAME_MWWORLD_REFDATA_H
#define GAME_MWWORLD_REFDATA_H

#include <string>

#include "../mwscript/locals.hpp"

namespace ESM
{
    class Script;
}

namespace MWWorld
{
    class RefData
    {
            std::string mHandle;
            
            MWScript::Locals mLocals; // if we find the overhead of heaving a locals
                                      // object in the refdata of refs without a script,
                                      // we can make this a pointer later.
            bool mHasLocals;
            bool mEnabled;
        
        public:
        
            RefData() : mHasLocals (false), mEnabled (true) {}
                         
            std::string getHandle()
            {
                return mHandle;
            }
                        
            void setLocals (const ESM::Script& script)
            {
                if (!mHasLocals)
                {
                    mLocals.configure (script);
                    mHasLocals = true;
                }
            }
            
            void setHandle (const std::string& handle)
            {
                mHandle = handle;
            }
            
            MWScript::Locals& getLocals()
            {
                return mLocals;
            }
            
            bool isEnabled() const
            {
                return mEnabled;
            }
            
            void enable()
            {
                mEnabled = true;
            }
            
            void disable()
            {
                mEnabled = true;
            }
    };        
}

#endif
