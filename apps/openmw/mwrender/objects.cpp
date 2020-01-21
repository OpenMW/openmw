#include "objects.hpp"

#include <osg/UserDataContainer>
#include <osg/ValueObject>

#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/sceneutil/occlusionquerynode.hpp>
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
    mOQNSettings.minOQNSize = Settings::Manager::getFloat("min node size", "OcclusionQueries");
    mOQNSettings.maxOQNCapacity = Settings::Manager::getInt("max node capacity", "OcclusionQueries");
    mOQNSettings.querymargin = Settings::Manager::getFloat("queries margin", "OcclusionQueries");
    mOQNSettings.maxBVHOQLevelCount = Settings::Manager::getInt("max BVH OQ level count", "OcclusionQueries");
    mOQNSettings.securepopdistance = Settings::Manager::getFloat("min pop in distance", "OcclusionQueries");
    mOQNSettings.OQMask = Mask_OcclusionQuery;
    mOQNSettings.OQRenderBin = RenderBin_OcclusionQuery;

    ///update current queries
    SceneUtil::SettingsUpdatorVisitor updator(mOQNSettings);
    for (CellMap::iterator iter = mCellSceneNodes.begin(); iter != mCellSceneNodes.end(); ++iter)
        iter->second->accept(updator);
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
            ocnode->getQueryStateSet()->setRenderBinDetails( mOQNSettings.OQRenderBin, "SORT_FRONT_TO_BACK", osg::StateSet::PROTECTED_RENDERBIN_DETAILS);
            for(unsigned int i = 0; i<8; ++i)
                ocnode->addChild(new osg::Group());
            ocnode->setDebugDisplay(mOQNSettings.debugDisplay);
            ocnode->setVisibilityThreshold(mOQNSettings.querypixelcount);
            ocnode->setQueryFrameCount(mOQNSettings.queryframecount);
            ocnode->setQueryMargin(mOQNSettings.querymargin);            
            ocnode->getQueryGeometry()->setNodeMask(mOQNSettings.OQMask);
            ocnode->getDebugGeometry()->setNodeMask(mOQNSettings.OQMask);
            ocnode->setDataVariance(osg::Object::DYNAMIC);

            //not part of OQ hierarchy so disable query
            ocnode->setQueriesEnabled(false);
            ocnode->getQueryGeometry()->setNodeMask(0);
            ocnode->getDebugGeometry()->setNodeMask(0);

            SceneUtil::StaticOcclusionQueryNode*qnode = new SceneUtil::StaticOcclusionQueryNode;
            qnode->getQueryStateSet()->setRenderBinDetails( mOQNSettings.OQRenderBin, "SORT_FRONT_TO_BACK", osg::StateSet::PROTECTED_RENDERBIN_DETAILS);
            qnode->setDebugDisplay(mOQNSettings.debugDisplay);
            qnode->setVisibilityThreshold(mOQNSettings.querypixelcount);
            qnode->setQueryFrameCount(mOQNSettings.queryframecount);
            qnode->setQueryMargin(mOQNSettings.querymargin);

            //not part of OQ hierarchy so disable query
            qnode->setQueriesEnabled(false);
            qnode->getQueryGeometry()->setNodeMask(0);
            qnode->getDebugGeometry()->setNodeMask(0);
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

void Objects::cellAddStaticObject(osg::Group* cellnode, const MWWorld::Ptr &ptr ){
    SceneUtil::PositionAttitudeTransform *objectNode = ptr.getRefData().getBaseNode();
    ///could be static casted but leave a way to make occlusion query a runtime controlled
    SceneUtil::StaticOcclusionQueryNode* ocq = dynamic_cast<SceneUtil::StaticOcclusionQueryNode*>(cellnode);
    if(ocq)
    {
        ocq = static_cast<SceneUtil::StaticOcclusionQueryNode *>(ocq->getChild(0));
        const ESM::Cell * esmcell = ptr.getCell()->getCell();
        const ESM::CellId::CellIndex &cellid = esmcell->getCellId().mIndex;
        float cellSize = static_cast<float>(ESM::Land::REAL_SIZE);

        osg::BoundingSphere bs;
        bs.center() = osg::Vec3( (static_cast<float>(cellid.mX)+0.5f) * cellSize,
                                 (static_cast<float>(cellid.mY)+0.5f) * cellSize, 0);
        bs.radius() = 0.5f * cellSize;

        osg::BoundingSphere bsi = objectNode->getBound();
        if(objectNode->getNodeMask() == Mask_Actor)
        {
            SceneUtil::StaticOcclusionQueryNode * qnode=new SceneUtil::StaticOcclusionQueryNode();
            qnode->setQueryMargin(mOQNSettings.querymargin);
            qnode->setVisibilityThreshold(mOQNSettings.querypixelcount);
            qnode->setDebugDisplay(mOQNSettings.debugDisplay);
            qnode->setQueryFrameCount(mOQNSettings.queryframecount);
            qnode->setDistancePreventingPopin(mOQNSettings.securepopdistance);
            qnode->getQueryGeometry()->setNodeMask(mOQNSettings.OQMask);
            qnode->getDebugGeometry()->setNodeMask(mOQNSettings.OQMask);
            unsigned int childc = objectNode->getNumChildren();
            osg::Node * cht;
            for(unsigned int i=0; i<childc; ++i)
            {
                cht=objectNode->getChild(i);
                cht->setNodeMask(Mask_Object);//a bit hacky but required for actor to be OQN culled
                qnode->addChild(cht);
            }
            qnode->resetStaticQueryGeometry();
            objectNode->removeChildren(0, childc);
            objectNode->addChild(qnode);
            cellnode->addChild(objectNode);
        }
        else if(bsi.valid() )
        {
            SceneUtil::OctreeAddRemove adder(mOQNSettings);
            adder.recursivCellAddStaticObject(bs, *ocq, objectNode, bsi);
        }
        else cellnode->addChild(objectNode);
    }
    else cellnode->addChild(objectNode);
}

