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
    std::string mModel, mIcon, mScript, mName;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
};
}
#endif
