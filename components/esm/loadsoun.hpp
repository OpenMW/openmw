#ifndef OPENMW_ESM_SOUN_H
#define OPENMW_ESM_SOUN_H

#include <string>

#include "record.hpp"

namespace ESM
{

struct SOUNstruct
{
    unsigned char mVolume, mMinRange, mMaxRange;
};

struct Sound : public Record
{
    SOUNstruct mData;
    std::string mSound;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);

    int getName() { return REC_SOUN; }
};
}
#endif
