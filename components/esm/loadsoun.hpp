#ifndef _ESM_SOUN_H
#define _ESM_SOUN_H

#include "record.hpp"
#include "esm_reader.hpp"
#include "esm_writer.hpp"

namespace ESM
{

struct SOUNstruct
{
    unsigned char volume, minRange, maxRange;
};

struct Sound : public Record
{
    SOUNstruct data;
    std::string sound;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);

    int getName() { return REC_SOUN; }
};
}
#endif
