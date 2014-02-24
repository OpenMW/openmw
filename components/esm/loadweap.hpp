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
    static unsigned int sRecordId;

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

    enum AttackType
    {
        AT_Chop,
        AT_Slash,
        AT_Thrust
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
        short mEnchant; // Enchantment points. The real value is mEnchant/10.f
        unsigned char mChop[2], mSlash[2], mThrust[2]; // Min and max
        int mFlags;
    }; // 32 bytes
#pragma pack(pop)

    WPDTstruct mData;

    std::string mId, mName, mModel, mIcon, mEnchant, mScript;

    void load(ESMReader &esm);
    void save(ESMWriter &esm) const;

    void blank();
    ///< Set record to default state (does not touch the ID).
};
}
#endif
