#ifndef _ESM_SOUN_H
#define _ESM_SOUN_H

#include "esm_reader.hpp"
#include "esm_writer.hpp"

namespace ESM
{

struct SOUNstruct
{
    unsigned char volume, minRange, maxRange;
};

struct Sound
{
    SOUNstruct data;
    std::string sound;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
};
}
#endif
