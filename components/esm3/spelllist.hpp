#ifndef OPENMW_ESM_SPELLLIST_H
#define OPENMW_ESM_SPELLLIST_H

#include <components/esm/refid.hpp>
#include <string>
#include <vector>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    /** A list of references to spells and spell effects. This is shared
     between the records BSGN, NPC and RACE.
     NPCS subrecord.
     */
    struct SpellList
    {
        std::vector<ESM::RefId> mList;

        /// Is this spell ID in mList?
        bool exists(const ESM::RefId& spell) const;

        /// Load one spell, assumes the subrecord name was already read
        void add(ESMReader& esm);

        void save(ESMWriter& esm) const;
    };
}

#endif
