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

    std::string mId, mName, mModel, mScript, mOpenSound, mCloseSound;

    bool mIsDeleted;

    void load(ESMReader &esm);
    void save(ESMWriter &esm) const;

    void blank();
    ///< Set record to default state (does not touch the ID).
};
}
#endif
