#include "objects.hpp"

#include <osg/UserDataContainer>
#include <osg/ValueObject>

#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/sceneutil/util.hpp>
#include <osg/PositionAttitudeTransform>
#include <osg/MatrixTransform>
#include <components/sceneutil/unrefqueue.hpp>
#include <components/settings/settings.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/class.hpp"

#include "../mwworld/cellstore.hpp"
#include <components/esm/loadcell.hpp>
#include <components/esm/loadland.hpp>

#include "animation.hpp"
#include "npcanimation.hpp"
#include "creatureanimation.hpp"
#include "vismask.hpp"
#include "renderbin.hpp"
#include "components/sceneutil/optimizer.hpp"
#include <osgUtil/Optimizer>
#include <osgUtil/TransformAttributeFunctor>

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
        iter->second->getParent(0)->removeChild(iter->second);
    mCellSceneNodes.clear();
}

osg::Group * Objects::getOrCreateCell(const MWWorld::Ptr& ptr)
{

    osg::ref_ptr<osg::Group> cellnode;
    CellMap::iterator found = mCellSceneNodes.find(ptr.getCell());
    if (found == mCellSceneNodes.end())
    {
        SceneUtil::PositionAttitudeTransform *cell = new SceneUtil::PositionAttitudeTransform;
        cell->setPosition(SceneUtil::getCellOrigin(ptr.getCell()->getCell()));
        cellnode = cell;
        cellnode->setName("Cell Root");
        cellnode->setDataVariance(osg::Object::DYNAMIC);
        mRootNode->addChild(cellnode);
        mCellSceneNodes[ptr.getCell()] = cellnode;
    }
    else
        cellnode = found->second;
    return cellnode;
}

osg::Group * Objects::insertBegin(const MWWorld::Ptr& ptr)
{
    assert(mObjects.find(ptr) == mObjects.end());

    osg::ref_ptr<osg::Group> cellnode = getOrCreateCell(ptr);

    SceneUtil::PositionAttitudeTransform* insert = new SceneUtil::PositionAttitudeTransform;

    insert->getOrCreateUserDataContainer()->addUserObject(new PtrHolder(ptr));

    osg::Vec3 fc =SceneUtil::getCellOrigin(ptr.getCell()->getCell());

    const float scale = ptr.getCellRef().getScale();
    const float *f = ptr.getRefData().getPosition().pos;
    insert->setPosition(osg::Vec3(f[0]-fc[0], f[1]-fc[1], f[2]-fc[2]));

    osg::Vec3f scaleVec(scale, scale, scale);
    ptr.getClass().adjustScale(ptr, scaleVec, true);
    insert->setScale(scaleVec);

    ptr.getRefData().setBaseNode(insert);
    return cellnode;
}

void Objects::insertModel(const MWWorld::Ptr &ptr, const std::string &mesh, unsigned int mask, bool animated, bool allowLight)
{
    osg::Group *cellroot = insertBegin(ptr);
    SceneUtil::PositionAttitudeTransform* transbasenode = static_cast<SceneUtil::PositionAttitudeTransform *>(ptr.getRefData().getBaseNode());
    osg::Group * basenode = transbasenode;
    osg::ref_ptr<ObjectAnimation> anim;
    if(     /// quite a condition
            !animated &&
            (mask & Mask_Static) &&
            !ptr.getClass().isMobile(ptr) &&
            !ptr.getClass().isActivator() &&
            !ptr.getClass().isDoor() &&
            Settings::Manager::getBool("flattenstatics", "General"))
    {
        ptr.getRefData().setBaseNodeFlatten(true);

        /// substitute transform transbasenode with a group
        osg::Group* sub = new osg::Group;
        sub->getOrCreateUserDataContainer()->addUserObject(new PtrHolder(ptr));

        ptr.getRefData().setBaseNode(sub);
        anim =new ObjectAnimation(ptr, mesh, mResourceSystem, animated, allowLight);

        //save transform
        sub->getChild(0)->setUserData(transbasenode);
        transbasenode->addChild(sub->getChild(0));

        basenode = sub;
        basenode->setDataVariance(osg::Object::STATIC);
    }else
        anim = new ObjectAnimation(ptr, mesh, mResourceSystem, animated, allowLight);

    basenode->setNodeMask(mask);

    cellroot->addChild(basenode);

    mObjects.insert(std::make_pair(ptr, anim));
}

void Objects::insertCreature(const MWWorld::Ptr &ptr, const std::string &mesh, bool weaponsShields)
{
    osg::Group *cellroot = insertBegin(ptr);
    SceneUtil::PositionAttitudeTransform* basenode = static_cast<SceneUtil::PositionAttitudeTransform *>(ptr.getRefData().getBaseNode());

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
    osg::Group *cellroot = insertBegin(ptr);
    SceneUtil::PositionAttitudeTransform* basenode = static_cast<SceneUtil::PositionAttitudeTransform *>(ptr.getRefData().getBaseNode());

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
    osg::Group *basenode =  ptr.getRefData().getBaseNode();
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
        osg::Group *cellroot = mCellSceneNodes[ptr.getCell()];

        cellroot->removeChild(basenode);

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
    osg::Group* objectNode = cur.getRefData().getBaseNode()->asGroup();
    if (!objectNode)
        return;

    osg::Group* cellnode = getOrCreateCell(cur);

    osg::UserDataContainer* userDataContainer = objectNode->getUserDataContainer();
    if (userDataContainer)
        for (unsigned int i=0; i<userDataContainer->getNumUserObjects(); ++i)
        {
            if (dynamic_cast<PtrHolder*>(userDataContainer->getUserObject(i)))
                userDataContainer->setUserObject(i, new PtrHolder(cur));
        }

    osg::Vec3 oldorig(0,0,0);
    if (objectNode->getNumParents()>0)
    {
        objectNode->getParent(0)->removeChild(objectNode);
        oldorig = SceneUtil::getCellOrigin(old.getCell()->getCell());
    }
    SceneUtil::PositionAttitudeTransform* trans = cur.getRefData().isBaseNodeFlatten() ?
        static_cast<SceneUtil::PositionAttitudeTransform*>(cur.getRefData().getBaseNode()->getChild(0)->getUserData())
        : static_cast<SceneUtil::PositionAttitudeTransform*>(cur.getRefData().getBaseNode());

    const float *f = cur.getRefData().getPosition().pos;
    trans->setPosition( osg::Vec3(f[0],f[1],f[2])-SceneUtil::getCellOrigin(cur.getCell()->getCell()));
    cellnode->addChild(objectNode);

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
