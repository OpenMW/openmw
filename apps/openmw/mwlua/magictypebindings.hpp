#ifndef MWLUA_MAGICTYPEBINDINGS_H
#define MWLUA_MAGICTYPEBINDINGS_H

#include <sol/forward.hpp>

namespace ESM
{
    struct EffectList;
    struct Enchantment;
    struct Potion;
    struct Spell;
}

namespace MWLua
{
    void addSpellBindings(sol::state_view&);
    void addMutableSpellType(sol::state_view& lua);
    ESM::Spell tableToSpell(const sol::table&);
    void addEnchantmentBindings(sol::state_view&);
    void addMutableEnchantmentType(sol::state_view& lua);
    ESM::Enchantment tableToEnchantment(const sol::table&);
    void addEffectParamsBindings(sol::state_view&);

    void addPotionType(sol::state_view& lua);
    void addMutablePotionType(sol::state_view& lua);

    ESM::EffectList tableToEffectList(const sol::table&);
}

#endif // MWLUA_MAGICTYPEBINDINGS_H