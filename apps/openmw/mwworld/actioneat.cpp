
#include "actioneat.hpp"

#include <cstdlib>

#include <components/esm/loadskil.hpp>

#include <components/esm_store/store.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "class.hpp"

namespace MWWorld
{
    void ActionEat::executeImp (const Ptr& actor)
    {
        // remove used item
        getTarget().getRefData().setCount (getTarget().getRefData().getCount()-1);

        // check for success
        const MWMechanics::CreatureStats& creatureStats = MWWorld::Class::get (actor).getCreatureStats (actor);
        MWMechanics::NpcStats& npcStats = MWWorld::Class::get (actor).getNpcStats (actor);
    
        float x =
            (npcStats.getSkill (ESM::Skill::Alchemy).getModified() +
            0.2 * creatureStats.getAttribute (1).getModified()
            + 0.1 * creatureStats.getAttribute (7).getModified())
            * creatureStats.getFatigueTerm();

        if (x>=100*static_cast<float> (std::rand()) / RAND_MAX)
        {
            // apply to actor
            std::string id = Class::get (getTarget()).getId (getTarget());
            
            Class::get (actor).apply (actor, id, actor);
            // we ignore the result here. Skill increases no matter if the ingredient did something or not.      
        
            // increase skill
            Class::get (actor).skillUsageSucceeded (actor, ESM::Skill::Alchemy, 1);
        }
    }    

    ActionEat::ActionEat (const MWWorld::Ptr& object) : Action (false, object) {}
}
