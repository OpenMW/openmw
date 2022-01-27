#ifndef OPENMW_ESSIMPORT_CONVERTACDT_H
#define OPENMW_ESSIMPORT_CONVERTACDT_H

#include <components/esm3/creaturestats.hpp>
#include <components/esm3/npcstats.hpp>
#include <components/esm3/loadskil.hpp>
#include <components/esm3/animationstate.hpp>

#include "importacdt.hpp"

namespace ESSImport
{

    // OpenMW uses Health,Magicka,Fatigue, MW uses Health,Fatigue,Magicka
    int translateDynamicIndex(int mwIndex);


    void convertACDT (const ACDT& acdt, ESM::CreatureStats& cStats);
    void convertACSC (const ACSC& acsc, ESM::CreatureStats& cStats);

    void convertNpcData (const ActorData& actorData, ESM::NpcStats& npcStats);

    void convertANIS (const ANIS& anis, ESM::AnimationState& state);
}

#endif
