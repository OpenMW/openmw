#pragma once

#include "sol.hpp"

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

        sol::object makeLuaObject(ESM::MagicEffect* object);
        sol::object makeLuaObject(ESM::WeaponType* object);
        sol::object makeLuaObject(MWWorld::Ptr object);

        MWWorld::Ptr getOptionalParamReference(sol::optional<sol::table> maybeParams, const char* key);
	}
}
