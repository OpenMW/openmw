
#include "pagedworldspacewidget.hpp"

#include <sstream>

#include <QMouseEvent>

#include <OgreCamera.h>
#include <OgreSceneManager.h>
#include <OgreManualObject.h>
#include <OgreOverlayContainer.h>
#include <OgreOverlayManager.h>
#include <OgreRoot.h>
#include <OgreSceneQuery.h>

#include <OgreEntity.h>              // FIXME: visual highlight, clone
#include <OgreMaterialManager.h>     // FIXME: visual highlight, material
#include <OgreHardwarePixelBuffer.h> // FIXME: visual highlight, texture

#include <components/esm/loadland.hpp>
#include "textoverlay.hpp"
#include "overlaymask.hpp"

#include "../../model/world/tablemimedata.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/settings/usersettings.hpp"

#include "../widget/scenetooltoggle.hpp"
#include "../world/physicssystem.hpp"

#include "elements.hpp"

namespace
{
    // FIXME: this section should be removed once the debugging is completed
    void showHitPoint(Ogre::SceneManager *sceneMgr, std::string name, Ogre::Vector3 point)
    {
        if(sceneMgr->hasManualObject("manual" + name))
            sceneMgr->destroyManualObject("manual" + name);
        Ogre::ManualObject* manual = sceneMgr->createManualObject("manual" + name);
        manual->begin("BaseWhite", Ogre::RenderOperation::OT_LINE_LIST);
        manual-> position(point.x,     point.y,     point.z-100);
        manual-> position(point.x,     point.y,     point.z+100);
        manual-> position(point.x,     point.y-100, point.z);
        manual-> position(point.x,     point.y+100, point.z);
        manual-> position(point.x-100, point.y,     point.z);
        manual-> position(point.x+100, point.y,     point.z);
        manual->end();
        sceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(manual);
    }

    void removeHitPoint(Ogre::SceneManager *sceneMgr, std::string name)
    {
        if(sceneMgr->hasManualObject("manual" + name))
            sceneMgr->destroyManualObject("manual" + name);
    }

