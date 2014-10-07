
#include "pagedworldspacewidget.hpp"

#include <sstream>

#include <QMouseEvent>

#include <OgreCamera.h>
#include <OgreSceneManager.h>
#include <OgreManualObject.h>
#include <OGRE/Overlay/OgreOverlayContainer.h>
#include <OGRE/Overlay/OgreOverlayManager.h>
#include <OgreRoot.h>
#include <OgreSceneQuery.h>
#include <OgreEntity.h>

#include "../../../../components/esm/loadland.hpp"
#include "textoverlay.hpp"

#include "../../model/world/tablemimedata.hpp"
#include "../../model/world/idtable.hpp"

#include "../widget/scenetooltoggle.hpp"

#include "elements.hpp"

bool CSVRender::PagedWorldspaceWidget::adjustCells()
{
    bool modified = false;
    bool setCamera = false;

    const CSMWorld::IdCollection<CSMWorld::Cell>& cells = mDocument.getData().getCells();

    {
        // remove (or name/region modified)
        std::map<CSMWorld::CellCoordinates, Cell *>::iterator iter (mCells.begin());

        while (iter!=mCells.end())
        {
            int index = cells.searchId (iter->first.getId (mWorldspace));

            if (!mSelection.has (iter->first) || index==-1 ||
                cells.getRecord (index).mState==CSMWorld::RecordBase::State_Deleted)
            {
                delete iter->second;
                mCells.erase (iter);

                // delete overlays
                std::map<CSMWorld::CellCoordinates, TextOverlay *>::iterator itOverlay = mTextOverlays.find(iter->first);
                if(itOverlay != mTextOverlays.end())
                {
                    delete itOverlay->second;
                    mTextOverlays.erase(itOverlay);
                }

                // destroy manual objects and entities
                std::map<CSMWorld::CellCoordinates, Ogre::Entity *>::iterator itEntity = mEntities.find(iter->first);
                if(itEntity != mEntities.end())
                {
                    getSceneManager()->destroyEntity(itEntity->second);
                    mEntities.erase(itEntity);
                }
                getSceneManager()->destroyManualObject("manual"+iter->first.getId(mWorldspace));

                iter++;
                modified = true;
            }
            else
            {
                // check if name or region field has changed
                std::string name = cells.getRecord(index).get().mName;
                std::string region = cells.getRecord(index).get().mRegion;

                std::map<CSMWorld::CellCoordinates, TextOverlay *>::iterator it = mTextOverlays.find(iter->first);
                if(it != mTextOverlays.end())
                {
                    if(it->second->getDesc() != "") // previously had name
                    {
                        if(name != it->second->getDesc()) // new name
                        {
                            if(name != "")
                                it->second->setDesc(name);
                            else // name deleted, use region
                                it->second->setDesc(region);
                            it->second->update();
                        }
                    }
                    else if(name != "") // name added
                    {
                        it->second->setDesc(name);
                        it->second->update();
                    }
                    else if(region != it->second->getDesc()) // new region
                    {
                        it->second->setDesc(region);
                        it->second->update();
                    }
                }
                ++iter;
            }
        }
    }

    if (mCells.begin()==mCells.end())
        setCamera = true;

    // add
    for (CSMWorld::CellSelection::Iterator iter (mSelection.begin()); iter!=mSelection.end();
        ++iter)
    {
        int index = cells.searchId (iter->getId (mWorldspace));

        if (index!=0 && cells.getRecord (index).mState!=CSMWorld::RecordBase::State_Deleted &&
            mCells.find (*iter)==mCells.end())
        {
            if (setCamera)
            {
                setCamera = false;
                getCamera()->setPosition (ESM::Land::REAL_SIZE * iter->getX() + ESM::Land::REAL_SIZE/2,
                                          ESM::Land::REAL_SIZE * iter->getY() + ESM::Land::REAL_SIZE/2, 0);
            }

            mCells.insert (std::make_pair (*iter,
                new Cell (mDocument.getData(), getSceneManager(), iter->getId (mWorldspace))));

            Ogre::ManualObject* manual = getSceneManager()->createManualObject("manual" + iter->getId(mWorldspace));

            manual->begin("BaseWhite", Ogre::RenderOperation::OT_LINE_LIST);

            // define start and end point (x, y, z)
            // FIXME: need terrain height to get the correct starting point
            manual-> position(ESM::Land::REAL_SIZE * iter->getX() + ESM::Land::REAL_SIZE/2,
                              ESM::Land::REAL_SIZE * iter->getY() + ESM::Land::REAL_SIZE/2, 0);
            manual-> position(ESM::Land::REAL_SIZE * iter->getX() + ESM::Land::REAL_SIZE/2,
                              ESM::Land::REAL_SIZE * iter->getY() + ESM::Land::REAL_SIZE/2, 2000);
            manual->end();
            Ogre::MeshPtr meshPtr = manual->convertToMesh("vLine" + iter->getId(mWorldspace));
            Ogre::Entity* entity = getSceneManager()->createEntity(meshPtr);
            getSceneManager()->getRootSceneNode()->createChildSceneNode()->attachObject(entity);
            entity->setVisible(false);

            // keep pointers so that they can be deleted later
            mEntities.insert(std::make_pair(*iter, entity));

            CSVRender::TextOverlay *textDisp = new CSVRender::TextOverlay(entity, getCamera(), iter->getId(mWorldspace));
            textDisp->enable(true);
            textDisp->setCaption(iter->getId(mWorldspace));
            std::string desc = cells.getRecord(index).get().mName;
            if(desc == "") desc = cells.getRecord(index).get().mRegion;
            textDisp->setDesc(desc);
            textDisp->update();
            mTextOverlays.insert(std::make_pair(*iter, textDisp));

            modified = true;
        }
    }

    return modified;
}

