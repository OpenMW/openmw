#include "mappings.hpp"

#include <stdexcept>

namespace ESM
{
    ESM::BodyPart::MeshPart getMeshPart(ESM::PartReferenceType type)
    {
        switch(type)
        {
            case ESM::PRT_Head:
                return ESM::BodyPart::MP_Head;
            case ESM::PRT_Hair:
                return ESM::BodyPart::MP_Hair;
            case ESM::PRT_Neck:
                return ESM::BodyPart::MP_Neck;
            case ESM::PRT_Cuirass:
                return ESM::BodyPart::MP_Chest;
            case ESM::PRT_Groin:
                return ESM::BodyPart::MP_Groin;
            case ESM::PRT_RHand:
                return ESM::BodyPart::MP_Hand;
            case ESM::PRT_LHand:
                return ESM::BodyPart::MP_Hand;
            case ESM::PRT_RWrist:
                return ESM::BodyPart::MP_Wrist;
            case ESM::PRT_LWrist:
                return ESM::BodyPart::MP_Wrist;
            case ESM::PRT_RForearm:
                return ESM::BodyPart::MP_Forearm;
            case ESM::PRT_LForearm:
                return ESM::BodyPart::MP_Forearm;
            case ESM::PRT_RUpperarm:
                return ESM::BodyPart::MP_Upperarm;
            case ESM::PRT_LUpperarm:
                return ESM::BodyPart::MP_Upperarm;
            case ESM::PRT_RFoot:
                return ESM::BodyPart::MP_Foot;
            case ESM::PRT_LFoot:
                return ESM::BodyPart::MP_Foot;
            case ESM::PRT_RAnkle:
                return ESM::BodyPart::MP_Ankle;
            case ESM::PRT_LAnkle:
                return ESM::BodyPart::MP_Ankle;
            case ESM::PRT_RKnee:
                return ESM::BodyPart::MP_Knee;
            case ESM::PRT_LKnee:
                return ESM::BodyPart::MP_Knee;
            case ESM::PRT_RLeg:
                return ESM::BodyPart::MP_Upperleg;
            case ESM::PRT_LLeg:
                return ESM::BodyPart::MP_Upperleg;
            case ESM::PRT_Tail:
                return ESM::BodyPart::MP_Tail;
            default:
                throw std::runtime_error("PartReferenceType " +
                    std::to_string(type) + " not associated with a mesh part");
        }
    }

    std::string getBoneName(ESM::PartReferenceType type)
    {
        switch(type)
        {
            case ESM::PRT_Head:
                return "head";
            case ESM::PRT_Hair:
                return "head"; // This is purposeful.
            case ESM::PRT_Neck:
                return "neck";
            case ESM::PRT_Cuirass:
                return "chest";
            case ESM::PRT_Groin:
                return "groin";
            case ESM::PRT_Skirt:
                return "groin";
            case ESM::PRT_RHand:
                return "right hand";
            case ESM::PRT_LHand:
                return "left hand";
            case ESM::PRT_RWrist:
                return "right wrist";
            case ESM::PRT_LWrist:
                return "left wrist";
            case ESM::PRT_Shield:
                return "shield bone";
            case ESM::PRT_RForearm:
                return "right forearm";
            case ESM::PRT_LForearm:
                return "left forearm";
            case ESM::PRT_RUpperarm:
                return "right upper arm";
            case ESM::PRT_LUpperarm:
                return "left upper arm";
            case ESM::PRT_RFoot:
                return "right foot";
            case ESM::PRT_LFoot:
                return "left foot";
            case ESM::PRT_RAnkle:
                return "right ankle";
            case ESM::PRT_LAnkle:
                return "left ankle";
            case ESM::PRT_RKnee:
                return "right knee";
            case ESM::PRT_LKnee:
                return "left knee";
            case ESM::PRT_RLeg:
                return "right upper leg";
            case ESM::PRT_LLeg:
                return "left upper leg";
            case ESM::PRT_RPauldron:
                return "right clavicle";
            case ESM::PRT_LPauldron:
                return "left clavicle";
            case ESM::PRT_Weapon:
                return "weapon bone";
            case ESM::PRT_Tail:
                return "tail";
            default:
                throw std::runtime_error("unknown PartReferenceType");
        }
    }

    std::string getMeshFilter(ESM::PartReferenceType type)
    {
        switch(type)
        {
            case ESM::PRT_Hair:
                return "hair";
            default:
                return getBoneName(type);
        }
    }
}
