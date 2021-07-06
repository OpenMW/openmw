#ifndef OPENMW_ESM_DOOR_H
#define OPENMW_ESM_DOOR_H

#include <string>

namespace ESM
{

class ESMReader;
class ESMWriter;

struct Door
{
    static unsigned int sRecordId;
    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string getRecordType() { return "Door"; }

    unsigned int mRecordFlags;
    std::string mId, mName, mModel, mScript, mOpenSound, mCloseSound;

    void load(ESMReader &esm, bool &isDeleted);
    void save(ESMWriter &esm, bool isDeleted = false) const;

    void blank();
    ///< Set record to default state (does not touch the ID).
};
}
#endif
