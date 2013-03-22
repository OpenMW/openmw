#ifndef OPENMW_ESM_PROBE_H
#define OPENMW_ESM_PROBE_H

#include <string>

namespace ESM
{

class ESMReader;
class ESMWriter;

struct Probe
{
    struct Data
    {
        float mWeight;
        int mValue;

        float mQuality;
        int mUses;
    }; // Size = 16

    Data mData;
    std::string mId, mName, mModel, mIcon, mScript;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
};

}
#endif
