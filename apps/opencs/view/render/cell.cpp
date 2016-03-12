#include "cell.hpp"

#include <osg/Group>

#include <components/misc/stringops.hpp>
#include <components/esm/loadcell.hpp>
#include <components/esm/loadland.hpp>

#include "../../model/world/idtable.hpp"
#include "../../model/world/columns.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/refcollection.hpp"
#include "../../model/world/cellcoordinates.hpp"

#include "mask.hpp"
#include "terrainstorage.hpp"

bool CSVRender::Cell::removeObject (const std::string& id)
{
    std::map<std::string, Object *>::iterator iter =
        mObjects.find (Misc::StringUtils::lowerCase (id));

    if (iter==mObjects.end())
        return false;

    removeObject (iter);
    return true;
}

std::map<std::string, CSVRender::Object *>::iterator CSVRender::Cell::removeObject (
    std::map<std::string, Object *>::iterator iter)
{
    delete iter->second;
    mObjects.erase (iter++);
    return iter;
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

            std::auto_ptr<Object> object (new Object (mData, mCellNode, id, false));

            if (mSubModeElementMask & Mask_Reference)
                object->setSubMode (mSubMode);

            mObjects.insert (std::make_pair (id, object.release()));
            modified = true;
        }
    }

    return modified;
}

CSVRender::Cell::Cell (CSMWorld::Data& data, osg::Group* rootNode, const std::string& id,
    bool deleted)
: mData (data), mId (Misc::StringUtils::lowerCase (id)), mDeleted (deleted), mSubMode (0),
  mSubModeElementMask (0)
{
    std::pair<CSMWorld::CellCoordinates, bool> result = CSMWorld::CellCoordinates::fromId (id);

    if (result.second)
        mCoordinates = result.first;

    mCellNode = new osg::Group;
    rootNode->addChild(mCellNode);

    setCellMarker();

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
                mTerrain.reset(new Terrain::TerrainGrid(mCellNode, data.getResourceSystem().get(), NULL, new TerrainStorage(mData), Mask_Terrain));
                mTerrain->loadCell(esmLand.mX,
                                   esmLand.mY);

                mCellBorder.reset(new CellBorder(mCellNode, mCoordinates));
                mCellBorder->buildShape(esmLand);
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

    std::map<std::string, Object *>::iterator iter = mObjects.begin();
    while (iter!=mObjects.end())
    {
        if (iter->second->referenceDataChanged (topLeft, bottomRight))
            modified = true;

        std::map<std::string, bool>::iterator iter2 = ids.find (iter->first);

        if (iter2!=ids.end())
        {
            bool deleted = iter2->second;
            ids.erase (iter2);

            if (deleted)
            {
                iter = removeObject (iter);
                modified = true;
                continue;
            }
        }

        ++iter;
    }

    // add new objects
    for (std::map<std::string, bool>::iterator iter (ids.begin()); iter!=ids.end(); ++iter)
    {
        if (!iter->second)
        {
            mObjects.insert (std::make_pair (
                iter->first, new Object (mData, mCellNode, iter->first, false)));

            modified = true;
        }
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
    if (elementMask & Mask_Reference)
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

void CSVRender::Cell::selectAllWithSameParentId (int elementMask)
{
    std::set<std::string> ids;

    for (std::map<std::string, Object *>::const_iterator iter (mObjects.begin());
        iter!=mObjects.end(); ++iter)
    {
        if (iter->second->getSelected())
            ids.insert (iter->second->getReferenceableId());
    }

    for (std::map<std::string, Object *>::const_iterator iter (mObjects.begin());
        iter!=mObjects.end(); ++iter)
    {
        if (!iter->second->getSelected() &&
            ids.find (iter->second->getReferenceableId())!=ids.end())
        {
            iter->second->setSelected (true);
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

void CSVRender::Cell::setCellMarker()
{
    bool cellExists = false;
    bool isInteriorCell = false;

    int cellIndex = mData.getCells().searchId(mId);
    if (cellIndex > -1)
    {
        const CSMWorld::Record<CSMWorld::Cell>& cellRecord = mData.getCells().getRecord(cellIndex);
        cellExists = !cellRecord.isDeleted();
        isInteriorCell = cellRecord.get().mData.mFlags & ESM::Cell::Interior;
    }

    if (!isInteriorCell) {
        mCellMarker.reset(new CellMarker(mCellNode, mCoordinates, cellExists));
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

std::vector<osg::ref_ptr<CSVRender::TagBase> > CSVRender::Cell::getSelection (unsigned int elementMask) const
{
    std::vector<osg::ref_ptr<TagBase> > result;

    if (elementMask & Mask_Reference)
        for (std::map<std::string, Object *>::const_iterator iter (mObjects.begin());
            iter!=mObjects.end(); ++iter)
            if (iter->second->getSelected())
                result.push_back (iter->second->getTag());

    return result;
}

std::vector<osg::ref_ptr<CSVRender::TagBase> > CSVRender::Cell::getEdited (unsigned int elementMask) const
{
    std::vector<osg::ref_ptr<TagBase> > result;

    if (elementMask & Mask_Reference)
        for (std::map<std::string, Object *>::const_iterator iter (mObjects.begin());
            iter!=mObjects.end(); ++iter)
            if (iter->second->isEdited())
                result.push_back (iter->second->getTag());

    return result;
}

void CSVRender::Cell::setSubMode (int subMode, unsigned int elementMask)
{
    mSubMode = subMode;
    mSubModeElementMask = elementMask;

    if (elementMask & Mask_Reference)
        for (std::map<std::string, Object *>::const_iterator iter (mObjects.begin());
            iter!=mObjects.end(); ++iter)
                iter->second->setSubMode (subMode);
}

void CSVRender::Cell::reset (unsigned int elementMask)
{
    if (elementMask & Mask_Reference)
        for (std::map<std::string, Object *>::const_iterator iter (mObjects.begin());
            iter!=mObjects.end(); ++iter)
            iter->second->reset();
}
