#include "objects.hpp"

#include "occlusionquerynode.hpp"

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
    mOQNSettings.enable = Settings::Manager::getBool("octree occlusion queries enable", "OcclusionQueries");
    mOQNSettings.debugDisplay = Settings::Manager::getBool("debug occlusion queries", "OcclusionQueries");
    mOQNSettings.querypixelcount = Settings::Manager::getInt("visibility threshold", "OcclusionQueries");
    mOQNSettings.queryframecount = Settings::Manager::getInt("queries frame count", "OcclusionQueries");
    mOQNSettings.maxCellSize = Settings::Manager::getFloat("max cell size", "OcclusionQueries");
    mOQNSettings.minOQNSize = Settings::Manager::getFloat("min node size", "OcclusionQueries");
    mOQNSettings.maxDrawablePerOQN = Settings::Manager::getInt("max node drawables", "OcclusionQueries");
    mOQNSettings.querymargin = Settings::Manager::getFloat("queries margin", "OcclusionQueries");
    mOQNSettings.maxBVHOQLevelCount = Settings::Manager::getInt("max BVH OQ level count", "OcclusionQueries");
}

Objects::~Objects()
{
    mObjects.clear();

    for (CellMap::iterator iter = mCellSceneNodes.begin(); iter != mCellSceneNodes.end(); ++iter)
        iter->second->getParent(0)->removeChild(iter->second);
    mCellSceneNodes.clear();
}

