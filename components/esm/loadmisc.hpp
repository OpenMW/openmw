#ifndef OPENMW_ESM_MISC_H
#define OPENMW_ESM_MISC_H

#include <string>

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 * Misc inventory items, basically things that have no use but can be
 * carried, bought and sold. It also includes keys.
 */

struct Miscellaneous
{
    static unsigned int sRecordId;
    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string getRecordType() { return "Miscellaneous"; }

    struct MCDTstruct
    {
        float mWeight;
        int mValue;
        int mIsKey; // There are many keys in Morrowind.esm that has this
                   // set to 0. TODO: Check what this field corresponds to
                   // in the editor.
    };
    MCDTstruct mData;

    unsigned int mRecordFlags;
    std::string mId, mName, mModel, mIcon, mScript;

    void load(ESMReader &esm, bool &isDeleted);
    void save(ESMWriter &esm, bool isDeleted = false) const;

    void blank();
    ///< Set record to default state (does not touch the ID).
};
}
#endif
