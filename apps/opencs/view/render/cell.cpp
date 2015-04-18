
#include "cell.hpp"

#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreManualObject.h>

#include <components/misc/stringops.hpp>
#include <components/esm/loadland.hpp>

#include "../../model/world/idtable.hpp"
#include "../../model/world/columns.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/refcollection.hpp"
#include "../../model/world/pathgrid.hpp"
#include "../world/physicssystem.hpp"

#include "elements.hpp"
#include "terrainstorage.hpp"
#include "pathgridpoint.hpp"

namespace CSVRender
{
    // PLEASE NOTE: pathgrid edge code copied and adapted from mwrender/debugging
    static const std::string PG_LINE_MATERIAL = "pathgridLineMaterial";
    static const int POINT_MESH_BASE = 35;
    static const std::string DEBUGGING_GROUP = "debugging";
}

void CSVRender::Cell::createGridMaterials()
{
    if(!Ogre::ResourceGroupManager::getSingleton().resourceGroupExists(DEBUGGING_GROUP))
        Ogre::ResourceGroupManager::getSingleton().createResourceGroup(DEBUGGING_GROUP);

    if(Ogre::MaterialManager::getSingleton().getByName(PG_LINE_MATERIAL, DEBUGGING_GROUP).isNull())
    {
        Ogre::MaterialPtr lineMatPtr =
            Ogre::MaterialManager::getSingleton().create(PG_LINE_MATERIAL, DEBUGGING_GROUP);
        lineMatPtr->setReceiveShadows(false);
        lineMatPtr->getTechnique(0)->setLightingEnabled(true);
        lineMatPtr->getTechnique(0)->getPass(0)->setDiffuse(1,1,0,0);
        lineMatPtr->getTechnique(0)->getPass(0)->setAmbient(1,1,0);
        lineMatPtr->getTechnique(0)->getPass(0)->setSelfIllumination(1,1,0);
    }
}

void CSVRender::Cell::destroyGridMaterials()
{
    if(Ogre::ResourceGroupManager::getSingleton().resourceGroupExists(DEBUGGING_GROUP))
    {
        if(!Ogre::MaterialManager::getSingleton().getByName(PG_LINE_MATERIAL, DEBUGGING_GROUP).isNull())
            Ogre::MaterialManager::getSingleton().remove(PG_LINE_MATERIAL);

        Ogre::ResourceGroupManager::getSingleton().destroyResourceGroup(DEBUGGING_GROUP);
    }
}

Ogre::ManualObject *CSVRender::Cell::createPathgridEdge(const std::string &name,
        const Ogre::Vector3 &start, const Ogre::Vector3 &end)
{
    Ogre::ManualObject *result = mSceneMgr->createManualObject(name);

    createGridMaterials();
    result->begin(PG_LINE_MATERIAL, Ogre::RenderOperation::OT_LINE_LIST);

    Ogre::Vector3 direction = (end - start);
    Ogre::Vector3 lineDisplacement = direction.crossProduct(Ogre::Vector3::UNIT_Z).normalisedCopy();
    // move lines up a little, so they will be less covered by meshes/landscape
    lineDisplacement = lineDisplacement * POINT_MESH_BASE + Ogre::Vector3(0, 0, 10);
    result->position(start + lineDisplacement);
    result->position(end + lineDisplacement);

    result->end();

    return result;
}

bool CSVRender::Cell::removeObject (const std::string& id)
{
    std::map<std::string, Object *>::iterator iter =
        mObjects.find (Misc::StringUtils::lowerCase (id));

    if (iter==mObjects.end())
        return false;

    delete iter->second;
    mObjects.erase (iter);
    return true;
}

bool CSVRender::Cell::addObjects (int start, int end)
{
    bool modified = false;

    const CSMWorld::RefCollection& collection = mData.getReferences();

    for (int i=start; i<=end; ++i)
    {
        std::string cell = Misc::StringUtils::lowerCase (collection.getRecord (i).get().mCell);

        CSMWorld::RecordBase::State state = collection.getRecord (i).mState;

        if (cell==mId && state!=CSMWorld::RecordBase::State_Deleted)
        {
            std::string id = Misc::StringUtils::lowerCase (collection.getRecord (i).get().mId);

            mObjects.insert (std::make_pair (id, new Object (mData, mCellNode, id, false, mPhysics)));
            modified = true;
        }
    }

    return modified;
}