void CSVRender::PagedWorldspaceWidget::mouseReleaseEvent (QMouseEvent *event)
{
    if(event->button() == Qt::RightButton)
    {
        std::map<CSMWorld::CellCoordinates, TextOverlay *>::iterator iter = mTextOverlays.begin();
        for(; iter != mTextOverlays.end(); ++iter)
        {
            if(mDisplayCellCoord &&
               iter->second->isEnabled() && iter->second->container().contains(event->x(), event->y()))
            {
                std::cout << "clicked: " << iter->second->getCaption() << std::endl;
                break;
            }
        }
    }
}

void CSVRender::PagedWorldspaceWidget::mouseDoubleClickEvent (QMouseEvent *event)
{
    if(event->button() == Qt::RightButton)
    {
        std::cout << "double clicked" << std::endl;
    }
}

void CSVRender::PagedWorldspaceWidget::updateOverlay()
{
    // better camera position at the start
    if(getCamera()->getViewport() && getCamera()->getPosition().z < 1)
    {
        getCamera()->move(getCamera()->getDirection() * -6000);
    }

    Ogre::OverlayManager &overlayMgr = Ogre::OverlayManager::getSingleton();
    Ogre::Overlay* overlay = overlayMgr.getByName("CellIDPanel");
    if(overlay && !mTextOverlays.empty())
    {
        if(getCamera()->getViewport())
        {
            if((uint32_t)getCamera()->getViewport()->getVisibilityMask()
                                    & (uint32_t)CSVRender::Element_CellMarker)
            {
                mDisplayCellCoord = true;
                overlay->show();
            }
            else
            {
                mDisplayCellCoord = false;
                overlay->hide();
            }
        }
    }

    if(!mTextOverlays.empty())
    {
        std::map<CSMWorld::CellCoordinates, TextOverlay *>::iterator it = mTextOverlays.begin();
        for(; it != mTextOverlays.end(); ++it)
        {
            it->second->update();
        }
    }
}

void CSVRender::PagedWorldspaceWidget::referenceableDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    for (std::map<CSMWorld::CellCoordinates, Cell *>::iterator iter (mCells.begin());
        iter!=mCells.end(); ++iter)
        if (iter->second->referenceableDataChanged (topLeft, bottomRight))
            flagAsModified();
}

