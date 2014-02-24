#ifndef OPENMW_ESM_REPA_H
#define OPENMW_ESM_REPA_H

#include <string>

namespace ESM
{

class ESMReader;
class ESMWriter;

struct Repair
{
    static unsigned int sRecordId;

    struct Data
    {
        float mWeight;
        int mValue;

        int mUses;
        float mQuality;
    }; // Size = 16

    Data mData;
    std::string mId, mName, mModel, mIcon, mScript;

    void load(ESMReader &esm);
    void save(ESMWriter &esm) const;

    void blank();
    ///< Set record to default state (does not touch the ID).
};

}
#endif
