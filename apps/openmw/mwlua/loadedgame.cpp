#include "loadedgame.hpp"

#include "luamanager.hpp"
#include "luautil.hpp"

//#include "TES3MobilePlayer.h"
//#include "TES3Reference.h"
//#include "TES3WorldController.h"

namespace mwse {
	namespace lua {
		namespace event {
			LoadedGameEvent::LoadedGameEvent(const char* fileName, bool quickLoad, bool newGame) :
				GenericEvent("loaded"),
				m_FileName(fileName),
				m_QuickLoad(quickLoad),
				m_NewGame(newGame)
			{
				if (m_FileName == nullptr && !m_NewGame) {
					m_QuickLoad = true;
					m_FileName = "quiсksave.omwsave";
				}
			}

			sol::table LoadedGameEvent::createEventTable() {
				auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
				sol::state& state = stateHandle.state;
				sol::table eventData = state.create_table();

				if (m_NewGame) {
					eventData["newGame"] = true;
					eventData["quickload"] = false;
				}
				else {
					std::string filename = m_FileName;
					eventData["filename"] = filename.substr(0, filename.find_last_of('.'));;
					eventData["quickload"] = m_QuickLoad;
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

			sol::object LoadedGameEvent::getEventOptions() {
				auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
				sol::state& state = stateHandle.state;
				sol::table options = state.create_table();

				options["filter"] = m_FileName;

				return options;
			}

			//bool LoadedGameEvent::m_EventEnabled = false;
		}
	}
}