void Objects::cellRemoveStaticObject(osg::Group* cellnode,const MWWorld::Ptr &ptr){
    SceneUtil::PositionAttitudeTransform *objectNode = ptr.getRefData().getBaseNode();
    ///could be static casted but leave a way to make occlusion query a runtime controlled
    SceneUtil::StaticOcclusionQueryNode* ocq = dynamic_cast<SceneUtil::StaticOcclusionQueryNode*>(cellnode);
    if(ocq)
    {
        SceneUtil::OctreeAddRemove remover(mOQNSettings);
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

    basenode->setNodeMask(Mask_Object);

    osg::ref_ptr<ObjectAnimation> anim (new ObjectAnimation(ptr, mesh, mResourceSystem, animated, allowLight));

    cellAddStaticObject(cellroot, ptr);

    mObjects.insert(std::make_pair(ptr, anim));
}

void Objects::insertCreature(const MWWorld::Ptr &ptr, const std::string &mesh, bool weaponsShields)
{
    osg::Group *cellroot = insertBegin(ptr);
    SceneUtil::PositionAttitudeTransform* basenode = ptr.getRefData().getBaseNode();

    basenode->setDataVariance(osg::Object::DYNAMIC);
    basenode->setNodeMask(Mask_Actor);

    // CreatureAnimation
    osg::ref_ptr<Animation> anim;

    if (weaponsShields)
        anim = new CreatureWeaponAnimation(ptr, mesh, mResourceSystem);
    else
        anim = new CreatureAnimation(ptr, mesh, mResourceSystem);

     cellAddStaticObject(cellroot, ptr);

    if (mObjects.insert(std::make_pair(ptr, anim)).second)
        ptr.getClass().getContainerStore(ptr).setContListener(static_cast<ActorAnimation*>(anim.get()));
}

void Objects::insertNPC(const MWWorld::Ptr &ptr)
{
    osg::Group *cellroot = insertBegin(ptr);
    SceneUtil::PositionAttitudeTransform* basenode = ptr.getRefData().getBaseNode();

    basenode->setDataVariance(osg::Object::DYNAMIC);
    basenode->setNodeMask(Mask_Actor);

    osg::ref_ptr<NpcAnimation> anim = new NpcAnimation(ptr, basenode, mResourceSystem);

    cellAddStaticObject(cellroot, ptr);

    if (mObjects.insert(std::make_pair(ptr, anim)).second)
    {
        ptr.getClass().getInventoryStore(ptr).setInvListener(anim.get(), ptr);
        ptr.getClass().getInventoryStore(ptr).setContListener(anim.get());
    }
}

bool Objects::removeObject (const MWWorld::Ptr& ptr)
{
    SceneUtil::PositionAttitudeTransform *basenode = ptr.getRefData().getBaseNode();
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

        if(basenode->getNodeMask() & (Mask_Object | Mask_Static) )
            cellRemoveStaticObject(cellroot, ptr);
        else basenode->getParent(0)->removeChild(basenode);

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
    if(objectNode->getNodeMask() & (Mask_Object | Mask_Static) )
    {
        if (objectNode->getNumParents())
            cellRemoveStaticObject(getOrCreateCell(old), cur);
        cellAddStaticObject(cellnode, cur);
    }
    else {
        if (objectNode->getNumParents())
            objectNode->getParent(0)->removeChild(objectNode);
        cellnode->addChild(objectNode);
    }

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
