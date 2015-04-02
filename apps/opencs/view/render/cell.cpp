
#include "cell.hpp"

#include <OgreSceneManager.h>
#include <OgreSceneNode.h>

#include <components/misc/stringops.hpp>
#include <components/esm/loadland.hpp>

#include "../../model/world/idtable.hpp"
#include "../../model/world/columns.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/refcollection.hpp"
#include "../world/physicssystem.hpp"

#include "elements.hpp"
#include "terrainstorage.hpp"

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
: mData (data), mId (Misc::StringUtils::lowerCase (id)), mSceneMgr(sceneManager), mPhysics(physics), mX(0), mY(0)
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
}

CSVRender::Cell::~Cell()
{
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
