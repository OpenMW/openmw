#include "debugging.hpp"

#include <assert.h>

#include <OgreNode.h>
#include <OgreSceneManager.h>

#include "../mwworld/world.hpp" // these includes can be removed once the static-hack is gone
#include "../mwworld/environment.hpp"
#include "../mwworld/ptr.hpp"
#include <components/esm/loadstat.hpp>
#include <components/esm/loadpgrd.hpp>

#include "player.hpp"

using namespace MWRender;
using namespace Ogre;

Debugging::Debugging(SceneNode *mwRoot, MWWorld::Environment &env, OEngine::Physic::PhysicEngine *engine) :
    mMwRoot(mwRoot), mEnvironment(env), mEngine(engine),
    mSceneMgr(mwRoot->getCreator()),
    pathgridEnabled(false),
    mInteriorPathgridNode(NULL), mPathGridRoot(NULL)
{
}


bool Debugging::toggleRenderMode (int mode){
    switch (mode)
    {
        case MWWorld::World::Render_CollisionDebug:

            // TODO use a proper function instead of accessing the member variable
            // directly.
            mEngine->setDebugRenderingMode (!mEngine->isDebugCreated);
            return mEngine->isDebugCreated;
        case MWWorld::World::Render_Pathgrid:
            togglePathgrid();
            return pathgridEnabled;
    }

    return false;
}

void Debugging::cellAdded(MWWorld::Ptr::CellStore *store)
{
    std::cout << "Cell added to debugging" << std::endl;
    mActiveCells.push_back(store);
    if (pathgridEnabled)
        togglePathgridForCell(store, true);
}

void Debugging::cellRemoved(MWWorld::Ptr::CellStore *store)
{
    mActiveCells.erase(std::remove(mActiveCells.begin(), mActiveCells.end(), store), mActiveCells.end());
    std::cout << "Cell removed from debugging, active cells count: " << mActiveCells.size() << std::endl;
    if (pathgridEnabled)
        togglePathgridForCell(store, false);
}

void Debugging::togglePathgrid()
{
    pathgridEnabled = !pathgridEnabled;
    if (pathgridEnabled)
    {
        // add path grid meshes to already loaded cells
        mPathGridRoot = mMwRoot->createChildSceneNode();
        for(CellList::iterator it = mActiveCells.begin(); it != mActiveCells.end(); it++)
        {
            togglePathgridForCell(*it, true);
        }
    }
    else {
        // remove path grid meshes from already loaded cells
        for(CellList::iterator it = mActiveCells.begin(); it != mActiveCells.end(); it++)
        {
            togglePathgridForCell(*it, false);
        }
        mPathGridRoot->removeAndDestroyAllChildren();
        mSceneMgr->destroySceneNode(mPathGridRoot);
        mPathGridRoot = NULL;
    }
}

void Debugging::togglePathgridForCell(MWWorld::Ptr::CellStore *store, bool enabled)
{
    ESM::Pathgrid *pathgrid = mEnvironment.mWorld->getStore().pathgrids.search(*store->cell);
    if (!pathgrid)
    {
        return;
    }

    if (enabled)
    {
        Vector3 cellPathGridPos;
        /// \todo replace tests like this with isExterior method of ESM::Cell after merging with terrain branch
        if (!(store->cell->data.flags & ESM::Cell::Interior))
        {
            /// \todo Replace with ESM::Land::REAL_SIZE after merging with terrain branch
            cellPathGridPos.x = store->cell->data.gridX * 8192;
            cellPathGridPos.y = store->cell->data.gridY * 8192;
        }
        SceneNode *cellPathGrid = mPathGridRoot->createChildSceneNode(cellPathGridPos);
        ESM::Pathgrid::PointList points = pathgrid->points;
        for (ESM::Pathgrid::PointList::iterator it = points.begin(); it != points.end(); it++)
        {
            Vector3 position(it->x, it->y, it->z);
            SceneNode* pointNode = cellPathGrid->createChildSceneNode(position);
            pointNode->setScale(0.5, 0.5, 0.5);
            Entity *pointMesh = mSceneMgr->createEntity(SceneManager::PT_CUBE);
            pointNode->attachObject(pointMesh);
        }

        if (!(store->cell->data.flags & ESM::Cell::Interior))
        {
            mExteriorPathgridNodes[std::make_pair(store->cell->data.gridX, store->cell->data.gridY)] = cellPathGrid;
        }
        else
        {
            assert(mInteriorPathgridNode == NULL);
            mInteriorPathgridNode = cellPathGrid;
        }
    }
    else
    {
        if (!(store->cell->data.flags & ESM::Cell::Interior))
        {
            ExteriorPathgridNodes::iterator it =
                    mExteriorPathgridNodes.find(std::make_pair(store->cell->data.gridX, store->cell->data.gridY));
            if (it != mExteriorPathgridNodes.end())
            {
                destroyCellPathgridNode(it->second);
                mExteriorPathgridNodes.erase(it);
            }
        }
        else
        {
            if (mInteriorPathgridNode)
            {
                destroyCellPathgridNode(mInteriorPathgridNode);
                mInteriorPathgridNode = NULL;
            }
        }
    }
}

void Debugging::destroyCellPathgridNode(SceneNode *node)
{
    mPathGridRoot->removeChild(node);

    SceneNode::ChildNodeIterator childIt = node->getChildIterator();
    while (childIt.hasMoreElements())
    {
        SceneNode *child = static_cast<SceneNode *>(childIt.getNext());
        SceneNode::ObjectIterator objIt = child->getAttachedObjectIterator();
        while (objIt.hasMoreElements())
        {
            MovableObject *mesh = static_cast<MovableObject *>(objIt.getNext());
            child->getCreator()->destroyMovableObject(mesh);
        }
    }
    node->removeAndDestroyAllChildren();
    mSceneMgr->destroySceneNode(node);
}
