#ifndef OPENMW_ESM_DIAL_H
#define OPENMW_ESM_DIAL_H

#include <string>
#include <vector>

#include "loadinfo.hpp"

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 * Dialogue topic and journal entries. The actual data is contained in
 * the INFO records following the DIAL.
 */

struct Dialogue
{
    enum Type
    {
        Topic = 0,
        Voice = 1,
        Greeting = 2,
        Persuasion = 3,
        Journal = 4,
        Deleted = -1
    };

    std::string mId;
    char mType;
    std::vector<DialInfo> mInfo;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
};
}
#endif