CSVRender::Cell::Cell (CSMWorld::Data& data, Ogre::SceneManager *sceneManager,
    const std::string& id, boost::shared_ptr<CSVWorld::PhysicsSystem> physics, const Ogre::Vector3& origin)
: mData (data), mId (Misc::StringUtils::lowerCase (id)), mSceneMgr(sceneManager), mPhysics(physics), mX(0), mY(0),
    mPathgridId("")
{
    mCellNode = sceneManager->getRootSceneNode()->createChildSceneNode();
    mCellNode->setPosition (origin);

    CSMWorld::IdTable& references = dynamic_cast<CSMWorld::IdTable&> (
        *mData.getTableModel (CSMWorld::UniversalId::Type_References));

    int rows = references.rowCount();

    addObjects (0, rows-1);

    const CSMWorld::IdCollection<CSMWorld::Land>& land = mData.getLand();
    int landIndex = land.searchId(mId);
    if (landIndex != -1)
    {
        const ESM::Land* esmLand = land.getRecord(mId).get().mLand.get();
        if(esmLand && esmLand->mDataTypes&ESM::Land::DATA_VHGT)
        {
            mTerrain.reset(new Terrain::TerrainGrid(sceneManager, new TerrainStorage(mData), Element_Terrain, true,
                                                    Terrain::Align_XY));
            mTerrain->loadCell(esmLand->mX,
                               esmLand->mY);

            float verts = ESM::Land::LAND_SIZE;
            float worldsize = ESM::Land::REAL_SIZE;
            mX = esmLand->mX;
            mY = esmLand->mY;
            mPhysics->addHeightField(sceneManager,
                    esmLand->mLandData->mHeights, mX, mY, 0, worldsize / (verts-1), verts);
        }
    }

    loadPathgrid();
}

CSVRender::Cell::~Cell()
{
    // destroy manual objects
    for(std::map<std::pair<int, int>, std::string>::iterator iter = mPgEdges.begin();
        iter != mPgEdges.end(); ++iter)
    {
        if(mSceneMgr->hasManualObject((*iter).second))
        {
            Ogre::ManualObject *manual = mSceneMgr->getManualObject((*iter).second);
            Ogre::SceneNode *node = manual->getParentSceneNode();
            mSceneMgr->destroyManualObject((*iter).second);
            if(mSceneMgr->hasSceneNode(node->getName()))
                mSceneMgr->destroySceneNode(node);
        }
    }
    destroyGridMaterials();

    for(std::map<std::string, PathgridPoint *>::iterator iter (mPgPoints.begin());
        iter!=mPgPoints.end(); ++iter)
    {
        delete iter->second;
    }

    if (mTerrain.get())
        mPhysics->removeHeightField(mSceneMgr, mX, mY);

    for (std::map<std::string, Object *>::iterator iter (mObjects.begin());
        iter!=mObjects.end(); ++iter)
        delete iter->second;

    mCellNode->getCreator()->destroySceneNode (mCellNode);
}

bool CSVRender::Cell::referenceableDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    bool modified = false;

    for (std::map<std::string, Object *>::iterator iter (mObjects.begin());
        iter!=mObjects.end(); ++iter)
        if (iter->second->referenceableDataChanged (topLeft, bottomRight))
            modified = true;

    return modified;
}

bool CSVRender::Cell::referenceableAboutToBeRemoved (const QModelIndex& parent, int start,
    int end)
{
    if (parent.isValid())
        return false;

    bool modified = false;

    for (std::map<std::string, Object *>::iterator iter (mObjects.begin());
        iter!=mObjects.end(); ++iter)
        if (iter->second->referenceableAboutToBeRemoved (parent, start, end))
            modified = true;

    return modified;
}

