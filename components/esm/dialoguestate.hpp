#ifndef OPENMW_ESM_DIALOGUESTATE_H
#define OPENMW_ESM_DIALOGUESTATE_H

#include <string>
#include <vector>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only

    struct DialogueState
    {
        std::vector<std::string> mKnownTopics;

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };
}

#endif