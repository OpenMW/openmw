#ifndef OPENMW_ESM_PROBE_H
#define OPENMW_ESM_PROBE_H

#include <string>

namespace ESM
{

class ESMReader;
class ESMWriter;

struct Probe
{
    static unsigned int sRecordId;
    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string getRecordType() { return "Probe"; }

    struct Data
    {
        float mWeight;
        int mValue;

        float mQuality;
        int mUses;
    }; // Size = 16

    Data mData;
    std::string mId, mName, mModel, mIcon, mScript;

    void load(ESMReader &esm, bool &isDeleted);
    void save(ESMWriter &esm, bool isDeleted = false) const;

    void blank();
    ///< Set record to default state (does not touch the ID).
};

}
#endif