bool CSVRender::Cell::referenceDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    CSMWorld::IdTable& references = dynamic_cast<CSMWorld::IdTable&> (
        *mData.getTableModel (CSMWorld::UniversalId::Type_References));

    int idColumn = references.findColumnIndex (CSMWorld::Columns::ColumnId_Id);
    int cellColumn = references.findColumnIndex (CSMWorld::Columns::ColumnId_Cell);
    int stateColumn = references.findColumnIndex (CSMWorld::Columns::ColumnId_Modification);

    // list IDs in cell
    std::map<std::string, bool> ids; // id, deleted state

    for (int i=topLeft.row(); i<=bottomRight.row(); ++i)
    {
        std::string cell = Misc::StringUtils::lowerCase (references.data (
            references.index (i, cellColumn)).toString().toUtf8().constData());

        if (cell==mId)
        {
            std::string id = Misc::StringUtils::lowerCase (references.data (
                references.index (i, idColumn)).toString().toUtf8().constData());

            int state = references.data (references.index (i, stateColumn)).toInt();

            ids.insert (std::make_pair (id, state==CSMWorld::RecordBase::State_Deleted));
        }
    }

    // perform update and remove where needed
    bool modified = false;

    for (std::map<std::string, Object *>::iterator iter (mObjects.begin());
        iter!=mObjects.end(); ++iter)
    {
        if (iter->second->referenceDataChanged (topLeft, bottomRight))
            modified = true;

        std::map<std::string, bool>::iterator iter2 = ids.find (iter->first);

        if (iter2!=ids.end())
        {
            if (iter2->second)
            {
                removeObject (iter->first);
                modified = true;
            }

            ids.erase (iter2);
        }
    }

    // add new objects
    for (std::map<std::string, bool>::iterator iter (ids.begin()); iter!=ids.end(); ++iter)
    {
        mObjects.insert (std::make_pair (
            iter->first, new Object (mData, mCellNode, iter->first, false, mPhysics)));

        modified = true;
    }

    return modified;
}

bool CSVRender::Cell::referenceAboutToBeRemoved (const QModelIndex& parent, int start,
    int end)
{
    if (parent.isValid())
        return false;

    CSMWorld::IdTable& references = dynamic_cast<CSMWorld::IdTable&> (
        *mData.getTableModel (CSMWorld::UniversalId::Type_References));

    int idColumn = references.findColumnIndex (CSMWorld::Columns::ColumnId_Id);

    bool modified = false;

    for (int row = start; row<=end; ++row)
        if (removeObject (references.data (
            references.index (row, idColumn)).toString().toUtf8().constData()))
            modified = true;

    return modified;
}

bool CSVRender::Cell::referenceAdded (const QModelIndex& parent, int start, int end)
{
    if (parent.isValid())
        return false;

    return addObjects (start, end);
}

float CSVRender::Cell::getTerrainHeightAt(const Ogre::Vector3 &pos) const
{
    if(mTerrain.get() != NULL)
        return mTerrain->getHeightAt(pos);
    else
        return -std::numeric_limits<float>::max();
}