    void initDebug()
    {
        // material for visual cue on selected objects
        Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().getByName("DynamicTrans");
        if(texture.isNull())
        {
            texture = Ogre::TextureManager::getSingleton().createManual(
                "DynamicTrans", // name
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                Ogre::TEX_TYPE_2D,  // type
                8, 8,               // width & height
                0,                  // number of mipmaps
                Ogre::PF_BYTE_BGRA, // pixel format
                Ogre::TU_DEFAULT);  // usage; should be TU_DYNAMIC_WRITE_ONLY_DISCARDABLE for
                                    // textures updated very often (e.g. each frame)

            Ogre::HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();
            pixelBuffer->lock(Ogre::HardwareBuffer::HBL_NORMAL);
            const Ogre::PixelBox& pixelBox = pixelBuffer->getCurrentLock();

            uint8_t* pDest = static_cast<uint8_t*>(pixelBox.data);

            // Fill in some pixel data. This will give a semi-transparent colour,
            // but this is of course dependent on the chosen pixel format.
            for (size_t j = 0; j < 8; j++)
            {
                for(size_t i = 0; i < 8; i++)
                {
                    *pDest++ = 255; // B
                    *pDest++ = 255; // G
                    *pDest++ = 127; // R
                    *pDest++ =  63; // A
                }

                pDest += pixelBox.getRowSkip() * Ogre::PixelUtil::getNumElemBytes(pixelBox.format);
            }
            pixelBuffer->unlock();
        }
        Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().getByName(
                    "TransMaterial");
        if(material.isNull())
        {
            Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().create(
                        "TransMaterial",
                        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, true );
            Ogre::Pass *pass = material->getTechnique( 0 )->getPass( 0 );
            pass->setLightingEnabled( false );
            pass->setDepthWriteEnabled( false );
            pass->setSceneBlending( Ogre::SBT_TRANSPARENT_ALPHA );

            Ogre::TextureUnitState *tex = pass->createTextureUnitState("CustomState", 0);
            tex->setTextureName("DynamicTrans");
            tex->setTextureFiltering( Ogre::TFO_ANISOTROPIC );
            material->load();
        }
    }
}

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
                // delete overlays
                std::map<CSMWorld::CellCoordinates, TextOverlay *>::iterator itOverlay = mTextOverlays.find(iter->first);
                if(itOverlay != mTextOverlays.end())
                {
                    delete itOverlay->second;
                    mTextOverlays.erase(itOverlay);
                }

                // destroy manual objects
                getSceneManager()->destroyManualObject("manual"+iter->first.getId(mWorldspace));

                delete iter->second;
                mCells.erase (iter++);

                modified = true;
            }
            else
            {
                // check if name or region field has changed
                // FIXME: config setting
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
                    modified = true;
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

        if (index > 0 && cells.getRecord (index).mState!=CSMWorld::RecordBase::State_Deleted &&
            mCells.find (*iter)==mCells.end())
        {
            Cell *cell = new Cell (mDocument.getData(), getSceneManager(),
                    iter->getId (mWorldspace));
            mCells.insert (std::make_pair (*iter, cell));

            float height = cell->getTerrainHeightAt(Ogre::Vector3(
                              ESM::Land::REAL_SIZE * iter->getX() + ESM::Land::REAL_SIZE/2,
                              ESM::Land::REAL_SIZE * iter->getY() + ESM::Land::REAL_SIZE/2,
                              0));
            if (setCamera)
            {
                setCamera = false;
                getCamera()->setPosition (
                              ESM::Land::REAL_SIZE * iter->getX() + ESM::Land::REAL_SIZE/2,
                              ESM::Land::REAL_SIZE * iter->getY() + ESM::Land::REAL_SIZE/2,
                              height);
                // better camera position at the start
                getCamera()->move(getCamera()->getDirection() * -6000); // FIXME: config setting
            }

            Ogre::ManualObject* manual =
                    getSceneManager()->createManualObject("manual" + iter->getId(mWorldspace));
            manual->begin("BaseWhite", Ogre::RenderOperation::OT_LINE_LIST);
            // define start and end point (x, y, z)
            manual-> position(ESM::Land::REAL_SIZE * iter->getX() + ESM::Land::REAL_SIZE/2,
                              ESM::Land::REAL_SIZE * iter->getY() + ESM::Land::REAL_SIZE/2,
                              height);
            manual-> position(ESM::Land::REAL_SIZE * iter->getX() + ESM::Land::REAL_SIZE/2,
                              ESM::Land::REAL_SIZE * iter->getY() + ESM::Land::REAL_SIZE/2,
                              height+200); // FIXME: config setting
            manual->end();
            manual->setBoundingBox(Ogre::AxisAlignedBox(
                              ESM::Land::REAL_SIZE * iter->getX() + ESM::Land::REAL_SIZE/2,
                              ESM::Land::REAL_SIZE * iter->getY() + ESM::Land::REAL_SIZE/2,
                              height,
                              ESM::Land::REAL_SIZE * iter->getX() + ESM::Land::REAL_SIZE/2,
                              ESM::Land::REAL_SIZE * iter->getY() + ESM::Land::REAL_SIZE/2,
                              height+200));
            getSceneManager()->getRootSceneNode()->createChildSceneNode()->attachObject(manual);
            manual->setVisible(false);

            CSVRender::TextOverlay *textDisp =
                    new CSVRender::TextOverlay(manual, getCamera(), iter->getId(mWorldspace));
            textDisp->enable(true);
            textDisp->setCaption(iter->getId(mWorldspace));
            std::string desc = cells.getRecord(index).get().mName;
            if(desc == "") desc = cells.getRecord(index).get().mRegion;
            textDisp->setDesc(desc); // FIXME: config setting
            textDisp->update();
            mTextOverlays.insert(std::make_pair(*iter, textDisp));
            if(!mOverlayMask)
            {
                mOverlayMask = new OverlayMask(mTextOverlays, getViewport());
                addRenderTargetListener(mOverlayMask);
            }

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

        // mouse picking
        // FIXME: need to virtualise mouse buttons
        if(!getCamera()->getViewport())
            return;

        debugMousePicking((float) event->x() / getCamera()->getViewport()->getActualWidth(),
                          (float) event->y() / getCamera()->getViewport()->getActualHeight());
    }
}

