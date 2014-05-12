#ifndef OPENMW_ESM_SPELLSTATE_H
#define OPENMW_ESM_SPELLSTATE_H

#include <map>
#include <string>

#include "defs.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    struct SpellState
    {
        typedef std::map<std::string, std::map<const int, float> > TContainer;
        TContainer mSpells;

        std::map<std::string, TimeStamp> mUsedPowers;

        std::string mSelectedSpell;

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };

}

#endif
