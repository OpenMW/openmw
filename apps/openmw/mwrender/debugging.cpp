#include "debugging.hpp"

#include <cassert>

#include <OgreNode.h>
#include <OgreSceneManager.h>
#include <OgreMaterial.h>
#include <OgreMaterialManager.h>
#include <OgreManualObject.h>

#include <components/esm/loadstat.hpp>
#include <components/esm/loadpgrd.hpp>

#include <components/esm_store/store.hpp>

#include "../mwbase/world.hpp" // these includes can be removed once the static-hack is gone
#include "../mwbase/environment.hpp"

#include "../mwworld/ptr.hpp"

#include "player.hpp"

using namespace Ogre;

namespace MWRender
{

static const std::string PATHGRID_POINT_MATERIAL = "pathgridPointMaterial";
static const std::string PATHGRID_LINE_MATERIAL = "pathgridLineMaterial";
static const std::string DEBUGGING_GROUP = "debugging";
static const int POINT_MESH_BASE = 35;

void Debugging::createGridMaterials()
{
    if (mGridMatsCreated) return;

    if (MaterialManager::getSingleton().getByName(PATHGRID_LINE_MATERIAL, DEBUGGING_GROUP).isNull())
    {
        MaterialPtr lineMatPtr = MaterialManager::getSingleton().create(PATHGRID_LINE_MATERIAL, DEBUGGING_GROUP);
        lineMatPtr->setReceiveShadows(false);
        lineMatPtr->getTechnique(0)->setLightingEnabled(true);
        lineMatPtr->getTechnique(0)->getPass(0)->setDiffuse(1,1,0,0);
        lineMatPtr->getTechnique(0)->getPass(0)->setAmbient(1,1,0);
        lineMatPtr->getTechnique(0)->getPass(0)->setSelfIllumination(1,1,0);
    }

    if (MaterialManager::getSingleton().getByName(PATHGRID_POINT_MATERIAL, DEBUGGING_GROUP).isNull())
    {
        MaterialPtr pointMatPtr = MaterialManager::getSingleton().create(PATHGRID_POINT_MATERIAL, DEBUGGING_GROUP);
        pointMatPtr->setReceiveShadows(false);
        pointMatPtr->getTechnique(0)->setLightingEnabled(true);
        pointMatPtr->getTechnique(0)->getPass(0)->setDiffuse(1,0,0,0);
        pointMatPtr->getTechnique(0)->getPass(0)->setAmbient(1,0,0);
        pointMatPtr->getTechnique(0)->getPass(0)->setSelfIllumination(1,0,0);
    }
    mGridMatsCreated = true;
}

void Debugging::destroyGridMaterials()
{
    if (mGridMatsCreated)
    {
        MaterialManager::getSingleton().remove(PATHGRID_POINT_MATERIAL);
        MaterialManager::getSingleton().remove(PATHGRID_LINE_MATERIAL);
        mGridMatsCreated = false;
    }
}

ManualObject *Debugging::createPathgridLines(const ESM::Pathgrid *pathgrid)
{
    ManualObject *result = mSceneMgr->createManualObject();

    result->begin(PATHGRID_LINE_MATERIAL, RenderOperation::OT_LINE_LIST);
    for(ESM::Pathgrid::EdgeList::const_iterator it = pathgrid->edges.begin();
        it != pathgrid->edges.end();
        ++it)
    {
        const ESM::Pathgrid::Edge &edge = *it;
        const ESM::Pathgrid::Point &p1 = pathgrid->points[edge.v0], &p2 = pathgrid->points[edge.v1];
        Vector3 direction = (Vector3(p2.x, p2.y, p2.z) - Vector3(p1.x, p1.y, p1.z));
        Vector3 lineDisplacement = direction.crossProduct(Vector3::UNIT_Z).normalisedCopy();
        lineDisplacement = lineDisplacement * POINT_MESH_BASE +
                                Vector3(0, 0, 10); // move lines up a little, so they will be less covered by meshes/landscape
        result->position(Vector3(p1.x, p1.y, p1.z) + lineDisplacement);
        result->position(Vector3(p2.x, p2.y, p2.z) + lineDisplacement);
    }
    result->end();

    return result;
}

ManualObject *Debugging::createPathgridPoints(const ESM::Pathgrid *pathgrid)
{
    ManualObject *result = mSceneMgr->createManualObject();
    const float height = POINT_MESH_BASE * sqrtf(2);

    result->begin(PATHGRID_POINT_MATERIAL, RenderOperation::OT_TRIANGLE_STRIP);

    bool first = true;
    uint32 startIndex = 0;
    for(ESM::Pathgrid::PointList::const_iterator it = pathgrid->points.begin();
        it != pathgrid->points.end();
        it++, startIndex += 6)
    {
        Vector3 pointPos(it->x, it->y, it->z);

        if (!first)
        {
            // degenerate triangle from previous octahedron
            result->index(startIndex - 4); // 2nd point of previous octahedron
            result->index(startIndex); // start point of current octahedron
        }

        result->position(pointPos + Vector3(0, 0, height)); // 0
        result->position(pointPos + Vector3(-POINT_MESH_BASE, -POINT_MESH_BASE, 0)); // 1
        result->position(pointPos + Vector3(POINT_MESH_BASE, -POINT_MESH_BASE, 0)); // 2
        result->position(pointPos + Vector3(POINT_MESH_BASE, POINT_MESH_BASE, 0)); // 3
        result->position(pointPos + Vector3(-POINT_MESH_BASE, POINT_MESH_BASE, 0)); // 4
        result->position(pointPos + Vector3(0, 0, -height)); // 5

        result->index(startIndex + 0);
        result->index(startIndex + 1);
        result->index(startIndex + 2);
        result->index(startIndex + 5);
        result->index(startIndex + 3);
        result->index(startIndex + 4);
        // degenerates
        result->index(startIndex + 4);
        result->index(startIndex + 5);
        result->index(startIndex + 5);
        // end degenerates
        result->index(startIndex + 1);
        result->index(startIndex + 4);
        result->index(startIndex + 0);
        result->index(startIndex + 3);
        result->index(startIndex + 2);

        first = false;
    }

    result->end();

    return result;
}

Debugging::Debugging(SceneNode *mwRoot, OEngine::Physic::PhysicEngine *engine) :
    mMwRoot(mwRoot), mEngine(engine),
    mSceneMgr(mwRoot->getCreator()),
    mPathgridEnabled(false),
    mInteriorPathgridNode(NULL), mPathGridRoot(NULL),
    mGridMatsCreated(false)
{
    ResourceGroupManager::getSingleton().createResourceGroup(DEBUGGING_GROUP);
}

Debugging::~Debugging()
{
    if (mPathgridEnabled)
    {
        togglePathgrid();
    }

    ResourceGroupManager::getSingleton().destroyResourceGroup(DEBUGGING_GROUP);
}


bool Debugging::toggleRenderMode (int mode){
    switch (mode)
    {
        case MWBase::World::Render_CollisionDebug:

            return mEngine->toggleDebugRendering();

        case MWBase::World::Render_Pathgrid:
            togglePathgrid();
            return mPathgridEnabled;
    }

    return false;
}

void Debugging::cellAdded(MWWorld::Ptr::CellStore *store)
{
    mActiveCells.push_back(store);
    if (mPathgridEnabled)
        enableCellPathgrid(store);
}

void Debugging::cellRemoved(MWWorld::Ptr::CellStore *store)
{
    mActiveCells.erase(std::remove(mActiveCells.begin(), mActiveCells.end(), store), mActiveCells.end());
    if (mPathgridEnabled)
        disableCellPathgrid(store);
}

void Debugging::togglePathgrid()
{
    mPathgridEnabled = !mPathgridEnabled;
    if (mPathgridEnabled)
    {
        createGridMaterials();

        // add path grid meshes to already loaded cells
        mPathGridRoot = mMwRoot->createChildSceneNode();
        for(CellList::iterator it = mActiveCells.begin(); it != mActiveCells.end(); ++it)
        {
            enableCellPathgrid(*it);
        }
    }
    else
    {
        // remove path grid meshes from already loaded cells
        for(CellList::iterator it = mActiveCells.begin(); it != mActiveCells.end(); ++it)
        {
            disableCellPathgrid(*it);
        }
        mPathGridRoot->removeAndDestroyAllChildren();
        mSceneMgr->destroySceneNode(mPathGridRoot);
        mPathGridRoot = NULL;
        destroyGridMaterials();
    }
}

void Debugging::enableCellPathgrid(MWWorld::Ptr::CellStore *store)
{
    ESM::Pathgrid *pathgrid = MWBase::Environment::get().getWorld()->getStore().pathgrids.search(*store->cell);
    if (!pathgrid) return;

    Vector3 cellPathGridPos(0, 0, 0);
    if (store->cell->isExterior())
    {
        cellPathGridPos.x = store->cell->data.gridX * ESM::Land::REAL_SIZE;
        cellPathGridPos.y = store->cell->data.gridY * ESM::Land::REAL_SIZE;
    }
    SceneNode *cellPathGrid = mPathGridRoot->createChildSceneNode(cellPathGridPos);
    cellPathGrid->attachObject(createPathgridLines(pathgrid));
    cellPathGrid->attachObject(createPathgridPoints(pathgrid));

    if (store->cell->isExterior())
    {
        mExteriorPathgridNodes[std::make_pair(store->cell->data.gridX, store->cell->data.gridY)] = cellPathGrid;
    }
    else
    {
        assert(mInteriorPathgridNode == NULL);
        mInteriorPathgridNode = cellPathGrid;
    }
}

void Debugging::disableCellPathgrid(MWWorld::Ptr::CellStore *store)
{
    if (store->cell->isExterior())
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

void Debugging::destroyCellPathgridNode(SceneNode *node)
{
    mPathGridRoot->removeChild(node);
    destroyAttachedObjects(node);
    mSceneMgr->destroySceneNode(node);
}

void Debugging::destroyAttachedObjects(SceneNode *node)
{
    SceneNode::ObjectIterator objIt = node->getAttachedObjectIterator();
    while (objIt.hasMoreElements())
    {
        MovableObject *mesh = static_cast<MovableObject *>(objIt.getNext());
        mSceneMgr->destroyMovableObject(mesh);
    }
}

}
