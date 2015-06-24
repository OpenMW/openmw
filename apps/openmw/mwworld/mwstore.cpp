#include "mwstore.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "esmstore.hpp"

namespace MWWorld
{
    MWStore::MWStore()
        : mGmst(MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()),
          mSpells(MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>())
    { }

    MWStore::~MWStore()
    { }

    int MWStore::findGmstInt(const std::string& name) const { return mGmst.find(name)->getInt(); }

    float MWStore::findGmstFloat(const std::string& name) const { return mGmst.find(name)->getFloat(); }

    const ESM::Skill *MWStore::findSkill(int index) const
    {
        return MWBase::Environment::get().getWorld()->getStore().get<ESM::Skill>().find(index);
    }

    const ESM::MagicEffect* MWStore::findMagicEffect(int id) const
    {
        return MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(id);
    }

    const GamePlay::CommonStore<ESM::Spell>& MWStore::getSpells() const
    {
        return MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>();
    }
}
