#ifndef MWLUA_SOUNDBINDINGS_H
#define MWLUA_SOUNDBINDINGS_H

#include <sol/forward.hpp>

namespace ESM
{
    struct Sound;
}

namespace MWLua
{
    struct Context;

    sol::table initCoreSoundBindings(const Context& context);

    sol::table initAmbientPackage(const Context& context);

    void addMutableSoundType(sol::state_view& lua);

    ESM::Sound tableToSound(const sol::table&);
}

#endif // MWLUA_SOUNDBINDINGS_H
