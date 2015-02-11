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

        /// Is this spell ID in mList?
        bool exists(const std::string& spell) const;

        /// Load one spell, assumes the subrecord name was already read
        void add(ESMReader &esm);

        /// Load all spells
        /// TODO: remove this method, the ESM format doesn't guarantee that all spell subrecords follow one another
        void load(ESMReader &esm);
        void save(ESMWriter &esm) const;
    };
}

#endif

