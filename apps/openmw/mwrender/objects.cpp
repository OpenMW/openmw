#include "objects.hpp"

#include <osg/UserDataContainer>

#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/sceneutil/unrefqueue.hpp>
#include <components/settings/settings.hpp>

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
    resetSettings();
}

void Objects::resetSettings()
{
    mOQNSettings.enable = Settings::Manager::getBool("octree occlusion queries enable", "OcclusionQueries");
    mOQNSettings.debugDisplay = Settings::Manager::getBool("debug occlusion queries", "OcclusionQueries");
    mOQNSettings.querypixelcount = Settings::Manager::getInt("visibility threshold", "OcclusionQueries");
    mOQNSettings.queryframecount = Settings::Manager::getInt("queries frame count", "OcclusionQueries");
    mOQNSettings.maxCellSize = Settings::Manager::getFloat("max cell size", "OcclusionQueries");
    mOQNSettings.minOQNSize = Settings::Manager::getFloat("min node size", "OcclusionQueries");
    mOQNSettings.maxDrawablePerOQN = Settings::Manager::getInt("max node drawables", "OcclusionQueries");
    mOQNSettings.querymargin = Settings::Manager::getFloat("queries margin", "OcclusionQueries");
    mOQNSettings.maxBVHOQLevelCount = Settings::Manager::getInt("max BVH OQ level count", "OcclusionQueries");
    mOQNSettings.securepopdistance = Settings::Manager::getFloat("min pop in distance", "OcclusionQueries");
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
        cellnode = new osg::Group;
        if(mOQNSettings.enable)
        {
            SceneUtil::StaticOcclusionQueryNode* ocnode = new SceneUtil::StaticOcclusionQueryNode;
            for(unsigned int i=0; i<8; ++i)
                ocnode->addChild(new osg::Group());
            ocnode->setDebugDisplay(mOQNSettings.debugDisplay);
            ocnode->setVisibilityThreshold(mOQNSettings.querypixelcount);
            ocnode->setQueryFrameCount(mOQNSettings.queryframecount);
            ocnode->setQueryMargin(mOQNSettings.querymargin);

            SceneUtil::StaticOcclusionQueryNode*qnode = new SceneUtil::StaticOcclusionQueryNode;
            qnode->setDebugDisplay(mOQNSettings.debugDisplay);
            qnode->setVisibilityThreshold(mOQNSettings.querypixelcount);
            qnode->setQueryFrameCount(mOQNSettings.queryframecount);
            qnode->setQueryMargin(mOQNSettings.querymargin);
            cellnode = qnode;
            cellnode->addChild(ocnode);

        }
        cellnode->setName("Cell Root");
        cellnode->setDataVariance(osg::Object::DYNAMIC);
        mRootNode->addChild(cellnode);
        mCellSceneNodes[ptr.getCell()] = cellnode;
    }
    else
        cellnode = found->second;
    return cellnode;
}

void Objects::cellAddStaticObject(osg::Group* cellnode, osg::Group* objectNode){
    ///could be static casted but leave a way to make occlusion query a runtime controlled
    SceneUtil::StaticOcclusionQueryNode* ocq = dynamic_cast<SceneUtil::StaticOcclusionQueryNode*>(cellnode);
    if(ocq)
    {
        // TO FIX Hacky way to retrieve cell bounds
        // the first child center considered as cell center
        // cellsize is set statically in settings.cfg
        ocq=static_cast<SceneUtil::StaticOcclusionQueryNode *>(ocq->getChild(0));
        osg::Node * center = dynamic_cast<osg::Node*>(ocq->getUserData());
        osg::BoundingSphere bs;
        if(!center)
        {
            bs = objectNode->getBound();
            osg::Node * n=new osg::Node();
            n->setInitialBound(bs);
            ocq->setUserData(n);
        }else bs.center()=center->getInitialBound().center();

        bs.radius() = mOQNSettings.maxCellSize;

        osg::BoundingSphere bsi = objectNode->getBound();
        if(bs.valid() && bsi.valid() )
        {
            SceneUtil::OctreeAddRemove adder(mOQNSettings, VisMask::Mask_OcclusionQuery);
            adder.recursivCellAddStaticObject(bs, *ocq, objectNode, bsi);
        }
    }
    else cellnode->addChild(objectNode);
}

