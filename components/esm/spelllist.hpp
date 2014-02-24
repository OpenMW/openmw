#ifndef OPENMW_ESM_SPELLLIST_H
#define OPENMW_ESM_SPELLLIST_H

#include <vector>
#include <string>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    /** A list of references to spells and spell effects. This is shared
     between the records BSGN, NPC and RACE.
     */
    struct SpellList
    {
        std::vector<std::string> mList;

        void load(ESMReader &esm);
        void save(ESMWriter &esm) const;
    };
}

#endif

