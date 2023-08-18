#include "musicbindings.hpp"
#include "luabindings.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "context.hpp"
#include "luamanagerimp.hpp"

namespace MWLua
{
    sol::table initMusicPackage(const Context& context)
    {
        sol::table api(context.mLua->sol(), sol::create);
        api["streamMusic"] = [](std::string_view fileName) {
            MWBase::Environment::get().getSoundManager()->streamMusic(std::string(fileName));
        };

        api["stopMusic"] = []() { MWBase::Environment::get().getSoundManager()->stopMusic(); };

        return LuaUtil::makeReadOnly(api);
    }
}
