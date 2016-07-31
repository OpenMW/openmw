#ifndef OPENMW_ESM_BODY_H
#define OPENMW_ESM_BODY_H

#include <string>

namespace ESM
{

class ESMReader;
class ESMWriter;

struct BodyPart
{
    static unsigned int sRecordId;
    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string getRecordType() { return "BodyPart"; }

    enum MeshPart
    {
        MP_Head = 0,
        MP_Hair = 1,
        MP_Neck = 2,
        MP_Chest = 3,
        MP_Groin = 4,
        MP_Hand = 5,
        MP_Wrist = 6,
        MP_Forearm = 7,
        MP_Upperarm = 8,
        MP_Foot = 9,
        MP_Ankle = 10,
        MP_Knee = 11,
        MP_Upperleg = 12,
        MP_Clavicle = 13,
        MP_Tail = 14,

        MP_Count = 15
    };

    enum Flags
    {
        BPF_Female = 1,
        BPF_NotPlayable = 2
    };

    enum MeshType
    {
        MT_Skin = 0,
        MT_Clothing = 1,
        MT_Armor = 2
    };

    struct BYDTstruct
    {
        unsigned char mPart; // mesh part
        unsigned char mVampire; // boolean
        unsigned char mFlags;
        unsigned char mType; // mesh type
    };

    BYDTstruct mData;
    std::string mId, mModel, mRace;

    void load(ESMReader &esm, bool &isDeleted);
    void save(ESMWriter &esm, bool isDeleted = false) const;

    void blank();
    ///< Set record to default state (does not touch the ID).
};
}
#endif
