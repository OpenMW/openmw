#ifndef GAME_SCRIPT_INTERPRETERCONTEXT_H
#define GAME_SCRIPT_INTERPRETERCONTEXT_H

#include <components/interpreter/context.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/environment.hpp"

namespace MWWorld
{
    class World;
}

namespace MWSound
{
    class SoundManager;
}

namespace MWScript
{
    struct Locals;

    class InterpreterContext : public Interpreter::Context
    {   
            MWWorld::Environment& mEnvironment;
            Locals *mLocals;
            MWWorld::Ptr mReference;
        
        public:
        
            InterpreterContext (MWWorld::Environment& environment, 
                MWScript::Locals *locals, MWWorld::Ptr reference);
            ///< The ownership of \a locals is not transferred. 0-pointer allowed.
    
            virtual int getLocalShort (int index) const;

            virtual int getLocalLong (int index) const;

            virtual float getLocalFloat (int index) const;

            virtual void setLocalShort (int index, int value); 

            virtual void setLocalLong (int index, int value);

            virtual void setLocalFloat (int index, float value);
            
            virtual void messageBox (const std::string& message,
                const std::vector<std::string>& buttons);   

            virtual bool menuMode();
            
            virtual int getGlobalShort (const std::string& name) const;

            virtual int getGlobalLong (const std::string& name) const;

            virtual float getGlobalFloat (const std::string& name) const;

            virtual void setGlobalShort (const std::string& name, int value);        

            virtual void setGlobalLong (const std::string& name, int value);        

            virtual void setGlobalFloat (const std::string& name, float value);
            
            virtual bool isScriptRunning (const std::string& name);
            
            virtual void startScript (const std::string& name);
            
            virtual void stopScript (const std::string& name);
                        
            virtual float getDistance (const std::string& name);
                        
            virtual bool hasBeenActivated() const;
            
            virtual float getSecondsPassed() const;
                        
            MWWorld::World& getWorld();
            
            MWSound::SoundManager& getSoundManager();
            
            MWWorld::Ptr getReference();
            ///< Reference, that the script is running from (can be empty)
    };
}

#endif


