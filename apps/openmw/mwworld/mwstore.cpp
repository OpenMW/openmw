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

    void MWStore::getSpells(std::vector<ESM::Spell*>& spells)
    {
        for (Store<ESM::Spell>::iterator iter = mSpells.begin(); iter != mSpells.end(); ++iter)
                 spells.push_back(const_cast<ESM::Spell*>(&*iter));
    }
}