void Objects::cellRemoveObject(osg::Group* cellnode, osg::Group* objectNode){
    ///could be static casted but leave a way to make occlusion query a runtime controlled
    SceneUtil::StaticOcclusionQueryNode* ocq = dynamic_cast<SceneUtil::StaticOcclusionQueryNode*>(cellnode);
    if(ocq && objectNode->getDataVariance()==osg::Object::STATIC)
    {
        SceneUtil::OctreeAddRemove remover(mOQNSettings, VisMask::Mask_OcclusionQuery);
        if(!remover.recursivCellRemoveStaticObject(*ocq, objectNode))
            OSG_WARN<<"removal failed"<<std::endl;
    }else if(!cellnode->removeChild(objectNode))
        OSG_WARN<<"removal failed"<<std::endl;
}

osg::Group * Objects::insertBegin(const MWWorld::Ptr& ptr)
{
    assert(mObjects.find(ptr) == mObjects.end());

    osg::ref_ptr<osg::Group> cellnode = getOrCreateCell(ptr);

    SceneUtil::PositionAttitudeTransform* insert = new SceneUtil::PositionAttitudeTransform;

    insert->getOrCreateUserDataContainer()->addUserObject(new PtrHolder(ptr));

    const float *f = ptr.getRefData().getPosition().pos;

    insert->setPosition(osg::Vec3(f[0], f[1], f[2]));

    const float scale = ptr.getCellRef().getScale();
    osg::Vec3f scaleVec(scale, scale, scale);
    ptr.getClass().adjustScale(ptr, scaleVec, true);
    insert->setScale(scaleVec);

    ptr.getRefData().setBaseNode(insert);
    return cellnode;
}


void Objects::insertModel(const MWWorld::Ptr &ptr, const std::string &mesh, bool animated, bool allowLight)
{
    osg::Group *cellroot = insertBegin(ptr);
    SceneUtil::PositionAttitudeTransform* basenode = ptr.getRefData().getBaseNode();
    basenode->setDataVariance(osg::Object::STATIC);
    basenode->setNodeMask(Mask_Object);
    osg::ref_ptr<ObjectAnimation> anim (new ObjectAnimation(ptr, mesh, mResourceSystem, animated, allowLight));

    cellAddStaticObject(cellroot, basenode);

    mObjects.insert(std::make_pair(ptr, anim));
}

void Objects::insertCreature(const MWWorld::Ptr &ptr, const std::string &mesh, bool weaponsShields)
{
    osg::Group *cellroot = insertBegin(ptr);
    SceneUtil::PositionAttitudeTransform* basenode = ptr.getRefData().getBaseNode();
    basenode->setNodeMask(Mask_Actor);
    basenode->setDataVariance(osg::Object::DYNAMIC);

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
    SceneUtil::PositionAttitudeTransform* basenode = ptr.getRefData().getBaseNode();
    basenode->setNodeMask(Mask_Actor);
    basenode->setDataVariance(osg::Object::DYNAMIC);

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
    if(!ptr.getRefData().getBaseNode())
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
        cellRemoveObject(cellroot, ptr.getRefData().getBaseNode());

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

    osg::Group* cellnode=getOrCreateCell(cur);

    osg::UserDataContainer* userDataContainer = objectNode->getUserDataContainer();
    if (userDataContainer)
        for (unsigned int i=0; i<userDataContainer->getNumUserObjects(); ++i)
        {
            if (dynamic_cast<PtrHolder*>(userDataContainer->getUserObject(i)))
                userDataContainer->setUserObject(i, new PtrHolder(cur));
        }

    if (objectNode->getNumParents()){
        osg::Group * par= objectNode->getParent(0);
        cellRemoveObject(par,objectNode);
    }
    if(objectNode->getDataVariance()==osg::Object::STATIC)
        cellAddStaticObject(cellnode,objectNode);
    else cellnode->addChild(objectNode);

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
