#include "actor.hpp"

#include <iostream>

#include <osg/Group>
#include <osg/Node>

#include <components/esm/mappings.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcemanager.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/attach.hpp>
#include <components/sceneutil/skeleton.hpp>
#include <components/sceneutil/visitor.hpp>

#include "../../model/world/data.hpp"

namespace CSVRender
{
    Actor::Actor(const std::string& id, int type, CSMWorld::Data& data)
        : mId(id)
        , mType(type)
        , mData(data)
        , mSkeleton(nullptr)
        , mBaseNode(new osg::Group())
    {
    }

    osg::Group* Actor::getBaseNode()
    {
        return mBaseNode;
    }

    void Actor::update()
    {
        const std::string MeshPrefix = "meshes\\";
        const unsigned int FemaleFlag = ESM::BodyPart::BPF_Female;

        auto& bodyParts = mData.getBodyParts();
        auto& races = mData.getRaces();
        auto& referenceables = mData.getReferenceables();
        auto sceneMgr = mData.getResourceSystem()->getSceneManager();


        // Remove children
        mBaseNode->removeChildren(0, mBaseNode->getNumChildren());

        // Npcs and creatures are handled differently
        if (mType == CSMWorld::UniversalId::Type_Npc)
        {
            auto& npc = dynamic_cast<const CSMWorld::Record<ESM::NPC>& >(referenceables.getRecord(mId)).get();

            auto isBeast = [&](std::string race) -> bool {
                int index = races.searchId(race);
                if (index != -1 && !races.getRecord(index).isDeleted())
                    return races.getRecord(index).get().mData.mFlags & ESM::Race::Beast;
                else
                    return false;
            };

            // Load skeleton
            std::string skeletonResource;
            if (isBeast(npc.mRace)) {
                std::cout << "is beast\n";
                skeletonResource = "base_animkna.nif";
            }
            else if (npc.isMale()) {
                std::cout << "is male\n";
                skeletonResource = "base_anim.nif";
            }
            else {
                std::cout << "is female\n";
                skeletonResource = "base_anim_female.nif";
            }

            std::string skeletonModel = MeshPrefix + skeletonResource;
            skeletonModel = Misc::ResourceHelpers::correctActorModelPath(skeletonModel, mData.getResourceSystem()->getVFS());
            {
                osg::ref_ptr<osg::Node> temp = sceneMgr->getInstance(skeletonModel);
                mSkeleton = dynamic_cast<SceneUtil::Skeleton*>(temp.get());
                if (!mSkeleton)
                {
                    mSkeleton = new SceneUtil::Skeleton();
                    mSkeleton->addChild(temp);
                }
                mBaseNode->addChild(mSkeleton);
            }

            // Map bone names to bones
            SceneUtil::NodeMapVisitor::NodeMap nodeMap;
            SceneUtil::NodeMapVisitor nmVisitor(nodeMap);
            mSkeleton->accept(nmVisitor);

            if (!npc.isMale()) {
                for (auto it : nodeMap) {
                    std::cout << it.first << "\n";
                }
            }


            // Female mesh has some drawables attached, get rid of them
            SceneUtil::HideDrawablesVisitor hdVisitor;
            mSkeleton->accept(hdVisitor);

            // Convenience method to retrieve the mesh name of a body part
            auto getBodyPartMesh = [&](std::string bpName) -> std::string {
                int index = bodyParts.searchId(bpName);
                if (index != -1 && !bodyParts.getRecord(index).isDeleted())
                    return MeshPrefix + bodyParts.getRecord(index).get().mModel;
                else
                    return "";
            };

            using BPRaceKey = std::tuple<int, int, std::string>;
            using RaceToBPMap = std::map<BPRaceKey, std::string>;
            // Convenience method to generate a map from body part + race to mesh name
            auto genRaceToBodyPartMap = [&](RaceToBPMap& bpMap) {
                int size = bodyParts.getSize();
                for (int i = 0; i < size; ++i)
                {
                    auto& record = bodyParts.getRecord(i);
                    if (!record.isDeleted())
                    {
                        auto& bodyPart = record.get();
                        bpMap.emplace(
                            BPRaceKey(bodyPart.mData.mPart, bodyPart.mData.mFlags & FemaleFlag ? 1 : 0, bodyPart.mRace),
                            MeshPrefix + bodyPart.mModel);
                    }
                }
            };

            // Generate mapping
            RaceToBPMap r2bpMap;
            genRaceToBodyPartMap(r2bpMap);

            // Convenience method to add a body part
            auto addBodyPart = [&](ESM::PartReferenceType type, std::string mesh) {
                // Retrieve mesh name if necessary
                if (mesh.empty())
                {
                    auto meshResult = r2bpMap.find(BPRaceKey(ESM::getMeshPart(type), npc.isMale() ? 0 : 1, npc.mRace));
                    if (meshResult != r2bpMap.end())
                    {
                        mesh = meshResult->second;
                    }
                }

                // Attach to skeleton
                std::string boneName = ESM::getBoneName(type);
                auto node = nodeMap.find(boneName);
                if (!mesh.empty() && node != nodeMap.end())
                {
                    auto instance = sceneMgr->getInstance(mesh);
                    if (!npc.isMale() && type == ESM::PRT_LHand) {
                        SceneUtil::NodeMapVisitor::NodeMap handNodeMap;
                        SceneUtil::NodeMapVisitor nmVisitor(handNodeMap);
                        instance->accept(nmVisitor);

                        std::cout << "Left hand\n";
                        for (auto it : handNodeMap) {
                            std::cout << it.first << std::endl;
                        }
                    }
                    SceneUtil::attach(instance, mSkeleton, boneName, node->second);
                }
            };

            // Add body parts
            for (unsigned int i = 0; i < ESM::PRT_Count; ++i)
            {
                auto part = static_cast<ESM::PartReferenceType>(i);
                switch (part)
                {
                    case ESM::PRT_Head:
                        addBodyPart(part, getBodyPartMesh(npc.mHead));
                        break;
                    case ESM::PRT_Hair:
                        addBodyPart(part, getBodyPartMesh(npc.mHair));
                        break;
                    case ESM::PRT_Skirt:
                    case ESM::PRT_Shield:
                    case ESM::PRT_RPauldron:
                    case ESM::PRT_LPauldron:
                    case ESM::PRT_Weapon:
                        // No body part mesh associated
                        break;
                    default:
                        addBodyPart(part, "");
                }
            }
            // Post setup
            mSkeleton->markDirty();
            mSkeleton->setActive(SceneUtil::Skeleton::Active);
        }
    }
}
