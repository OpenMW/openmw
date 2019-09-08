#pragma once

#include "sol.hpp"
#include "luamanager.hpp"

#include <unordered_map>

#include "../mwworld/ptr.hpp"

#include <components/esm/loadmgef.hpp>
#include <components/esm/loadweap.hpp>

namespace mwse {
	namespace lua {
		template <typename T>
		T getOptionalParam(sol::optional<sol::table> maybeParams, const char* key, T defaultValue) {
			T value = defaultValue;

			if (maybeParams) {
				sol::table params = maybeParams.value();
				sol::object maybeValue = params[key];
				if (maybeValue.valid() && maybeValue.is<T>()) {
					value = maybeValue.as<T>();
				}
			}

			return value;
		}

		// Dumps the current stacktrace to the log.
		void logStackTrace(const char* message = nullptr);

		template<typename T>
		sol::object makeLuaObject(T* object)
        {
			if (object == nullptr)
            {
				return sol::nil;
			}

			LuaManager& luaManager = LuaManager::getInstance();
			auto stateHandle = luaManager.getThreadSafeStateHandle();

            /*
			// Search in cache first.
			sol::object result = stateHandle.getCachedUserdata(object);
			if (result != sol::nil) {
				return result;
			}
			*/

			sol::state& state = stateHandle.state;

			sol::object result = sol::make_object(state, object);

            /*
			// Insert the object into cache.
			if (result != sol::nil) {
				stateHandle.insertUserdataIntoCache(object, result);
			}
			*/

			return result;
		}

        sol::object makeLuaObject(MWWorld::Ptr object);

        MWWorld::Ptr getOptionalParamReference(sol::optional<sol::table> maybeParams, const char* key);
	}
}
