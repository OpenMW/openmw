#include "objects.hpp"

#include <cmath>

#include <osg/io_utils>
#include <osg/Group>
#include <osg/Geode>
#include <osg/PositionAttitudeTransform>

#include <osgParticle/ParticleSystem>
#include <osgParticle/ParticleProcessor>

// light
#include <components/sceneutil/lightmanager.hpp>

#include <components/resource/scenemanager.hpp>

#include <components/sceneutil/visitor.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/class.hpp"

#include "renderconst.hpp"
#include "animation.hpp"
#include "npcanimation.hpp"
#include "creatureanimation.hpp"

namespace
{

    /// Removes all particle systems and related nodes in a subgraph.
    class RemoveParticlesVisitor : public osg::NodeVisitor
    {
    public:
        RemoveParticlesVisitor()
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        { }

        virtual void apply(osg::Node &node)
        {
            if (dynamic_cast<osgParticle::ParticleSystem*>(&node) || dynamic_cast<osgParticle::ParticleProcessor*>(&node))
                mToRemove.push_back(&node);

            traverse(node);
        }

        void remove()
        {
            for (std::vector<osg::ref_ptr<osg::Node> >::iterator it = mToRemove.begin(); it != mToRemove.end(); ++it)
            {
                osg::Node* node = *it;
                if (node->getNumParents())
                    node->getParent(0)->removeChild(node);
            }
            mToRemove.clear();
        }

    private:
        std::vector<osg::ref_ptr<osg::Node> > mToRemove;
    };

}


namespace MWRender
{

Objects::Objects(Resource::ResourceSystem* resourceSystem, osg::ref_ptr<osg::Group> rootNode)
    : mResourceSystem(resourceSystem)
    , mRootNode(rootNode)
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

    osg::ref_ptr<osg::PositionAttitudeTransform> insert (new osg::PositionAttitudeTransform);
    cellnode->addChild(insert);

    const float *f = ptr.getRefData().getPosition().pos;

    insert->setPosition(osg::Vec3(f[0], f[1], f[2]));

    ptr.getRefData().setBaseNode(insert);
}

void Objects::insertModel(const MWWorld::Ptr &ptr, const std::string &mesh, bool animated, bool allowLight)
{
    insertBegin(ptr);

    std::auto_ptr<ObjectAnimation> anim (new ObjectAnimation(ptr, mesh, mResourceSystem, allowLight));

    if (anim->getObjectRoot())
        anim->getObjectRoot()->addCullCallback(new SceneUtil::LightListCallback);

    if (!allowLight)
    {
        RemoveParticlesVisitor visitor;
        anim->getObjectRoot()->accept(visitor);
        visitor.remove();
    }

    mObjects.insert(std::make_pair(ptr, anim.release()));
}

void Objects::insertCreature(const MWWorld::Ptr &ptr, const std::string &mesh, bool weaponsShields)
{
    insertBegin(ptr);

    // CreatureAnimation
    std::auto_ptr<Animation> anim;

    if (weaponsShields)
        anim.reset(new CreatureWeaponAnimation(ptr, mesh, mResourceSystem));
    else
        anim.reset(new CreatureAnimation(ptr, mesh, mResourceSystem));

    if (anim->getObjectRoot())
        anim->getObjectRoot()->addCullCallback(new SceneUtil::LightListCallback);

    mObjects.insert(std::make_pair(ptr, anim.release()));
}

void Objects::insertNPC(const MWWorld::Ptr &ptr)
{
    insertBegin(ptr);

    std::auto_ptr<NpcAnimation> anim (new NpcAnimation(ptr, osg::ref_ptr<osg::Group>(ptr.getRefData().getBaseNode()), mResourceSystem, 0));

    if (anim->getObjectRoot())
        anim->getObjectRoot()->addCullCallback(new SceneUtil::LightListCallback);

    mObjects.insert(std::make_pair(ptr, anim.release()));
}

bool Objects::deleteObject (const MWWorld::Ptr& ptr)
{
    if(!ptr.getRefData().getBaseNode())
        return true;

    PtrAnimationMap::iterator iter = mObjects.find(ptr);
    if(iter != mObjects.end())
    {
        delete iter->second;
        mObjects.erase(iter);

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
        mCellSceneNodes.erase(cell);
    }
}

void Objects::update(float dt)
{

}

void Objects::updateObjectCell(const MWWorld::Ptr &old, const MWWorld::Ptr &cur)
{
    /*
    Ogre::SceneNode *node;
    MWWorld::CellStore *newCell = cur.getCell();

    if(mCellSceneNodes.find(newCell) == mCellSceneNodes.end()) {
        node = mRootNode->createChildSceneNode();
        mCellSceneNodes[newCell] = node;
    } else {
        node = mCellSceneNodes[newCell];
    }

    node->addChild(cur.getRefData().getBaseNode());

    PtrAnimationMap::iterator iter = mObjects.find(old);
    if(iter != mObjects.end())
    {
        ObjectAnimation *anim = iter->second;
        mObjects.erase(iter);
        anim->updatePtr(cur);
        mObjects[cur] = anim;
    }
    */
}

Animation* Objects::getAnimation(const MWWorld::Ptr &ptr)
{
    PtrAnimationMap::const_iterator iter = mObjects.find(ptr);
    if(iter != mObjects.end())
        return iter->second;

    return NULL;
}

}
