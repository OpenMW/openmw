#ifndef OPENMW_ESM_ACTI_H
#define OPENMW_ESM_ACTI_H

#include <string>
#include "components/esm/defs.hpp"

namespace ESM
{

class ESMReader;
class ESMWriter;

struct Activator
{
    constexpr static RecNameInts sRecordId = REC_ACTI;

    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string_view getRecordType() { return "Activator"; }

    unsigned int mRecordFlags;
    std::string mId, mName, mScript, mModel;

    void load(ESMReader &esm, bool &isDeleted);
    void save(ESMWriter &esm, bool isDeleted = false) const;

    void blank();
    ///< Set record to default state (does not touch the ID).
};

}
#endif
