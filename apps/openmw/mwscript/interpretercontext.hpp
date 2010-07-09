#ifndef GAME_SCRIPT_INTERPRETERCONTEXT_H
#define GAME_SCRIPT_INTERPRETERCONTEXT_H

#include <components/interpreter/context.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"

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
        
            typedef std::pair<MWWorld::Ptr, MWWorld::World::CellStore *> PtrWithCell;
            typedef std::pair<const MWWorld::Ptr, const MWWorld::World::CellStore *> CPtrWithCell;
        
            PtrWithCell getReference (const std::string& id, bool activeOnly);
            
            CPtrWithCell getReference (const std::string& id, bool activeOnly) const;

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
            
            virtual bool isScriptRunning (const std::string& name) const;
            
            virtual void startScript (const std::string& name);
            
            virtual void stopScript (const std::string& name);
                        
            virtual float getDistance (const std::string& name, const std::string& id = "") const;
                        
            virtual bool hasBeenActivated() const;
            
            virtual float getSecondsPassed() const;
                               
            virtual bool isDisabled (const std::string& id = "") const;
            
            virtual void enable (const std::string& id = "");
            
            virtual void disable (const std::string& id = "");
                                           
            MWWorld::World& getWorld();
            
            MWSound::SoundManager& getSoundManager();

            MWGui::GuiManager& getGuiManager();
            
            MWWorld::Ptr getReference();
            ///< Reference, that the script is running from (can be empty)
    };
}

#endif


