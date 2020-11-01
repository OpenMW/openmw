#ifndef MWMECHANICS_SPELLABSORPTION_H
#define MWMECHANICS_SPELLABSORPTION_H

#include <string>

namespace MWWorld
{
    class Ptr;
}

namespace MWMechanics
{
    // Try to absorb a spell based on the magnitude of every Spell Absorption effect source on the target.
    bool absorbSpell(const std::string& spellId, const MWWorld::Ptr& caster, const MWWorld::Ptr& target);
}

#endif