// FIXME:
//  - updating indicies, including mData
//  - adding edges (need the ability to select a pathgrid and highlight)
//  - save to document & signals
//  - repainting edges while moving
void CSVRender::Cell::loadPathgrid()
{
    int worldsize = ESM::Land::REAL_SIZE;

    CSMWorld::SubCellCollection<CSMWorld::Pathgrid>& pathgridCollection = mData.getPathgrids();
    int index = pathgridCollection.searchId(mId);
    if(index != -1)
    {
        const CSMWorld::Pathgrid &pathgrid = pathgridCollection.getRecord(index).get();
        mPathgridId = pathgrid.mId; // FIXME: temporary storage (should be document)

        mPoints.resize(pathgrid.mPoints.size());
        std::vector<ESM::Pathgrid::Point>::const_iterator iter = pathgrid.mPoints.begin();
        for(index = 0; iter != pathgrid.mPoints.end(); ++iter, ++index)
        {
            std::string name = PathgridPoint::getName(pathgrid.mId, index);

            Ogre::Vector3 pos =
                Ogre::Vector3(worldsize*mX+(*iter).mX, worldsize*mY+(*iter).mY, (*iter).mZ);

            mPgPoints.insert(std::make_pair(name, new PathgridPoint(name, mCellNode, pos, mPhysics)));
            mPoints[index] = *iter; // FIXME: temporary storage (should be document)
        }

        for(ESM::Pathgrid::EdgeList::const_iterator it = pathgrid.mEdges.begin();
            it != pathgrid.mEdges.end();
            ++it)
        {
            Ogre::SceneNode *node = mCellNode->createChildSceneNode();
            const ESM::Pathgrid::Edge &edge = *it;
            const ESM::Pathgrid::Point &p0 = pathgrid.mPoints[edge.mV0];
            const ESM::Pathgrid::Point &p1 = pathgrid.mPoints[edge.mV1];

            std::ostringstream stream;
            stream << pathgrid.mId << "_" << edge.mV0 << " " << edge.mV1;
            std::string name = stream.str();

            Ogre::ManualObject *line = createPathgridEdge(name,
                Ogre::Vector3(worldsize*mX+p0.mX, worldsize*mY+p0.mY, p0.mZ),
                Ogre::Vector3(worldsize*mX+p1.mX, worldsize*mY+p1.mY, p1.mZ));
            line->setVisibilityFlags(Element_Pathgrid);
            node->attachObject(line);

            mPgEdges.insert(std::make_pair(std::make_pair(edge.mV0, edge.mV1), name));
            mEdges.push_back(*it); // FIXME: temporary storage (should be document)
        }
    }
}

// NOTE: pos is in world coordinates
// FIXME: save to the document
void CSVRender::Cell::pathgridPointAdded(const Ogre::Vector3 &pos, bool interior)
{
    std::string name = PathgridPoint::getName(mId, mPoints.size());

    mPgPoints.insert(std::make_pair(name, new PathgridPoint(name, mCellNode, pos, mPhysics)));

    // store to document
    int worldsize = ESM::Land::REAL_SIZE;

    int x = pos.x;
    int y = pos.y;
    if(!interior)
    {
        x = x - (worldsize * mX);
        y = y - (worldsize * mY);
    }

    ESM::Pathgrid::Point point(x, y, (int)pos.z);
    point.mConnectionNum = 0;
    mPoints.push_back(point);
    mPathgridId = mId;
    // FIXME: update other scene managers
}

// FIXME: save to the document
void CSVRender::Cell::pathgridPointRemoved(const std::string &name)
{
    std::pair<std::string, int> result = PathgridPoint::getIdAndIndex(name);
    if(result.first == "")
        return;

    std::string pathgridId = result.first;
    int index = result.second;

    // check if the point exists
    if(index < 0 || mPathgridId != pathgridId || (unsigned int)index >= mPoints.size())
        return;

    int numToDelete = mPoints[index].mConnectionNum * 2; // for sanity check later
    int edgeCount = 0;

    // find edges to delete
    std::vector<std::pair<int, int> > edges;
    for(unsigned i = 0; i < mEdges.size(); ++i)
    {
        if(mEdges[i].mV0 == index || mEdges[i].mV1 == index)
        {
            for(std::map<std::pair<int, int>, std::string>::iterator iter = mPgEdges.begin();
                iter != mPgEdges.end(); ++iter)
            {
                if((*iter).first.first == index || (*iter).first.second == index)
                {
                    edges.push_back(std::make_pair((*iter).first.first, (*iter).first.second));
                }
            }
        }
    }

    // delete the edges
    for(std::vector<std::pair<int, int> >::iterator iter = edges.begin();
        iter != edges.end(); ++iter)
    {
        std::string name = mPgEdges[*iter];
        if(mSceneMgr->hasManualObject(name))
        {
            // remove manual objects
            Ogre::ManualObject *manual = mSceneMgr->getManualObject(name);
            Ogre::SceneNode *node = manual->getParentSceneNode();
            mSceneMgr->destroyManualObject(name);
            if(mSceneMgr->hasSceneNode(node->getName()))
                mSceneMgr->destroySceneNode(node);

            edgeCount++; // for sanity check later

            // update map
            mPgEdges.erase(*iter);

            // update document
            assert(mPoints[(*iter).first].mConnectionNum > 0);
            mPoints[(*iter).first].mConnectionNum -= 1;
            for(unsigned i = mEdges.size() - 1; i > 0; --i)
            {
                if(mEdges[i].mV0 == index || mEdges[i].mV1 == index)
                    mEdges.erase(mEdges.begin() + i);
            }
        }
    }

    if(edgeCount != numToDelete)
    {
        // WARNING: continue anyway?  Or should this be an exception?
        std::cerr << "The no of edges del does not match the no of conn for: "
            << mPathgridId + "_" + QString::number(index).toStdString() << std::endl;
    }

    if(edgeCount || mPoints[index].mConnectionNum == 0)
    {
        // remove the point
        delete mPgPoints[name];
        mPgPoints.erase(name);
        // FIXME: update other scene managers
    }

    // store to document
    //mPoints.erase(mPoints.begin() + index);  // WARNING: Can't erase because the index will change
    // FIXME: it should be possible to refresh indicies but that means index values
    // can't be stored in maps, names, etc
}

