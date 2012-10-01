#ifndef OPENMW_ESM_WEAP_H
#define OPENMW_ESM_WEAP_H

#include <string>

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 * Weapon definition
 */

struct Weapon
{
    enum Type
    {
        ShortBladeOneHand = 0,
        LongBladeOneHand = 1,
        LongBladeTwoHand = 2,
        BluntOneHand = 3,
        BluntTwoClose = 4,
        BluntTwoWide = 5,
        SpearTwoWide = 6,
        AxeOneHand = 7,
        AxeTwoHand = 8,
        MarksmanBow = 9,
        MarksmanCrossbow = 10,
        MarksmanThrown = 11,
        Arrow = 12,
        Bolt = 13
    };

    enum Flags
    {
        Magical = 0x01,
        Silver = 0x02
    };

#pragma pack(push)
#pragma pack(1)
    struct WPDTstruct
    {
        float mWeight;
        int mValue;
        short mType;
        short mHealth;
        float mSpeed, mReach;
        short mEnchant; // Enchantment points
        unsigned char mChop[2], mSlash[2], mThrust[2]; // Min and max
        int mFlags;
    }; // 32 bytes
#pragma pack(pop)

    WPDTstruct mData;

    std::string mName, mModel, mIcon, mEnchant, mScript;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
};
}
#endif