void CSVRender::PagedWorldspaceWidget::referenceableAboutToBeRemoved (
    const QModelIndex& parent, int start, int end)
{
    for (std::map<CSMWorld::CellCoordinates, Cell *>::iterator iter (mCells.begin());
        iter!=mCells.end(); ++iter)
        if (iter->second->referenceableAboutToBeRemoved (parent, start, end))
            flagAsModified();
}

void CSVRender::PagedWorldspaceWidget::referenceableAdded (const QModelIndex& parent,
    int start, int end)
{
    CSMWorld::IdTable& referenceables = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_Referenceables));

    for (std::map<CSMWorld::CellCoordinates, Cell *>::iterator iter (mCells.begin());
        iter!=mCells.end(); ++iter)
    {
        QModelIndex topLeft = referenceables.index (start, 0);
        QModelIndex bottomRight =
            referenceables.index (end, referenceables.columnCount());

        if (iter->second->referenceableDataChanged (topLeft, bottomRight))
            flagAsModified();
    }
}

void CSVRender::PagedWorldspaceWidget::referenceDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    for (std::map<CSMWorld::CellCoordinates, Cell *>::iterator iter (mCells.begin());
        iter!=mCells.end(); ++iter)
        if (iter->second->referenceDataChanged (topLeft, bottomRight))
            flagAsModified();
}

void CSVRender::PagedWorldspaceWidget::referenceAboutToBeRemoved (const QModelIndex& parent,
    int start, int end)
{
    for (std::map<CSMWorld::CellCoordinates, Cell *>::iterator iter (mCells.begin());
        iter!=mCells.end(); ++iter)
        if (iter->second->referenceAboutToBeRemoved (parent, start, end))
            flagAsModified();
}

void CSVRender::PagedWorldspaceWidget::referenceAdded (const QModelIndex& parent, int start,
    int end)
{
    for (std::map<CSMWorld::CellCoordinates, Cell *>::iterator iter (mCells.begin());
        iter!=mCells.end(); ++iter)
        if (iter->second->referenceAdded (parent, start, end))
            flagAsModified();
}

std::string CSVRender::PagedWorldspaceWidget::getStartupInstruction()
{
    Ogre::Vector3 position = getCamera()->getPosition();

    std::ostringstream stream;

    stream
        << "player->position "
        << position.x << ", " << position.y << ", " << position.z
        << ", 0";

    return stream.str();
}

CSVRender::PagedWorldspaceWidget::PagedWorldspaceWidget (QWidget* parent, CSMDoc::Document& document)
: WorldspaceWidget(document, parent), mDocument(document), mWorldspace("std::default"), mDisplayCellCoord(true)
{
    QAbstractItemModel *cells =
        document.getData().getTableModel (CSMWorld::UniversalId::Type_Cells);

    connect (cells, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (cellDataChanged (const QModelIndex&, const QModelIndex&)));
    connect (cells, SIGNAL (rowsRemoved (const QModelIndex&, int, int)),
        this, SLOT (cellRemoved (const QModelIndex&, int, int)));
    connect (cells, SIGNAL (rowsInserted (const QModelIndex&, int, int)),
        this, SLOT (cellAdded (const QModelIndex&, int, int)));
}

CSVRender::PagedWorldspaceWidget::~PagedWorldspaceWidget()
{
    for (std::map<CSMWorld::CellCoordinates, Cell *>::iterator iter (mCells.begin());
        iter!=mCells.end(); ++iter)
    {
        delete iter->second;

        std::map<CSMWorld::CellCoordinates, Ogre::Entity *>::iterator it = mEntities.find(iter->first);
        if(it != mEntities.end())
        {
            getSceneManager()->destroyEntity(it->second);
            mEntities.erase(it);
        }
        getSceneManager()->destroyManualObject("manual"+iter->first.getId(mWorldspace));
    }
}

