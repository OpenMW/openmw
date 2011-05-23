#ifndef _ESM_SOUN_H
#define _ESM_SOUN_H

#include "esm_reader.hpp"

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
};
}
#endif
