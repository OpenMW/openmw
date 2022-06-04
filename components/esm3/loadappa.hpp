#ifndef OPENMW_ESM_APPA_H
#define OPENMW_ESM_APPA_H

#include <string>

#include "components/esm/defs.hpp"

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 * Alchemist apparatus
 */

struct Apparatus
{
    constexpr static RecNameInts sRecordId = REC_APPA;

    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string_view getRecordType() { return "Apparatus"; }

    enum AppaType
    {
        MortarPestle = 0,
        Alembic = 1,
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
    unsigned int mRecordFlags;
    std::string mId, mModel, mIcon, mScript, mName;

    void load(ESMReader &esm, bool &isDeleted);
    void save(ESMWriter &esm, bool isDeleted = false) const;

    void blank();
    ///< Set record to default state (does not touch the ID).
};
}
#endif
