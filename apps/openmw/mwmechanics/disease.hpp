#ifndef OPENMW_MECHANICS_DISEASE_H
#define OPENMW_MECHANICS_DISEASE_H

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwmechanics/spells.hpp"
#include "../mwmechanics/spellcasting.hpp"
#include "../mwmechanics/creaturestats.hpp"

namespace MWMechanics
{

    /// Call when \a actor has got in contact with \a carrier (e.g. hit by him, or loots him)
    /// @param actor The actor that will potentially catch diseases. Currently only the player can catch diseases.
    /// @param carrier The disease carrier.
    inline void diseaseContact (MWWorld::Ptr actor, MWWorld::Ptr carrier)
    {
        if (!carrier.getClass().isActor() || actor != MWBase::Environment::get().getWorld()->getPlayerPtr())
            return;

        float fDiseaseXferChance =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(
                    "fDiseaseXferChance")->getFloat();

        MagicEffects& actorEffects = actor.getClass().getCreatureStats(actor).getMagicEffects();

        Spells& spells = carrier.getClass().getCreatureStats(carrier).getSpells();
        for (Spells::TIterator it = spells.begin(); it != spells.end(); ++it)
        {
            const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(it->first);

            if (actor.getClass().getCreatureStats(actor).getSpells().hasSpell(spell->mId))
                continue;

            float resist = 0.f;
            if (spells.hasCorprusEffect(spell))
                resist = 1.f - 0.01 * getEffectResistanceAttribute(ESM::MagicEffect::Corprus,&actorEffects);
            else if (spell->mData.mType == ESM::Spell::ST_Disease)
                resist = 1.f - 0.01 * actorEffects.getCommonDiseaseResistance();
            else if (spell->mData.mType == ESM::Spell::ST_Blight)
                resist = 1.f - 0.01 * actorEffects.getBlightDiseaseResistance();
            else
                continue;

            int x = fDiseaseXferChance * 100 * resist;
            float roll = std::rand()/ (static_cast<double> (RAND_MAX) + 1) * 10000; // [0, 9999]

            if (roll < x)
            {
                // Contracted disease!
                actor.getClass().getCreatureStats(actor).getSpells().add(it->first);

                std::string msg = "sMagicContractDisease";
                msg = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(msg)->getString();
                if (msg.find("%s") != std::string::npos)
                    msg.replace(msg.find("%s"), 2, spell->mName);
                MWBase::Environment::get().getWindowManager()->messageBox(msg);
            }
        }
    }
}


#endif
