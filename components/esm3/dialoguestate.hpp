#ifndef OPENMW_ESM_DIALOGUESTATE_H
#define OPENMW_ESM_DIALOGUESTATE_H

#include <components/esm/refid.hpp>
#include <map>
#include <string>
#include <vector>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only

    struct DialogueState
    {
        // must be lower case topic IDs
        std::vector<ESM::RefId> mKnownTopics;

        // must be lower case faction IDs
        std::map<ESM::RefId, std::map<ESM::RefId, int>> mChangedFactionReaction;

        void load(ESMReader& esm);
        void save(ESMWriter& esm) const;
    };
}

#endif
