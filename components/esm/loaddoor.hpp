#ifndef _ESM_DOOR_H
#define _ESM_DOOR_H

#include "esm_reader.hpp"
#include "esm_writer.hpp"

namespace ESM
{

struct Door
{
    std::string name, model, script, openSound, closeSound;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
};
}
#endif
