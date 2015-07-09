#ifndef OPENMW_ESM_APPA_H
#define OPENMW_ESM_APPA_H

#include <string>

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 * Alchemist apparatus
 */

struct Apparatus
{
    static unsigned int sRecordId;
    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string getRecordType() { return "Apparatus"; }

    enum AppaType
    {
        MortarPestle = 0,
        Albemic = 1,
        Calcinator = 2,
        Retort = 3
    };

    struct AADTstruct
    {
        int mType;
        float mQuality;
        float mWeight;
        int mValue;
    };

    AADTstruct mData;
    std::string mId, mModel, mIcon, mScript, mName;

    bool mIsDeleted;

    Apparatus();

    void load(ESMReader &esm);
    void save(ESMWriter &esm) const;

    void blank();
    ///< Set record to default state (does not touch the ID).
};
}
#endif
