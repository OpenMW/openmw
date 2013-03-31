#ifndef OPENMW_ESM_REPA_H
#define OPENMW_ESM_REPA_H

#include <string>

namespace ESM
{

class ESMReader;
class ESMWriter;

struct Repair
{
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
    void save(ESMWriter &esm);
};

}
#endif
