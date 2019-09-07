#pragma once

#include "sol.hpp"

#include "dynamicluaobject.hpp"

#include <components/esm/loadscpt.hpp>

#include "../mwworld/ptr.hpp"

namespace mwse {
	namespace lua {
		struct LuaScript : public DynamicLuaObject {
			LuaScript() :
				script(nullptr),
				reference(MWWorld::Ptr()) {
			}
			ESM::Script* script;
			MWWorld::Ptr reference;
		};
	}
}
