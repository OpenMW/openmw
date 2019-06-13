#ifndef OPENMW_MECHANICS_DISEASE_H
#define OPENMW_MECHANICS_DISEASE_H

#include <components/misc/rng.hpp>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "spells.hpp"
#include "creaturestats.hpp"
#include "actorutil.hpp"

namespace MWMechanics
{

    /// Call when \a actor has got in contact with \a carrier (e.g. hit by him, or loots him)
    /// @param actor The actor that will potentially catch diseases. Currently only the player can catch diseases.
    /// @param carrier The disease carrier.
    inline void diseaseContact (MWWorld::Ptr actor, MWWorld::Ptr carrier)
    {
        if (!carrier.getClass().isActor() || actor != getPlayer())
            return;

        float fDiseaseXferChance =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(
                    "fDiseaseXferChance")->mValue.getFloat();

        MagicEffects& actorEffects = actor.getClass().getCreatureStats(actor).getMagicEffects();

        Spells& spells = carrier.getClass().getCreatureStats(carrier).getSpells();
        for (Spells::TIterator it = spells.begin(); it != spells.end(); ++it)
        {
            const ESM::Spell* spell = it->first;
            if (actor.getClass().getCreatureStats(actor).getSpells().hasSpell(spell->mId))
                continue;

            float resist = 0.f;
            if (spells.hasCorprusEffect(spell))
                resist = 1.f - 0.01f * (actorEffects.get(ESM::MagicEffect::ResistCorprusDisease).getMagnitude()
                                        - actorEffects.get(ESM::MagicEffect::WeaknessToCorprusDisease).getMagnitude());
            else if (spell->mData.mType == ESM::Spell::ST_Disease)
                resist = 1.f - 0.01f * (actorEffects.get(ESM::MagicEffect::ResistCommonDisease).getMagnitude()
                                        - actorEffects.get(ESM::MagicEffect::WeaknessToCommonDisease).getMagnitude());
            else if (spell->mData.mType == ESM::Spell::ST_Blight)
                resist = 1.f - 0.01f * (actorEffects.get(ESM::MagicEffect::ResistBlightDisease).getMagnitude()
                                        - actorEffects.get(ESM::MagicEffect::WeaknessToBlightDisease).getMagnitude());
            else
                continue;

            int x = static_cast<int>(fDiseaseXferChance * 100 * resist);
            if (Misc::Rng::rollDice(10000) < x)
            {
                // Contracted disease!
                actor.getClass().getCreatureStats(actor).getSpells().add(it->first);
                MWBase::Environment::get().getWorld()->applyLoopingParticles(actor);

                std::string msg = "sMagicContractDisease";
                msg = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(msg)->mValue.getString();
                msg = Misc::StringUtils::format(msg, spell->mName);
                MWBase::Environment::get().getWindowManager()->messageBox(msg);
            }
        }
    }
}


#endif
