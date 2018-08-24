#include "actor.hpp"

#include <osg/Group>
#include <osg/Node>

#include <components/debug/debuglog.hpp>
#include <components/esm/mappings.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcemanager.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/actorutil.hpp>
#include <components/sceneutil/attach.hpp>
#include <components/sceneutil/skeleton.hpp>

#include "../../model/world/data.hpp"

namespace CSVRender
{
    const std::string Actor::MeshPrefix = "meshes\\";

    Actor::Actor(const std::string& id, int type, CSMWorld::Data& data)
        : mId(id)
        , mInitialized(false)
        , mType(type)
        , mData(data)
        , mBaseNode(new osg::Group())
        , mSkeleton(nullptr)
    {
    }

    osg::Group* Actor::getBaseNode()
    {
        return mBaseNode;
    }

    void Actor::update()
    {
        try
        {
            mBaseNode->removeChildren(0, mBaseNode->getNumChildren());

            if (mType == CSMWorld::UniversalId::Type_Npc)
                updateNpc();
            else if (mType == CSMWorld::UniversalId::Type_Creature)
                updateCreature();
        }
        catch (std::exception& e)
        {
            Log(Debug::Info) << "Exception in Actor::update(): " << e.what();
        }

        if (!mInitialized)
        {
            mInitialized = true;
            connect(mData.getActorAdapter(), SIGNAL(actorChanged(const std::string&)), this, SLOT(handleActorChanged(const std::string&)));
        }
    }

    void Actor::handleActorChanged(const std::string& refId)
    {
        if (mId == refId)
        {
            Log(Debug::Info) << "Actor::actorChanged " << mId;
            update();
        }
    }

    void Actor::updateCreature()
    {
        auto& referenceables = mData.getReferenceables();

        auto& creature = dynamic_cast<const CSMWorld::Record<ESM::Creature>& >(referenceables.getRecord(mId)).get();

        // Load skeleton with meshes
        std::string skeletonModel = MeshPrefix + creature.mModel;
        skeletonModel = Misc::ResourceHelpers::correctActorModelPath(skeletonModel, mData.getResourceSystem()->getVFS());
        loadSkeleton(skeletonModel);

        SceneUtil::RemoveTriBipVisitor removeTriBipVisitor;
        mSkeleton->accept(removeTriBipVisitor);
        removeTriBipVisitor.remove();

        // Attach weapons
        loadBodyParts(creature.mId);

        // Post setup
        mSkeleton->markDirty();
        mSkeleton->setActive(SceneUtil::Skeleton::Active);
    }

    void Actor::updateNpc()
    {
        auto& races = mData.getRaces();
        auto& referenceables = mData.getReferenceables();

        auto& npc = dynamic_cast<const CSMWorld::Record<ESM::NPC>& >(referenceables.getRecord(mId)).get();
        auto& race = dynamic_cast<const CSMWorld::Record<ESM::Race>& >(races.getRecord(npc.mRace)).get();

        bool is1stPerson = false;
        bool isFemale = !npc.isMale();
        bool isBeast = race.mData.mFlags & ESM::Race::Beast;
        bool isWerewolf = false;

        // Load skeleton
        std::string skeletonModel = SceneUtil::getActorSkeleton(is1stPerson, isFemale, isBeast, isWerewolf);
        skeletonModel = Misc::ResourceHelpers::correctActorModelPath(skeletonModel, mData.getResourceSystem()->getVFS());
        loadSkeleton(skeletonModel);

        // Get rid of the extra attachments
        SceneUtil::CleanObjectRootVisitor cleanVisitor;
        mSkeleton->accept(cleanVisitor);
        cleanVisitor.remove();

        // Attach parts to skeleton
        loadBodyParts(npc.mId);

        // Post setup
        mSkeleton->markDirty();
        mSkeleton->setActive(SceneUtil::Skeleton::Active);
    }

    void Actor::loadSkeleton(const std::string& model)
    {
        auto sceneMgr = mData.getResourceSystem()->getSceneManager();

        osg::ref_ptr<osg::Node> temp = sceneMgr->getInstance(model);
        mSkeleton = dynamic_cast<SceneUtil::Skeleton*>(temp.get());
        if (!mSkeleton)
        {
            mSkeleton = new SceneUtil::Skeleton();
            mSkeleton->addChild(temp);
        }
        mBaseNode->addChild(mSkeleton);

        // Map bone names to bones
        mNodeMap.clear();
        SceneUtil::NodeMapVisitor nmVisitor(mNodeMap);
        mSkeleton->accept(nmVisitor);

    }

    void Actor::loadBodyParts(const std::string& actorId)
    {
        auto actorAdapter = mData.getActorAdapter();
        auto partMap = actorAdapter->getActorPartMap(actorId);
        if (partMap)
        {
            for (auto& pair : *partMap)
                attachBodyPart(pair.first, getBodyPartMesh(pair.second));
        }
    }

    void Actor::attachBodyPart(ESM::PartReferenceType type, const std::string& mesh)
    {
        auto sceneMgr = mData.getResourceSystem()->getSceneManager();

        // Attach to skeleton
        std::string boneName = ESM::getBoneName(type);
        auto node = mNodeMap.find(boneName);
        if (!mesh.empty() && node != mNodeMap.end())
        {
            auto instance = sceneMgr->getInstance(mesh);
            SceneUtil::attach(instance, mSkeleton, boneName, node->second);
        }
    }

    std::string Actor::getBodyPartMesh(const std::string& bodyPartId)
    {
        const auto& bodyParts = mData.getBodyParts();

        int index = bodyParts.searchId(bodyPartId);
        if (index != -1 && !bodyParts.getRecord(index).isDeleted())
            return MeshPrefix + bodyParts.getRecord(index).get().mModel;
        else
            return "";
    }
}
