
#include "unpagedworldspacewidget.hpp"

#include <sstream>

#include <OgreColourValue.h>
#include <OgreCamera.h>

#include <QtGui/qevent.h>

#include "../../model/doc/document.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/tablemimedata.hpp"

#include "../widget/scenetooltoggle.hpp"
#include "../widget/scenetooltoggle2.hpp"

#include "elements.hpp"

void CSVRender::UnpagedWorldspaceWidget::update()
{
    const CSMWorld::Record<CSMWorld::Cell>& record =
        dynamic_cast<const CSMWorld::Record<CSMWorld::Cell>&> (mCellsModel->getRecord (mCellId));

    Ogre::ColourValue colour;
    colour.setAsABGR (record.get().mAmbi.mAmbient);
    setDefaultAmbient (colour);

    /// \todo deal with mSunlight and mFog/mForDensity

    flagAsModified();
}

CSVRender::UnpagedWorldspaceWidget::UnpagedWorldspaceWidget (const std::string& cellId, CSMDoc::Document& document, QWidget* parent)
: WorldspaceWidget (document, parent), mCellId (cellId)
{
    mCellsModel = &dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_Cells));

    mReferenceablesModel = &dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_Referenceables));

    connect (mCellsModel, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (cellDataChanged (const QModelIndex&, const QModelIndex&)));
    connect (mCellsModel, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
        this, SLOT (cellRowsAboutToBeRemoved (const QModelIndex&, int, int)));

    update();

    mCell.reset (new Cell (document.getData(), getSceneManager(), mCellId, document.getPhysics()));
}

void CSVRender::UnpagedWorldspaceWidget::cellDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    int index = mCellsModel->findColumnIndex (CSMWorld::Columns::ColumnId_Modification);
    QModelIndex cellIndex = mCellsModel->getModelIndex (mCellId, index);

    if (cellIndex.row()>=topLeft.row() && cellIndex.row()<=bottomRight.row())
    {
        if (mCellsModel->data (cellIndex).toInt()==CSMWorld::RecordBase::State_Deleted)
        {
            emit closeRequest();
        }
        else
        {
            /// \todo possible optimisation: check columns and update only if relevant columns have
            /// changed
            update();
        }
    }
}

void CSVRender::UnpagedWorldspaceWidget::cellRowsAboutToBeRemoved (const QModelIndex& parent,
    int start, int end)
{
    QModelIndex cellIndex = mCellsModel->getModelIndex (mCellId, 0);

    if (cellIndex.row()>=start && cellIndex.row()<=end)
        emit closeRequest();
}

bool CSVRender::UnpagedWorldspaceWidget::handleDrop (const std::vector<CSMWorld::UniversalId>& data, DropType type)
{
    if (WorldspaceWidget::handleDrop (data, type))
        return true;

    if (type!=Type_CellsInterior)
        return false;

    mCellId = data.begin()->getId();
    mCell.reset (new Cell (getDocument().getData(), getSceneManager(), mCellId, getDocument().getPhysics()));

    update();
    emit cellChanged(*data.begin());

    return true;
}

void CSVRender::UnpagedWorldspaceWidget::referenceableDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    if (mCell.get())
        if (mCell.get()->referenceableDataChanged (topLeft, bottomRight))
            flagAsModified();
}

void CSVRender::UnpagedWorldspaceWidget::referenceableAboutToBeRemoved (
    const QModelIndex& parent, int start, int end)
{
    if (mCell.get())
        if (mCell.get()->referenceableAboutToBeRemoved (parent, start, end))
            flagAsModified();
}

void CSVRender::UnpagedWorldspaceWidget::referenceableAdded (const QModelIndex& parent,
    int start, int end)
{
    if (mCell.get())
    {
        QModelIndex topLeft = mReferenceablesModel->index (start, 0);
        QModelIndex bottomRight =
            mReferenceablesModel->index (end, mReferenceablesModel->columnCount());

        if (mCell.get()->referenceableDataChanged (topLeft, bottomRight))
            flagAsModified();
    }
}

void CSVRender::UnpagedWorldspaceWidget::referenceDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    if (mCell.get())
        if (mCell.get()->referenceDataChanged (topLeft, bottomRight))
            flagAsModified();
}

void CSVRender::UnpagedWorldspaceWidget::referenceAboutToBeRemoved (const QModelIndex& parent,
    int start, int end)
{
    if (mCell.get())
        if (mCell.get()->referenceAboutToBeRemoved (parent, start, end))
            flagAsModified();
}

void CSVRender::UnpagedWorldspaceWidget::referenceAdded (const QModelIndex& parent, int start,
    int end)
{
    if (mCell.get())
        if (mCell.get()->referenceAdded (parent, start, end))
            flagAsModified();
}

void CSVRender::UnpagedWorldspaceWidget::addVisibilitySelectorButtons (
    CSVWidget::SceneToolToggle2 *tool)
{
    WorldspaceWidget::addVisibilitySelectorButtons (tool);
    tool->addButton (Element_Terrain, "Terrain", "", true);
    tool->addButton (Element_Fog, "Fog");
}

std::string CSVRender::UnpagedWorldspaceWidget::getStartupInstruction()
{
    Ogre::Vector3 position = getCamera()->getPosition();

    std::ostringstream stream;

    stream
        << "player->positionCell "
        << position.x << ", " << position.y << ", " << position.z
        << ", 0, \"" << mCellId << "\"";

    return stream.str();
}

CSVRender::WorldspaceWidget::dropRequirments CSVRender::UnpagedWorldspaceWidget::getDropRequirements (CSVRender::WorldspaceWidget::DropType type) const
{
    dropRequirments requirements = WorldspaceWidget::getDropRequirements (type);

    if (requirements!=ignored)
        return requirements;

    switch(type)
    {
        case Type_CellsInterior:
            return canHandle;

        case Type_CellsExterior:
            return needPaged;

        default:
            return ignored;
    }
}
