#ifndef MWLUA_MAGICTYPEBINDINGS_H
#define MWLUA_MAGICTYPEBINDINGS_H

#include <sol/forward.hpp>

namespace ESM
{
    struct Potion;
}

namespace MWLua
{
    void addSpellBindings(sol::state_view&);
    void addEnchantmentBindings(sol::state_view&);
    void addEffectParamsBindings(sol::state_view&);

    void addPotionType(sol::state_view& lua);
    void addMutablePotionType(sol::state_view& lua);
}

#endif // MWLUA_MAGICTYPEBINDINGS_H