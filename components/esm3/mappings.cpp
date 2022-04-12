#include "mappings.hpp"

#include <stdexcept>

namespace ESM
{
    BodyPart::MeshPart getMeshPart(PartReferenceType type)
    {
        switch(type)
        {
            case PRT_Head:
                return BodyPart::MP_Head;
            case PRT_Hair:
                return BodyPart::MP_Hair;
            case PRT_Neck:
                return BodyPart::MP_Neck;
            case PRT_Cuirass:
                return BodyPart::MP_Chest;
            case PRT_Groin:
                return BodyPart::MP_Groin;
            case PRT_RHand:
                return BodyPart::MP_Hand;
            case PRT_LHand:
                return BodyPart::MP_Hand;
            case PRT_RWrist:
                return BodyPart::MP_Wrist;
            case PRT_LWrist:
                return BodyPart::MP_Wrist;
            case PRT_RForearm:
                return BodyPart::MP_Forearm;
            case PRT_LForearm:
                return BodyPart::MP_Forearm;
            case PRT_RUpperarm:
                return BodyPart::MP_Upperarm;
            case PRT_LUpperarm:
                return BodyPart::MP_Upperarm;
            case PRT_RFoot:
                return BodyPart::MP_Foot;
            case PRT_LFoot:
                return BodyPart::MP_Foot;
            case PRT_RAnkle:
                return BodyPart::MP_Ankle;
            case PRT_LAnkle:
                return BodyPart::MP_Ankle;
            case PRT_RKnee:
                return BodyPart::MP_Knee;
            case PRT_LKnee:
                return BodyPart::MP_Knee;
            case PRT_RLeg:
                return BodyPart::MP_Upperleg;
            case PRT_LLeg:
                return BodyPart::MP_Upperleg;
            case PRT_Tail:
                return BodyPart::MP_Tail;
            default:
                throw std::runtime_error("PartReferenceType " +
                    std::to_string(type) + " not associated with a mesh part");
        }
    }

    std::string getBoneName(PartReferenceType type)
    {
        switch(type)
        {
            case PRT_Head:
                return "head";
            case PRT_Hair:
                return "head"; // This is purposeful.
            case PRT_Neck:
                return "neck";
            case PRT_Cuirass:
                return "chest";
            case PRT_Groin:
                return "groin";
            case PRT_Skirt:
                return "groin";
            case PRT_RHand:
                return "right hand";
            case PRT_LHand:
                return "left hand";
            case PRT_RWrist:
                return "right wrist";
            case PRT_LWrist:
                return "left wrist";
            case PRT_Shield:
                return "shield bone";
            case PRT_RForearm:
                return "right forearm";
            case PRT_LForearm:
                return "left forearm";
            case PRT_RUpperarm:
                return "right upper arm";
            case PRT_LUpperarm:
                return "left upper arm";
            case PRT_RFoot:
                return "right foot";
            case PRT_LFoot:
                return "left foot";
            case PRT_RAnkle:
                return "right ankle";
            case PRT_LAnkle:
                return "left ankle";
            case PRT_RKnee:
                return "right knee";
            case PRT_LKnee:
                return "left knee";
            case PRT_RLeg:
                return "right upper leg";
            case PRT_LLeg:
                return "left upper leg";
            case PRT_RPauldron:
                return "right clavicle";
            case PRT_LPauldron:
                return "left clavicle";
            case PRT_Weapon:
                return "weapon bone";
            case PRT_Tail:
                return "tail";
            default:
                throw std::runtime_error("unknown PartReferenceType");
        }
    }

    std::string getMeshFilter(PartReferenceType type)
    {
        switch(type)
        {
            case PRT_Hair:
                return "hair";
            default:
                return getBoneName(type);
        }
    }
}
