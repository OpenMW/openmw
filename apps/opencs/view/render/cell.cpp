#include "cell.hpp"

#include <osg/Group>

#include <components/misc/stringops.hpp>
#include <components/esm/loadland.hpp>

#include "../../model/world/idtable.hpp"
#include "../../model/world/columns.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/refcollection.hpp"
#include "../../model/world/cellcoordinates.hpp"

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

            mObjects.insert (std::make_pair (id, new Object (mData, mCellNode, id, false)));
            modified = true;
        }
    }

    return modified;
}

CSVRender::Cell::Cell (CSMWorld::Data& data, osg::Group* rootNode, const std::string& id,
    bool deleted)
: mData (data), mId (Misc::StringUtils::lowerCase (id)), mDeleted (deleted)
{
    std::pair<CSMWorld::CellCoordinates, bool> result = CSMWorld::CellCoordinates::fromId (id);

    if (result.second)
        mCoordinates = result.first;

    mCellNode = new osg::Group;
    rootNode->addChild(mCellNode);

    if (!mDeleted)
    {
        CSMWorld::IdTable& references = dynamic_cast<CSMWorld::IdTable&> (
            *mData.getTableModel (CSMWorld::UniversalId::Type_References));

        int rows = references.rowCount();

        addObjects (0, rows-1);

        const CSMWorld::IdCollection<CSMWorld::Land>& land = mData.getLand();
        int landIndex = land.searchId(mId);
        if (landIndex != -1)
        {
            const ESM::Land& esmLand = land.getRecord(mId).get();

            if (esmLand.getLandData (ESM::Land::DATA_VHGT))
            {
                mTerrain.reset(new Terrain::TerrainGrid(mCellNode, data.getResourceSystem().get(), NULL, new TerrainStorage(mData), Element_Terrain<<1));
                mTerrain->loadCell(esmLand.mX,
                                   esmLand.mY);
            }
        }
    }
}

CSVRender::Cell::~Cell()
{
    for (std::map<std::string, Object *>::iterator iter (mObjects.begin());
        iter!=mObjects.end(); ++iter)
        delete iter->second;

    mCellNode->getParent(0)->removeChild(mCellNode);
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
    if (mDeleted)
        return false;

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
            iter->first, new Object (mData, mCellNode, iter->first, false)));

        modified = true;
    }

    return modified;
}

bool CSVRender::Cell::referenceAboutToBeRemoved (const QModelIndex& parent, int start,
    int end)
{
    if (parent.isValid())
        return false;

    if (mDeleted)
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

    if (mDeleted)
        return false;

    return addObjects (start, end);
}

void CSVRender::Cell::setSelection (int elementMask, Selection mode)
{
    if (elementMask & Element_Reference)
    {
        for (std::map<std::string, Object *>::const_iterator iter (mObjects.begin());
            iter!=mObjects.end(); ++iter)
        {
            bool selected = false;

            switch (mode)
            {
                case Selection_Clear: selected = false; break;
                case Selection_All: selected = true; break;
                case Selection_Invert: selected = !iter->second->getSelected(); break;
            }

            iter->second->setSelected (selected);
        }
    }
}

void CSVRender::Cell::setCellArrows (int mask)
{
    for (int i=0; i<4; ++i)
    {
        CellArrow::Direction direction = static_cast<CellArrow::Direction> (1<<i);

        bool enable = mask & direction;

        if (enable!=(mCellArrows[i].get()!=0))
        {
            if (enable)
                mCellArrows[i].reset (new CellArrow (mCellNode, direction, mCoordinates));
            else
                mCellArrows[i].reset (0);
        }
    }
}

CSMWorld::CellCoordinates CSVRender::Cell::getCoordinates() const
{
    return mCoordinates;
}

bool CSVRender::Cell::isDeleted() const
{
    return mDeleted;
}
