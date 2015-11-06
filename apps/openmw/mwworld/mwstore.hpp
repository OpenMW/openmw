#ifndef GAME_MWWORLD_MWSTORE_H
#define GAME_MWWORLD_MWSTORE_H

#include <string>

#include <components/autocalc/store.hpp>

#include "store.hpp"

namespace MWWorld
{
    class MWStore : public AutoCalc::StoreCommon
    {
        const MWWorld::Store<ESM::GameSetting>& mGmst;
        const MWWorld::Store<ESM::Spell> &mSpells;

    public:

        MWStore();
        ~MWStore();

        virtual int findGmstInt(const std::string& name) const;

        virtual float findGmstFloat(const std::string& name) const;

        virtual const ESM::Skill *findSkill(int index) const;

        virtual const ESM::MagicEffect* findMagicEffect(int id) const;

        virtual void getSpells(std::vector<ESM::Spell*>& spells);
    };
}

#endif // GAME_MWWORLD_MWSTORE_H
