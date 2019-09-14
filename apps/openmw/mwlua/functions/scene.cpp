#include "world.hpp"

#include <osg/Group>

#include "../luamanager.hpp"
#include "../util.hpp"

#include "../../mwbase/environment.hpp"
#include "../../mwbase/world.hpp"

#include "../../mwworld/class.hpp"

#include <components/resource/scenemanager.hpp>

namespace MWLua
{
    void bindTES3SceneFunctions()
    {
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        state["omw"]["loadMesh"] = [](const char* model) -> sol::object
        {
            return makeLuaNiPointer(MWBase::Environment::get().getWorld()->getResourceSystem()->getSceneManager()->getInstance(model).get()->asGroup());
        };
    }
}
