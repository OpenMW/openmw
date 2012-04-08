#ifndef _ESM_DOOR_H
#define _ESM_DOOR_H

#include "record.hpp"
#include "esm_reader.hpp"
#include "esm_writer.hpp"

namespace ESM
{

struct Door : public Record
{
    std::string name, model, script, openSound, closeSound;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);

    int getName() { return REC_DOOR; }
};
}
#endif