// NOTE: newPos is in world coordinates
void CSVRender::Cell::pathgridPointMoved(const std::string &name,
        const Ogre::Vector3 &newPos, bool interior)
{
    std::pair<std::string, int> result = PathgridPoint::getIdAndIndex(name);
    if(result.first == "")
        return;

    std::string pathgridId = result.first;
    int index = result.second;

    // check if the point exists
    if(index < 0 || mPathgridId != pathgridId || (unsigned int)index >= mPoints.size())
        return;

    int worldsize = ESM::Land::REAL_SIZE;

    int x = newPos.x;
    int y = newPos.y;
    if(!interior)
    {
        x = x - (worldsize * mX);
        y = y - (worldsize * mY);
    }

    mPoints[index].mX = x;
    mPoints[index].mY = y;
    mPoints[index].mZ = newPos.z;

    // delete then recreate the edges
    for(unsigned i = 0; i < mEdges.size(); ++i)
    {
        if(mEdges[i].mV0 == index || mEdges[i].mV1 == index)
        {
            std::ostringstream stream;
            stream << pathgridId << "_" << mEdges[i].mV0 << " " << mEdges[i].mV1;
            std::string name = stream.str();

            if(mSceneMgr->hasManualObject(name))
            {
                // remove manual objects
                Ogre::ManualObject *manual = mSceneMgr->getManualObject(name);
                Ogre::SceneNode *node = manual->getParentSceneNode();
                mSceneMgr->destroyManualObject(name);

                if(mEdges[i].mV0 == index)
                {
                    const ESM::Pathgrid::Point &p1 = mPoints[mEdges[i].mV1];

                    Ogre::ManualObject *line = createPathgridEdge(name,
                        newPos,
                        Ogre::Vector3(worldsize*mX+p1.mX, worldsize*mY+p1.mY, p1.mZ));
                    line->setVisibilityFlags(Element_Pathgrid);
                    node->attachObject(line);
                }
                else if(mEdges[i].mV1 == index)
                {
                    const ESM::Pathgrid::Point &p0 = mPoints[mEdges[i].mV0];

                    Ogre::ManualObject *line = createPathgridEdge(name,
                        Ogre::Vector3(worldsize*mX+p0.mX, worldsize*mY+p0.mY, p0.mZ),
                        newPos);
                    line->setVisibilityFlags(Element_Pathgrid);
                    node->attachObject(line);
                }
            }
        }
    }
}

// FIXME: save to the document
void CSVRender::Cell::addPathgridEdge()
{
    // check if the points exist
    // update the edges
    // store to document
    // FIXME: update other scene managers
}

// FIXME: save to the document
void CSVRender::Cell::removePathgridEdge()
{
}