void CSVRender::PagedWorldspaceWidget::useViewHint (const std::string& hint)
{
    if (!hint.empty())
    {
        CSMWorld::CellSelection selection;

        if (hint[0]=='c')
        {
            // syntax: c:#x1 y1; #x2 y2 (number of coordinate pairs can be 0 or larger)
            char ignore;

            std::istringstream stream (hint.c_str());
            if (stream >> ignore)
            {
                char ignore1; // : or ;
                char ignore2; // #
                int x, y;

                while (stream >> ignore1 >> ignore2 >> x >> y)
                    selection.add (CSMWorld::CellCoordinates (x, y));

                /// \todo adjust camera position
            }
        }
        else if (hint[0]=='r')
        {
            /// \todo implement 'r' type hints
        }

        setCellSelection (selection);
    }
}

void CSVRender::PagedWorldspaceWidget::setCellSelection (const CSMWorld::CellSelection& selection)
{
    mSelection = selection;

    if (adjustCells())
        flagAsModified();

    emit cellSelectionChanged (mSelection);
}

std::pair< int, int > CSVRender::PagedWorldspaceWidget::getCoordinatesFromId (const std::string& record) const
{
    std::istringstream stream (record.c_str());
    char ignore;
    int x, y;
    stream >> ignore >> x >> y;
    return std::make_pair(x, y);
}

bool CSVRender::PagedWorldspaceWidget::handleDrop (
    const std::vector< CSMWorld::UniversalId >& data, DropType type)
{
    if (WorldspaceWidget::handleDrop (data, type))
        return true;

    if (type!=Type_CellsExterior)
        return false;

    bool selectionChanged = false;
    for (unsigned i = 0; i < data.size(); ++i)
    {
        std::pair<int, int> coordinates(getCoordinatesFromId(data[i].getId()));
        if (mSelection.add(CSMWorld::CellCoordinates(coordinates.first, coordinates.second)))
        {
            selectionChanged = true;
        }
    }
    if (selectionChanged)
    {
        if (adjustCells())
            flagAsModified();

        emit cellSelectionChanged(mSelection);
    }

    return true;
}

CSVRender::WorldspaceWidget::dropRequirments CSVRender::PagedWorldspaceWidget::getDropRequirements (CSVRender::WorldspaceWidget::DropType type) const
{
    dropRequirments requirements = WorldspaceWidget::getDropRequirements (type);

    if (requirements!=ignored)
        return requirements;

    switch (type)
    {
        case Type_CellsExterior:
            return canHandle;

        case Type_CellsInterior:
            return needUnpaged;

        default:
            return ignored;
    }
}


unsigned int CSVRender::PagedWorldspaceWidget::getElementMask() const
{
    return WorldspaceWidget::getElementMask() | mControlElements->getSelection();
}

CSVWidget::SceneToolToggle *CSVRender::PagedWorldspaceWidget::makeControlVisibilitySelector (
    CSVWidget::SceneToolbar *parent)
{
    mControlElements = new CSVWidget::SceneToolToggle (parent,
        "Controls & Guides Visibility", ":door.png");

    mControlElements->addButton (":activator.png", Element_CellMarker, ":activator.png",
        "Cell marker");
    mControlElements->addButton (":armor.png", Element_CellArrow, ":armor.png", "Cell arrows");
    mControlElements->addButton (":armor.png", Element_CellBorder, ":armor.png", "Cell border");

    mControlElements->setSelection (0xffffffff);

    connect (mControlElements, SIGNAL (selectionChanged()),
        this, SLOT (elementSelectionChanged()));

    return mControlElements;
}

void CSVRender::PagedWorldspaceWidget::cellDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    /// \todo check if no selected cell is affected and do not update, if that is the case
    if (adjustCells())
        flagAsModified();
}

void CSVRender::PagedWorldspaceWidget::cellRemoved (const QModelIndex& parent, int start,
    int end)
{
    if (adjustCells())
        flagAsModified();
}

void CSVRender::PagedWorldspaceWidget::cellAdded (const QModelIndex& index, int start,
    int end)
{
    /// \todo check if no selected cell is affected and do not update, if that is the case
    if (adjustCells())
        flagAsModified();
}