void CSVRender::PagedWorldspaceWidget::mouseDoubleClickEvent (QMouseEvent *event)
{
    if(event->button() == Qt::RightButton)
    {
        std::cout << "double clicked" << std::endl;

        CSMSettings::UserSettings &userSettings = CSMSettings::UserSettings::instance();
        if(userSettings.setting ("debug/mouse-picking", QString("false")) == "true" ? true : false)
        {
            // FIXME: OEngine::PhysicEngine creates only one child scene node for the
            // debug drawer.  Hence only the first subview that creates the debug drawer
            // can view the debug lines.  Will need to keep a map in OEngine if multiple
            // subviews are to be supported.
            //CSVWorld::PhysicsSystem::instance()->setSceneManager(getSceneManager());
            CSVWorld::PhysicsSystem::instance()->toggleDebugRendering();
            flagAsModified();
        }
    }
}

void CSVRender::PagedWorldspaceWidget::updateOverlay()
{
    if(getCamera()->getViewport())
    {
        if((uint32_t)getCamera()->getViewport()->getVisibilityMask()
                                & (uint32_t)CSVRender::Element_CellMarker)
            mDisplayCellCoord = true;
        else
            mDisplayCellCoord = false;
    }

    if(!mTextOverlays.empty())
    {
        std::map<CSMWorld::CellCoordinates, TextOverlay *>::iterator it = mTextOverlays.begin();
        for(; it != mTextOverlays.end(); ++it)
        {
            it->second->enable(mDisplayCellCoord);
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
: WorldspaceWidget (document, parent), mDocument (document), mWorldspace ("std::default"),
  mControlElements(NULL), mDisplayCellCoord(true), mOverlayMask(NULL)
{
    QAbstractItemModel *cells =
        document.getData().getTableModel (CSMWorld::UniversalId::Type_Cells);

    connect (cells, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (cellDataChanged (const QModelIndex&, const QModelIndex&)));
    connect (cells, SIGNAL (rowsRemoved (const QModelIndex&, int, int)),
        this, SLOT (cellRemoved (const QModelIndex&, int, int)));
    connect (cells, SIGNAL (rowsInserted (const QModelIndex&, int, int)),
        this, SLOT (cellAdded (const QModelIndex&, int, int)));

        initDebug();
}

CSVRender::PagedWorldspaceWidget::~PagedWorldspaceWidget()
{
    for (std::map<CSMWorld::CellCoordinates, Cell *>::iterator iter (mCells.begin());
        iter!=mCells.end(); ++iter)
    {
        delete iter->second;

        getSceneManager()->destroyManualObject("manual"+iter->first.getId(mWorldspace));
    }

    for (std::map<CSMWorld::CellCoordinates, TextOverlay *>::iterator iter (mTextOverlays.begin());
        iter != mTextOverlays.end(); ++iter)
    {
        delete iter->second;
    }

    removeRenderTargetListener(mOverlayMask);
    delete mOverlayMask;

    // For debugging only
    std::map<std::string, std::vector<std::string> >::iterator iter = mSelectedEntities.begin();
    for(;iter != mSelectedEntities.end(); ++iter)
    {
        removeHitPoint(getSceneManager(), iter->first);
        Ogre::SceneNode *scene = getSceneManager()->getSceneNode(iter->first);
        if(scene)
        {
            scene->removeAndDestroyAllChildren();
            getSceneManager()->destroySceneNode(iter->first);
        }
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


void CSVRender::PagedWorldspaceWidget::debugMousePicking(float mouseX, float mouseY)
{
    CSMSettings::UserSettings &userSettings = CSMSettings::UserSettings::instance();
    bool debug = userSettings.setting ("debug/mouse-picking", QString("false")) == "true" ? true : false;

    std::pair<std::string, Ogre::Vector3> result = CSVWorld::PhysicsSystem::instance()->castRay(
                                                mouseX, mouseY, NULL, NULL, getCamera());
    if(debug && result.first != "")
    {
        // FIXME: is there  a better way to distinguish terrain from objects?
        QString name  = QString(result.first.c_str());
        if(name.contains(QRegExp("^HeightField")))
        {
            // terrain
            std::cout << "terrain: " << result.first << std::endl;
            std::cout << "  hit pos "+ QString::number(result.second.x).toStdString()
                    + ", " + QString::number(result.second.y).toStdString()
                    + ", " + QString::number(result.second.z).toStdString()
                    << std::endl;
        }
        else
        {
            std::string sceneNode =
                CSVWorld::PhysicsSystem::instance()->referenceToSceneNode(result.first);

            uint32_t visibilityMask = getCamera()->getViewport()->getVisibilityMask();
            bool ignoreObjects = !(visibilityMask & (uint32_t)CSVRender::Element_Reference);

            if(!ignoreObjects && getSceneManager()->hasSceneNode(sceneNode))
            {
                if(userSettings.setting("debug/mouse-picking", QString("false")) == "true" ? true : false)
                    updateSelectionHighlight(sceneNode, result.second);
            }

            std::cout << "ReferenceId: " << result.first << std::endl;
            const CSMWorld::CellRef& cellref = mDocument.getData().getReferences().getRecord (result.first).get();
            //std::cout << "CellRef.mId: " << cellref.mId << std::endl; // Same as ReferenceId
            std::cout << "  CellRef.mCell: " << cellref.mCell << std::endl;

            const CSMWorld::RefCollection& references = mDocument.getData().getReferences();
            int index = references.searchId(result.first);
            if (index != -1)
            {
                int columnIndex =
                    references.findColumnIndex(CSMWorld::Columns::ColumnId_ReferenceableId);

                std::cout << "  index: " + QString::number(index).toStdString()
                        +", column index: " + QString::number(columnIndex).toStdString() << std::endl;
            }

            std::map<CSMWorld::CellCoordinates, Cell *>::iterator iter (mCells.begin());
            while (iter!=mCells.end())
            {
                if(iter->first.getId("dummy") == cellref.mCell)
                {
                    //std::cout << "Cell found" << std::endl;
                    break;
                }
                ++iter;
            }
            flagAsModified();
        }
    }
}

// FIXME: for debugging only
void CSVRender::PagedWorldspaceWidget::updateSelectionHighlight(std::string sceneNode, const Ogre::Vector3 &position)
{
    CSMSettings::UserSettings &userSettings = CSMSettings::UserSettings::instance();
    bool debugCursor = userSettings.setting(
                "debug/mouse-position", QString("false")) == "true" ? true : false;

    //TODO: Try http://www.ogre3d.org/tikiwiki/Create+outline+around+a+character
    Ogre::SceneNode *scene = getSceneManager()->getSceneNode(sceneNode);
    std::map<std::string, std::vector<std::string> >::iterator iter =
                                            mSelectedEntities.find(sceneNode);
    if(iter != mSelectedEntities.end()) // currently selected
    {
        std::vector<std::string> clonedEntities = mSelectedEntities[sceneNode];
        while(!clonedEntities.empty())
        {
            if(getSceneManager()->hasEntity(clonedEntities.back()))
            {
                scene->detachObject(clonedEntities.back());
                getSceneManager()->destroyEntity(clonedEntities.back());
            }
            clonedEntities.pop_back();
        }
        mSelectedEntities.erase(iter);

        if(debugCursor)
            removeHitPoint(getSceneManager(), sceneNode);
    }
    else
    {
        std::vector<std::string> clonedEntities;
        Ogre::SceneNode::ObjectIterator iter = scene->getAttachedObjectIterator();
        iter.begin();
        while(iter.hasMoreElements())
        {
            Ogre::MovableObject * element = iter.getNext();
            if(!element)
                break;

            if(element->getMovableType() != "Entity")
                continue;

            Ogre::Entity * entity = dynamic_cast<Ogre::Entity *>(element);
            if(getSceneManager()->hasEntity(entity->getName()+"cover"))
            {
                // FIXME: this shouldn't really happen... but does :(
                scene->detachObject(entity->getName()+"cover");
                getSceneManager()->destroyEntity(entity->getName()+"cover");
            }
            Ogre::Entity * clone = entity->clone(entity->getName()+"cover");

            Ogre::MaterialPtr mat =
                Ogre::MaterialManager::getSingleton().getByName("TransMaterial");
            if(!mat.isNull())
            {
                clone->setMaterial(mat);
                scene->attachObject(clone);
                clonedEntities.push_back(entity->getName()+"cover");
            }
        }
        mSelectedEntities[sceneNode] = clonedEntities;

        if(debugCursor)
            showHitPoint(getSceneManager(), sceneNode, position);
    }
}
