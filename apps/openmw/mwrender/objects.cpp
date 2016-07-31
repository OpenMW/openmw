#include "objects.hpp"

#include <cmath>

#include <osg/Group>
#include <osg/UserDataContainer>

#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/sceneutil/unrefqueue.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/class.hpp"

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
    for(PtrAnimationMap::iterator iter = mObjects.begin();iter != mObjects.end();++iter)
        delete iter->second;
    mObjects.clear();

    for (CellMap::iterator iter = mCellSceneNodes.begin(); iter != mCellSceneNodes.end(); ++iter)
        iter->second->getParent(0)->removeChild(iter->second);
    mCellSceneNodes.clear();
}

void Objects::insertBegin(const MWWorld::Ptr& ptr)
{
    osg::ref_ptr<osg::Group> cellnode;

    CellMap::iterator found = mCellSceneNodes.find(ptr.getCell());
    if (found == mCellSceneNodes.end())
    {
        cellnode = new osg::Group;
        mRootNode->addChild(cellnode);
        mCellSceneNodes[ptr.getCell()] = cellnode;
    }
    else
        cellnode = found->second;

    osg::ref_ptr<SceneUtil::PositionAttitudeTransform> insert (new SceneUtil::PositionAttitudeTransform);
    cellnode->addChild(insert);

    insert->getOrCreateUserDataContainer()->addUserObject(new PtrHolder(ptr));

    const float *f = ptr.getRefData().getPosition().pos;

    insert->setPosition(osg::Vec3(f[0], f[1], f[2]));

    const float scale = ptr.getCellRef().getScale();
    osg::Vec3f scaleVec(scale, scale, scale);
    ptr.getClass().adjustScale(ptr, scaleVec, true);
    insert->setScale(scaleVec);

    ptr.getRefData().setBaseNode(insert);
}

void Objects::insertModel(const MWWorld::Ptr &ptr, const std::string &mesh, bool animated, bool allowLight)
{
    insertBegin(ptr);

    std::auto_ptr<ObjectAnimation> anim (new ObjectAnimation(ptr, mesh, mResourceSystem, animated, allowLight));

    mObjects.insert(std::make_pair(ptr, anim.release()));
}

void Objects::insertCreature(const MWWorld::Ptr &ptr, const std::string &mesh, bool weaponsShields)
{
    insertBegin(ptr);
    ptr.getRefData().getBaseNode()->setNodeMask(Mask_Actor);

    // CreatureAnimation
    std::auto_ptr<Animation> anim;

    if (weaponsShields)
        anim.reset(new CreatureWeaponAnimation(ptr, mesh, mResourceSystem));
    else
        anim.reset(new CreatureAnimation(ptr, mesh, mResourceSystem));

    mObjects.insert(std::make_pair(ptr, anim.release()));
}

void Objects::insertNPC(const MWWorld::Ptr &ptr)
{
    insertBegin(ptr);
    ptr.getRefData().getBaseNode()->setNodeMask(Mask_Actor);

    std::auto_ptr<NpcAnimation> anim (new NpcAnimation(ptr, osg::ref_ptr<osg::Group>(ptr.getRefData().getBaseNode()), mResourceSystem));

    mObjects.insert(std::make_pair(ptr, anim.release()));
}

bool Objects::removeObject (const MWWorld::Ptr& ptr)
{
    if(!ptr.getRefData().getBaseNode())
        return true;

    PtrAnimationMap::iterator iter = mObjects.find(ptr);
    if(iter != mObjects.end())
    {
        delete iter->second;
        mObjects.erase(iter);

        if (mUnrefQueue.get())
            mUnrefQueue->push(ptr.getRefData().getBaseNode());

        ptr.getRefData().getBaseNode()->getParent(0)->removeChild(ptr.getRefData().getBaseNode());

        ptr.getRefData().setBaseNode(NULL);
        return true;
    }
    return false;
}


void Objects::removeCell(const MWWorld::CellStore* store)
{
    for(PtrAnimationMap::iterator iter = mObjects.begin();iter != mObjects.end();)
    {
        if(iter->first.getCell() == store)
        {
            if (mUnrefQueue.get())
                mUnrefQueue->push(iter->second->getObjectRoot());
            delete iter->second;
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
    osg::Node* objectNode = cur.getRefData().getBaseNode();
    if (!objectNode)
        return;

    MWWorld::CellStore *newCell = cur.getCell();

    osg::Group* cellnode;
    if(mCellSceneNodes.find(newCell) == mCellSceneNodes.end()) {
        cellnode = new osg::Group;
        mRootNode->addChild(cellnode);
        mCellSceneNodes[newCell] = cellnode;
    } else {
        cellnode = mCellSceneNodes[newCell];
    }

    osg::UserDataContainer* userDataContainer = objectNode->getUserDataContainer();
    if (userDataContainer)
        for (unsigned int i=0; i<userDataContainer->getNumUserObjects(); ++i)
        {
            if (dynamic_cast<PtrHolder*>(userDataContainer->getUserObject(i)))
                userDataContainer->setUserObject(i, new PtrHolder(cur));
        }

    if (objectNode->getNumParents())
        objectNode->getParent(0)->removeChild(objectNode);
    cellnode->addChild(objectNode);

    PtrAnimationMap::iterator iter = mObjects.find(old);
    if(iter != mObjects.end())
    {
        Animation *anim = iter->second;
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

    return NULL;
}

const Animation* Objects::getAnimation(const MWWorld::ConstPtr &ptr) const
{
    PtrAnimationMap::const_iterator iter = mObjects.find(ptr);
    if(iter != mObjects.end())
        return iter->second;

    return NULL;
}

}
