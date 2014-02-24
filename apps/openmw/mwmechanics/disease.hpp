#ifndef OPENMW_MECHANICS_DISEASE_H
#define OPENMW_MECHANICS_DISEASE_H

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwmechanics/spells.hpp"
#include "../mwmechanics/creaturestats.hpp"

namespace MWMechanics
{

    /// Call when \a actor has got in contact with \a carrier (e.g. hit by him, or loots him)
    inline void diseaseContact (MWWorld::Ptr actor, MWWorld::Ptr carrier)
    {
        if (!carrier.getClass().isActor())
            return;

        float fDiseaseXferChance =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(
                    "fDiseaseXferChance")->getFloat();

        Spells& spells = carrier.getClass().getCreatureStats(carrier).getSpells();
        for (Spells::TIterator it = spells.begin(); it != spells.end(); ++it)
        {
            const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(it->first);
            if (spell->mData.mType == ESM::Spell::ST_Disease
                    || spell->mData.mType == ESM::Spell::ST_Blight)
            {
                float roll = std::rand()/ (static_cast<double> (RAND_MAX) + 1) * 100; // [0, 99]
                if (roll < fDiseaseXferChance)
                {
                    // Contracted disease!
                    actor.getClass().getCreatureStats(actor).getSpells().add(it->first);

                    if (actor.getRefData().getHandle() == "player")
                    {
                        std::string msg = "sMagicContractDisease";
                        msg = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(msg)->getString();
                        if (msg.find("%s") != std::string::npos)
                            msg.replace(msg.find("%s"), 2, spell->mName);
                        MWBase::Environment::get().getWindowManager()->messageBox(msg);
                    }
                }
            }
        }
    }
}


#endif
