#include "loadedgame.hpp"

#include "luamanager.hpp"
#include "luautil.hpp"

//#include "TES3MobilePlayer.h"
//#include "TES3Reference.h"
//#include "TES3WorldController.h"

namespace MWLua
{
    namespace Event
    {
        LoadedGameEvent::LoadedGameEvent(const char* fileName, bool quickLoad, bool newGame) :
            GenericEvent("loaded"),
            mFileName(fileName),
            mQuickLoad(quickLoad),
            mNewGame(newGame)
        {
            if (mFileName == nullptr && !mNewGame)
            {
                mQuickLoad = true;
                mFileName = "quiсksave.omwsave";
            }
        }

        sol::table LoadedGameEvent::createEventTable()
        {
            auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
            sol::state& state = stateHandle.state;
            sol::table eventData = state.create_table();

            if (mNewGame)
            {
                eventData["newGame"] = true;
                eventData["quickload"] = false;
            }
            else
            {
                std::string filename = mFileName;
                eventData["filename"] = filename.substr(0, filename.find_last_of('.'));;
                eventData["quickload"] = mQuickLoad;
                eventData["newGame"] = false;
            }

            // FIXME: reference to player
            /*
            auto mobilePlayer = TES3::WorldController::get()->getMobilePlayer();
            if (mobilePlayer) {
                eventData["mobile"] = makeLuaObject(mobilePlayer);
                eventData["reference"] = makeLuaObject(mobilePlayer->reference);
            }
            */

            return eventData;
        }

        sol::object LoadedGameEvent::getEventOptions()
        {
            auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
            sol::state& state = stateHandle.state;
            sol::table options = state.create_table();

            options["filter"] = mFileName;

            return options;
        }

        //bool LoadedGameEvent::m_EventEnabled = false;
    }
}
