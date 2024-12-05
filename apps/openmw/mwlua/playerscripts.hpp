#ifndef MWLUA_PLAYERSCRIPTS_H
#define MWLUA_PLAYERSCRIPTS_H

#include <SDL_events.h>

#include <components/sdlutil/events.hpp>

#include "../mwbase/luamanager.hpp"

#include "inputprocessor.hpp"
#include "localscripts.hpp"

namespace MWLua
{

    class PlayerScripts : public LocalScripts
    {
    public:
        PlayerScripts(LuaUtil::LuaState* lua, const LObject& obj)
            : LocalScripts(lua, obj)
            , mInputProcessor(this)
        {
            registerEngineHandlers({ &mConsoleCommandHandlers, &mOnFrameHandlers, &mQuestUpdate, &mUiModeChanged });
        }

        void processInputEvent(const MWBase::LuaManager::InputEvent& event)
        {
            mInputProcessor.processInputEvent(event);
        }

        void onFrame(float dt) { callEngineHandlers(mOnFrameHandlers, dt); }
        void onQuestUpdate(std::string_view questId, int stage) { callEngineHandlers(mQuestUpdate, questId, stage); }

        bool consoleCommand(
            const std::string& consoleMode, const std::string& command, const sol::object& selectedObject)
        {
            callEngineHandlers(mConsoleCommandHandlers, consoleMode, command, selectedObject);
            return !mConsoleCommandHandlers.mList.empty();
        }

        // `arg` is either forwarded from MWGui::pushGuiMode or empty
        void uiModeChanged(ObjectId arg, bool byLuaAction)
        {
            if (arg.isZeroOrUnset())
                callEngineHandlers(mUiModeChanged, byLuaAction);
            else
                callEngineHandlers(mUiModeChanged, byLuaAction, LObject(arg));
        }

    private:
        friend class MWLua::InputProcessor<PlayerScripts>;
        InputProcessor<PlayerScripts> mInputProcessor;
        EngineHandlerList mConsoleCommandHandlers{ "onConsoleCommand" };
        EngineHandlerList mOnFrameHandlers{ "onFrame" };
        EngineHandlerList mQuestUpdate{ "onQuestUpdate" };
        EngineHandlerList mUiModeChanged{ "_onUiModeChanged" };
    };

}

#endif // MWLUA_PLAYERSCRIPTS_H
