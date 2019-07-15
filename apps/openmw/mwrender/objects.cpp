#include "objects.hpp"

#include <osg/UserDataContainer>

#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/sceneutil/util.hpp>
#include <components/sceneutil/unrefqueue.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"

#include "animation.hpp"
#include "npcanimation.hpp"
#include "creatureanimation.hpp"
#include "vismask.hpp"

namespace MWRender
{

Objects::Objects(Resource::ResourceSystem* resourceSystem, osg::ref_ptr<osg::Group> rootNode, SceneUtil::UnrefQueue* unrefQueue)
    : mRootNode(rootNode)
    , mResourceSystem(resourceSystem)
    , mUnrefQueue(unrefQueue)
{
}

Objects::~Objects()
{
    mObjects.clear();

    for (CellMap::iterator iter = mCellSceneNodes.begin(); iter != mCellSceneNodes.end(); ++iter)
    {
        MWWorld::CellStore* cell = const_cast<MWWorld::CellStore*>(iter->first);
        cell->getBaseNode()->removeChild(iter->second);
        cell->setBaseNode(nullptr);
    }
    mCellSceneNodes.clear();
}

SceneUtil::PositionAttitudeTransform * Objects::getOrCreateCell(const MWWorld::CellStore* cellptr)
{
    CellMap::iterator found = mCellSceneNodes.find(cellptr);
    if (found == mCellSceneNodes.end())
    {
        SceneUtil::PositionAttitudeTransform * cellnode = new SceneUtil::PositionAttitudeTransform;
        cellnode->setPosition(SceneUtil::getCellOrigin(cellptr->getCell()));
        cellnode->setName("Cell Root");
        cellnode->setDataVariance(osg::Object::DYNAMIC);

        //remember Objects is in charge of maintening this ownership scheme
        const_cast<MWWorld::CellStore*>(cellptr)->setBaseNode(cellnode);
        mRootNode->addChild(cellnode);

        mCellSceneNodes[cellptr] = cellnode;
        return cellnode;
    }
    else
        return found->second;
}

SceneUtil::PositionAttitudeTransform * Objects::insertBegin(const MWWorld::Ptr& ptr)
{
    assert(mObjects.find(ptr) == mObjects.end());
    const MWWorld::CellStore * cell = ptr.getCell();
    SceneUtil::PositionAttitudeTransform * cellnode = getOrCreateCell(cell);
    SceneUtil::PositionAttitudeTransform * insert = new SceneUtil::PositionAttitudeTransform;

    insert->getOrCreateUserDataContainer()->addUserObject(new PtrHolder(ptr));

    osg::Vec3 fc = SceneUtil::getCellOrigin(cell->getCell());

    const float scale = ptr.getCellRef().getScale();
    const float *f = ptr.getRefData().getPosition().pos;
    insert->setPosition(osg::Vec3(f[0]-fc[0], f[1]-fc[1], f[2]-fc[2]));

    osg::Vec3f scaleVec(scale, scale, scale);
    ptr.getClass().adjustScale(ptr, scaleVec, true);
    insert->setScale(scaleVec);

    ptr.getRefData().setBaseNode(insert);
    return cellnode;
}

void Objects::insertModel(const MWWorld::Ptr &ptr, const std::string &mesh, bool animated, bool allowLight)
{
    SceneUtil::PositionAttitudeTransform *cellroot = insertBegin(ptr);
    SceneUtil::PositionAttitudeTransform * basenode = ptr.getRefData().getBaseNode();
    osg::ref_ptr<ObjectAnimation> anim = new ObjectAnimation(ptr, mesh, mResourceSystem, animated, allowLight);

    basenode->setNodeMask(Mask_Object);

    cellroot->addChild(basenode);

    mObjects.insert(std::make_pair(ptr, anim));
}

void Objects::insertCreature(const MWWorld::Ptr &ptr, const std::string &mesh, bool weaponsShields)
{
    SceneUtil::PositionAttitudeTransform * cellroot = insertBegin(ptr);
    SceneUtil::PositionAttitudeTransform * basenode = ptr.getRefData().getBaseNode();

    basenode->setDataVariance(osg::Object::DYNAMIC);
    basenode->setNodeMask(Mask_Actor);

    // CreatureAnimation
    osg::ref_ptr<Animation> anim;

    if (weaponsShields)
        anim = new CreatureWeaponAnimation(ptr, mesh, mResourceSystem);
    else
        anim = new CreatureAnimation(ptr, mesh, mResourceSystem);

     cellroot->addChild(basenode);

    if (mObjects.insert(std::make_pair(ptr, anim)).second)
        ptr.getClass().getContainerStore(ptr).setContListener(static_cast<ActorAnimation*>(anim.get()));
}

void Objects::insertNPC(const MWWorld::Ptr &ptr)
{
    SceneUtil::PositionAttitudeTransform *cellroot = insertBegin(ptr);
    SceneUtil::PositionAttitudeTransform* basenode = ptr.getRefData().getBaseNode();

    basenode->setDataVariance(osg::Object::DYNAMIC);
    basenode->setNodeMask(Mask_Actor);
    osg::ref_ptr<NpcAnimation> anim = new NpcAnimation(ptr, basenode, mResourceSystem);
    cellroot->addChild(basenode);

    if (mObjects.insert(std::make_pair(ptr, anim)).second)
    {
        ptr.getClass().getInventoryStore(ptr).setInvListener(anim.get(), ptr);
        ptr.getClass().getInventoryStore(ptr).setContListener(anim.get());
    }
}

bool Objects::removeObject (const MWWorld::Ptr& ptr)
{
    SceneUtil::PositionAttitudeTransform * basenode =  ptr.getRefData().getBaseNode();
    if(!basenode)
        return true;

    PtrAnimationMap::iterator iter = mObjects.find(ptr);
    if(iter != mObjects.end())
    {
        if (mUnrefQueue.get())
            mUnrefQueue->push(iter->second);

        mObjects.erase(iter);

        if (ptr.getClass().isActor())
        {
            if (ptr.getClass().hasInventoryStore(ptr))
                ptr.getClass().getInventoryStore(ptr).setInvListener(nullptr, ptr);

            ptr.getClass().getContainerStore(ptr).setContListener(nullptr);
        }

        ptr.getRefData().getBaseNode()->getParent(0)->removeChild(ptr.getRefData().getBaseNode());

        ptr.getRefData().setBaseNode(nullptr);
        return true;
    }
    return false;
}


void Objects::removeCell(const MWWorld::CellStore* store)
{
    for(PtrAnimationMap::iterator iter = mObjects.begin();iter != mObjects.end();)
    {
        MWWorld::Ptr ptr = iter->second->getPtr();
        if(ptr.getCell() == store)
        {
            if (mUnrefQueue.get())
                mUnrefQueue->push(iter->second);

            if (ptr.getClass().isNpc() && ptr.getRefData().getCustomData())
            {
                MWWorld::InventoryStore& invStore = ptr.getClass().getInventoryStore(ptr);
                invStore.setInvListener(nullptr, ptr);
                invStore.setContListener(nullptr);
            }

            mObjects.erase(iter++);
        }
        else
            ++iter;
    }

    CellMap::iterator cell = mCellSceneNodes.find(store);
    if(cell != mCellSceneNodes.end())
    {
        cell->second->getParent(0)->removeChild(cell->second);
        if (mUnrefQueue.get())
            mUnrefQueue->push(cell->second);
        mCellSceneNodes.erase(cell);
    }
}

void Objects::updatePtr(const MWWorld::Ptr &old, const MWWorld::Ptr &cur)
{
    SceneUtil::PositionAttitudeTransform * objectNode = cur.getRefData().getBaseNode();
    if (!objectNode)
        return;

    SceneUtil::PositionAttitudeTransform * curcellnode = cur.getCell()->getBaseNode();

    osg::UserDataContainer* userDataContainer = objectNode->getUserDataContainer();
    if (userDataContainer)
        for (unsigned int i=0; i<userDataContainer->getNumUserObjects(); ++i)
        {
            if (dynamic_cast<PtrHolder*>(userDataContainer->getUserObject(i)))
                userDataContainer->setUserObject(i, new PtrHolder(cur));
        }

    if (objectNode->getNumParents()>0)
    {
        objectNode->getParent(0)->removeChild(objectNode);
    }

    const float *f = cur.getRefData().getPosition().pos;
    objectNode->setPosition( osg::Vec3(f[0],f[1],f[2]) - curcellnode->getPosition());
    curcellnode->addChild(objectNode);

    PtrAnimationMap::iterator iter = mObjects.find(old);
    if(iter != mObjects.end())
    {
        osg::ref_ptr<Animation> anim = iter->second;
        mObjects.erase(iter);
        anim->updatePtr(cur);
        mObjects[cur] = anim;
    }
}

Animation* Objects::getAnimation(const MWWorld::Ptr &ptr)
{
    PtrAnimationMap::const_iterator iter = mObjects.find(ptr);
    if(iter != mObjects.end())
        return iter->second;

    return nullptr;
}

const Animation* Objects::getAnimation(const MWWorld::ConstPtr &ptr) const
{
    PtrAnimationMap::const_iterator iter = mObjects.find(ptr);
    if(iter != mObjects.end())
        return iter->second;

    return nullptr;
}

}
