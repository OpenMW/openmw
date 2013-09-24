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
    static unsigned int sRecordId;

    SOUNstruct mData;
    std::string mId, mSound;

    void load(ESMReader &esm);
    void save(ESMWriter &esm) const;

    void blank();
    ///< Set record to default state (does not touch the ID/index).
};
}
#endif
