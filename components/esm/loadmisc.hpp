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
    struct MCDTstruct
    {
        float mWeight;
        int mValue;
        int mIsKey; // There are many keys in Morrowind.esm that has this
                   // set to 0. TODO: Check what this field corresponds to
                   // in the editor.
    };
    MCDTstruct mData;

    std::string mName, mModel, mIcon, mScript;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
};
}
#endif
