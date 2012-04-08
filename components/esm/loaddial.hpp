#ifndef _ESM_DIAL_H
#define _ESM_DIAL_H

#include <vector>

#include "record.hpp"
#include "esm_reader.hpp"
#include "esm_writer.hpp"
#include "loadinfo.hpp"

namespace ESM
{

/*
 * Dialogue topic and journal entries. The actual data is contained in
 * the INFO records following the DIAL.
 */

struct Dialogue : public Record
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

    char type;
    std::vector<DialInfo> mInfo;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);

    int getName() { return REC_DIAL; }
};
}
#endif
