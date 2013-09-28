#ifndef _ESM_BODY_H
#define _ESM_BODY_H

#include "esm_reader.hpp"

namespace ESM
{

struct BodyPart
{
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
        MP_Tail = 14
    };

    enum Flags
    {
        BPF_Female = 1,
        BPF_Playable = 2
    };

    enum MeshType
    {
        MT_Skin = 0,
        MT_Clothing = 1,
        MT_Armor = 2
    };

    struct BYDTstruct
    {
        char part;
        char vampire;
        char flags;
        char type;
    };

    BYDTstruct data;
    std::string model, name;

    void load(ESMReader &esm);
};
}
#endif
