#ifndef OPENMW_ESM_LTEX_H
#define OPENMW_ESM_LTEX_H

#include <string>

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 * Texture used for texturing landscape.
 *
 * They are probably indexed by 'num', not 'id', but I don't know for
 * sure. And num is not unique between files, so one option is to keep
 * a separate list for each input file (that has LTEX records, of
 * course.) We also need to resolve references to already existing
 * land textures to save space.

 * I'm not sure if it is even possible to override existing land
 * textures, probably not. I'll have to try it, and have to mimic the
 * behaviour of morrowind. First, check what you are allowed to do in
 * the editor. Then make an esp which changes a commonly used land
 * texture, and see if it affects the game.
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
