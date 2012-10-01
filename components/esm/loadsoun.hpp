#ifndef OPENMW_ESM_SOUN_H
#define OPENMW_ESM_SOUN_H

#include <string>

namespace ESM
{

class ESMReader;
class ESMWriter;

struct SOUNstruct
{
    unsigned char mVolume, mMinRange, mMaxRange;
};

struct Sound
{
    SOUNstruct mData;
    std::string mSound;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
};
}
#endif
