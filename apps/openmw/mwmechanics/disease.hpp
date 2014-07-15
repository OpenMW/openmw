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
    /// @param actor The actor that will potentially catch diseases. Currently only the player can catch diseases.
    /// @param carrier The disease carrier.
    inline void diseaseContact (MWWorld::Ptr actor, MWWorld::Ptr carrier)
    {
        if (!carrier.getClass().isActor() || actor != MWBase::Environment::get().getWorld()->getPlayerPtr())
            return;

        float fDiseaseXferChance =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(
                    "fDiseaseXferChance")->getFloat();

        Spells& spells = carrier.getClass().getCreatureStats(carrier).getSpells();
        for (Spells::TIterator it = spells.begin(); it != spells.end(); ++it)
        {
            const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(it->first);

            if (actor.getClass().getCreatureStats(actor).getSpells().hasSpell(spell->mId))
                continue;

            bool hasCorprusEffect = false;
            for (std::vector<ESM::ENAMstruct>::const_iterator effectIt = spell->mEffects.mList.begin(); effectIt != spell->mEffects.mList.end(); ++effectIt)
            {
                if (effectIt->mEffectID == ESM::MagicEffect::Corprus)
                {
                    hasCorprusEffect = true;
                    break;
                }
            }

            float resist = 0.f;
            if (hasCorprusEffect)
                resist = 1.f - 0.01 * (actor.getClass().getCreatureStats(actor).getMagicEffects().get(ESM::MagicEffect::ResistCorprusDisease).mMagnitude
                                        - actor.getClass().getCreatureStats(actor).getMagicEffects().get(ESM::MagicEffect::WeaknessToCorprusDisease).mMagnitude);
            else if (spell->mData.mType == ESM::Spell::ST_Disease)
                resist = 1.f - 0.01 * (actor.getClass().getCreatureStats(actor).getMagicEffects().get(ESM::MagicEffect::ResistCommonDisease).mMagnitude
                                        - actor.getClass().getCreatureStats(actor).getMagicEffects().get(ESM::MagicEffect::WeaknessToCommonDisease).mMagnitude);
            else if (spell->mData.mType == ESM::Spell::ST_Blight)
                resist = 1.f - 0.01 * (actor.getClass().getCreatureStats(actor).getMagicEffects().get(ESM::MagicEffect::ResistBlightDisease).mMagnitude
                                        - actor.getClass().getCreatureStats(actor).getMagicEffects().get(ESM::MagicEffect::WeaknessToBlightDisease).mMagnitude);
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