osg::Group * Objects::insertBegin(const MWWorld::Ptr& ptr)
{
    assert(mObjects.find(ptr) == mObjects.end());

    osg::ref_ptr<osg::Group> cellnode;

    CellMap::iterator found = mCellSceneNodes.find(ptr.getCell());
    if (found == mCellSceneNodes.end())
    {
        cellnode = new osg::Group;
        if(mOQNSettings.enable)
        {
            StaticOcclusionQueryNode* qnode = new StaticOcclusionQueryNode;
            for(unsigned int i=0; i<8; ++i)
                qnode->addChild(new osg::Group());
            qnode->setDebugDisplay(mOQNSettings.debugDisplay);
            qnode->setVisibilityThreshold(mOQNSettings.querypixelcount);
            qnode->setQueryFrameCount(mOQNSettings.queryframecount);
            qnode->setQueryMargin(mOQNSettings.querymargin);
            cellnode = qnode;
        }
        cellnode->setName("Cell Root");
        cellnode->setDataVariance(osg::Object::DYNAMIC);
        mRootNode->addChild(cellnode);
        mCellSceneNodes[ptr.getCell()] = cellnode;
    }
    else
        cellnode = found->second;

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

struct OctreeAddRemove
{
    OctreeAddRemove(const OcclusionQuerySettings & settings): mSettings(settings) {}
    const OcclusionQuerySettings & mSettings;

    void recursivCellAddStaticObject(osg::BoundingSphere&bs, StaticOcclusionQueryNode &parent, osg::Group *child, osg::BoundingSphere& childbs)
    {
        osg::Vec3i index =osg::Vec3i(childbs.center()[0]<bs.center()[0]?0:1,
                                     childbs.center()[1]<bs.center()[1]?0:1,
                                     childbs.center()[2]<bs.center()[2]?0:1);
        unsigned int ind = index[0] + index[1]*2 + index[2]*4;
        osg::Vec3 indexf = osg::Vec3(index[0]*2-1, index[1]*2-1, index[2]*2-1);
        osg::BoundingSphere bsi;
        bsi.radius() = bs.radius() * 0.5f;
        bsi.center() = bs.center() + indexf * bsi.radius();
        osg::ref_ptr<StaticOcclusionQueryNode> qnode;
        osg::ref_ptr<osg::Group> target = parent.getChild(ind)->asGroup();

        osg::BoundingSphere bst (target->getBound());
        if( bst.valid()
            && bst.radius() > mSettings.minOQNSize
            && bsi.radius() > mSettings.minOQNSize
            && target->getNumChildren()> mSettings.maxDrawablePerOQN
          )
        {
            qnode=dynamic_cast<StaticOcclusionQueryNode*>(target.get());
            if(!qnode.valid())
            {
                OSG_INFO<<"new OcclusionQueryNode with radius "<<bs.radius()<<std::endl;
                qnode= new StaticOcclusionQueryNode;
                for(unsigned int i=0; i<8; ++i)
                    qnode->addChild(new osg::Group);

                qnode->setQueryMargin(mSettings.querymargin);
                qnode->setVisibilityThreshold(mSettings.querypixelcount);
                qnode->setDebugDisplay(mSettings.debugDisplay);
                qnode->setQueryFrameCount(mSettings.queryframecount);

                for(unsigned int i=0; i<target->getNumChildren(); ++i)
                {
                    osg::Group * childi = target->getChild(i)->asGroup();
                    osg::BoundingSphere bschild (childi->getBound());
                    recursivCellAddStaticObject(bsi, *qnode, childi, bschild);
                }
                parent.setChild(ind, qnode);
                //disable not terminal query
                float powlev = float(1<<mSettings.maxBVHOQLevelCount);
                if(powlev>1)
                    if(bsi.radius() <powlev*mSettings.minOQNSize)
                    {
                        OSG_INFO<<"masking high level OQN"<<std::endl;
                        parent.getQueryGeometry()->setNodeMask(0);
                        parent.getDebugGeometry()->setNodeMask(0);
                        parent.setQueriesEnabled(false);
                        qnode->getQueryGeometry()->setNodeMask(0);
                        qnode->getDebugGeometry()->setNodeMask(0);
                        qnode->setQueriesEnabled(false);
                    }
            }
            recursivCellAddStaticObject(bsi, *qnode, child, childbs);
            qnode->invalidateQueryGeometry();

        }
        else
        {
            target->addChild(child);
            unsigned int nodemask = VisMask::Mask_RenderToTexture;
            parent.getQueryGeometry()->setNodeMask(nodemask);
            parent.getDebugGeometry()->setNodeMask(nodemask);
            parent.setQueriesEnabled(true);
        }
    }

    bool recursivCellRemoveStaticObject(osg::OcclusionQueryNode & parent, osg::Node * childtoremove)
    {
        osg::Group * pchild; bool removed=false;
        for(unsigned int i=0; i< parent.getNumChildren(); ++i)
        {
            pchild = parent.getChild(i)->asGroup();
            if((removed = pchild->removeChild(childtoremove))) break;
        }
        //TODO check criterion for parent splitting

        if(removed)
            return true;
        else
        {
            for(unsigned int i=0; i< parent.getNumChildren(); ++i)
            {
                osg::OcclusionQueryNode * child = dynamic_cast<osg::OcclusionQueryNode*>(parent.getChild(i));
                if(child && recursivCellRemoveStaticObject(*child, childtoremove))
                    return true;
            }
        }
        return false;
    }
};

void Objects::insertModel(const MWWorld::Ptr &ptr, const std::string &mesh, bool animated, bool allowLight)
{
    osg::Group *cellroot = insertBegin(ptr);
    SceneUtil::PositionAttitudeTransform* basenode = ptr.getRefData().getBaseNode();

    basenode->setNodeMask(Mask_Object);
    osg::ref_ptr<ObjectAnimation> anim (new ObjectAnimation(ptr, mesh, mResourceSystem, animated, allowLight));

    StaticOcclusionQueryNode *ocq = dynamic_cast<StaticOcclusionQueryNode*>(cellroot);
    if(ocq)
    {
        // TO FIX Hacky way to retrieve cell bounds
        // the first child center considered as cell center
        // cellsize is set statically in settings.cfg
        osg::BoundingSphere bs = ocq->getInitialBound();
        if(!bs.valid())
        {
            bs = basenode->getBound();
            bs.radius() = mOQNSettings.maxCellSize;
            ocq->setInitialBound(bs);
        }

        osg::BoundingSphere bsi = basenode->getBound();
        if(bs.valid() && bsi.valid() )
        {
            OctreeAddRemove adder(mOQNSettings);
            adder.recursivCellAddStaticObject(bs, *ocq, basenode, bsi);
        }
    }
    else cellroot->addChild(basenode);

    mObjects.insert(std::make_pair(ptr, anim));
}

void Objects::insertCreature(const MWWorld::Ptr &ptr, const std::string &mesh, bool weaponsShields)
{
    osg::Group *cellroot = insertBegin(ptr);
    SceneUtil::PositionAttitudeTransform* basenode = ptr.getRefData().getBaseNode();
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
    SceneUtil::PositionAttitudeTransform* basenode = ptr.getRefData().getBaseNode();
    ptr.getRefData().getBaseNode()->setNodeMask(Mask_Actor);

    osg::ref_ptr<NpcAnimation> anim (new NpcAnimation(ptr, osg::ref_ptr<osg::Group>(ptr.getRefData().getBaseNode()), mResourceSystem));

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
        osg::OcclusionQueryNode* ocq = dynamic_cast<osg::OcclusionQueryNode*>(cellroot);
        if(ocq)
        {
            OctreeAddRemove remover(mOQNSettings);
            if(!remover.recursivCellRemoveStaticObject(*ocq, ptr.getRefData().getBaseNode()))
                OSG_WARN<<"removal failed"<<std::endl;
        }
        else cellroot->removeChild(ptr.getRefData().getBaseNode());

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
