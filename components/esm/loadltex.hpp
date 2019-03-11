#ifndef OPENMW_ESM_LTEX_H
#define OPENMW_ESM_LTEX_H

#include <string>

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 * Texture used for texturing landscape.
 * They are indexed by 'num', but still use 'id' to override base records.
 * Original editor even does not allow to create new records with existing ID's.
 * TODO: currently OpenMW-CS does not allow to override LTEX records at all.
 */

struct LandTexture
{
    static unsigned int sRecordId;
    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string getRecordType() { return "LandTexture"; }

    // mId is merely a user friendly name for the texture in the editor.
    std::string mId, mTexture;
    int mIndex;

    void load(ESMReader &esm, bool &isDeleted);
    void save(ESMWriter &esm, bool isDeleted = false) const;

    /// Sets the record to the default state. Does not touch the index. Does touch mID.
    void blank();
};
}
#endif
