#include "debugging.hpp"

#include <assert.h>

#include "OgreRoot.h"
#include "OgreRenderWindow.h"
#include "OgreSceneManager.h"
#include "OgreViewport.h"
#include "OgreCamera.h"
#include "OgreTextureManager.h"

#include "../mwworld/world.hpp" // these includes can be removed once the static-hack is gone
#include "../mwworld/ptr.hpp"
#include <components/esm/loadstat.hpp>
#include <components/esm/loadpgrd.hpp>

#include "player.hpp"

using namespace MWRender;
using namespace Ogre;

Debugging::Debugging(const ESMS::ESMStore &store, SceneManager* sceneMgr, OEngine::Physic::PhysicEngine *engine) :
    mStore(store), mSceneMgr(sceneMgr), mEngine(engine), pathgridEnabled(false)
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
        mPathGridRoot = mSceneMgr->getRootSceneNode()->createChildSceneNode();
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
    }
}

void Debugging::togglePathgridForCell(MWWorld::Ptr::CellStore *store, bool enabled)
{
    ESM::Pathgrid *pathgrid = mStore.pathgrids.search(*store->cell);
    if (!pathgrid)
    {
        std::cout << "No path grid :(" << std::endl;
        return;
    }
    std::cout << "Path grid exists!" << std::endl;

    if (enabled)
    {
        Vector3 cellPathGridPos;
        if (!(store->cell->data.flags & ESM::Cell::Interior))
        {
            /// \todo Replace with ESM::Land::REAL_SIZE after merging with terrain branch
            cellPathGridPos.x = store->cell->data.gridX * 8192;
            cellPathGridPos.z = -store->cell->data.gridY * 8192;
        }
        SceneNode *cellPathGrid = mPathGridRoot->createChildSceneNode(cellPathGridPos);
        ESM::Pathgrid::PointList points = pathgrid->points;
        for (ESM::Pathgrid::PointList::iterator it = points.begin(); it != points.end(); it++)
        {
            Vector3 position(it->x, it->z, -it->y);
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
        /// \todo Don't forget to destroy cubes too!
        SceneNode *cellPathGridNode;
        if (!(store->cell->data.flags & ESM::Cell::Interior))
        {
            ExteriorPathgridNodes::iterator it =
                    mExteriorPathgridNodes.find(std::make_pair(store->cell->data.gridX, store->cell->data.gridY));
            if (it != mExteriorPathgridNodes.end())
            {
                cellPathGridNode = it->second;
                mPathGridRoot->removeChild(cellPathGridNode);
                cellPathGridNode->removeAndDestroyAllChildren();
                mSceneMgr->destroySceneNode(cellPathGridNode);
                mExteriorPathgridNodes.erase(it);
            }
        }
        else
        {
            if (mInteriorPathgridNode)
            {
                mPathGridRoot->removeChild(mInteriorPathgridNode);
                mInteriorPathgridNode->removeAndDestroyAllChildren();
                mSceneMgr->destroySceneNode(mInteriorPathgridNode);
                mInteriorPathgridNode = NULL;
            }
        }
    }
}
