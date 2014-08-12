
#include "pagedworldspacewidget.hpp"

#include <sstream>
#include <iostream>

#include <OgreCamera.h>
#include <OgreTextureManager.h>
#include <OgreTechnique.h>
#include <OgreMaterialManager.h>
#include <OgreBillboardSet.h>
#include <OgreBillboard.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>

#include <QtGui/qevent.h>
#include <Qt/qpainter.h>

#include "../../model/world/tablemimedata.hpp"
#include "../../model/world/idtable.hpp"

void CSVRender::PagedWorldspaceWidget::displayCellCoord(bool display)
{
    mDisplayCellCoord = display;
    std::map<CSMWorld::CellCoordinates, Cell *>::iterator iter(mCells.begin());

    while (iter != mCells.end())
    {
        getSceneManager()->getBillboardSet("CellBillboardSet" + iter->first.getId(mWorldspace))->setVisible(display);
        iter++;
    }
}

bool CSVRender::PagedWorldspaceWidget::adjustCells()
{
    bool modified = false;
    bool setCamera = false;

    const CSMWorld::IdCollection<CSMWorld::Cell>& cells = mDocument.getData().getCells();

    {
        // remove
        std::map<CSMWorld::CellCoordinates, Cell *>::iterator iter (mCells.begin());

        while (iter!=mCells.end())
        {
            int index = cells.searchId (iter->first.getId (mWorldspace));

            if (!mSelection.has (iter->first) || index==-1 ||
                cells.getRecord (index).mState==CSMWorld::RecordBase::State_Deleted)
            {
                delete iter->second;
                mCells.erase (iter++);

                getSceneManager()->getSceneNode("CellBillboardNode" + iter->first.getId(mWorldspace))->detachAllObjects();
                getSceneManager()->destroySceneNode("CellBillboardNode" + iter->first.getId(mWorldspace));
                getSceneManager()->destroyBillboardSet("CellBillboardSet" + iter->first.getId(mWorldspace));

                modified = true;
            }
            else
                ++iter;
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
                getCamera()->setPosition (8192*iter->getX()+4096, 8192*iter->getY()+4096, 0);
            }

            mCells.insert (std::make_pair (*iter,
                new Cell (mDocument.getData(), getSceneManager(),
                iter->getId (mWorldspace))));

            //billboard which indicate the Cell coord
            Ogre::SceneNode* billboardNode = getSceneManager()->getRootSceneNode()->createChildSceneNode("CellBillboardNode" + iter->getId(mWorldspace));
            billboardNode->setPosition(8192 * iter->getX() + 4096, 8192 * iter->getY() + 4096, 0);

            QImage image(QSize(1024, 1024), QImage::Format::Format_RGB888);
            QPainter painter(&image);
            std::string text = std::to_string(iter->getX()) + ";" + std::to_string(iter->getY());
            QFont font = painter.font();
            font.setPointSize(256);
            painter.setFont(font);
            painter.setPen(Qt::SolidLine);
            painter.setPen(Qt::white);
            painter.drawText(QRect(0, 0, 1024, 1024), Qt::AlignCenter, QString(text.c_str()));

            Ogre::TexturePtr texture;
            if (Ogre::TextureManager::getSingleton().resourceExists("CellBillboardTexture" + iter->getId(mWorldspace)))
            {
                texture = Ogre::TextureManager::getSingleton().getByName("CellBillboardTexture" + iter->getId(mWorldspace));
            }
            else
            {
                texture = Ogre::TextureManager::getSingleton().createManual("CellBillboardTexture" + iter->getId(mWorldspace), 
                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                    Ogre::TEX_TYPE_2D, 1024, 1024, 1, Ogre::PF_X8R8G8B8, Ogre::TU_DEFAULT);
                Ogre::HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();
                pixelBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD);
                const Ogre::PixelBox& pixBox = pixelBuffer->getCurrentLock();

                std::cout << texture->getWidth() << texture->getHeight();

                Ogre::uint8* pDest = static_cast<Ogre::uint8*>(pixBox.data);
                for (size_t i = 0, width = texture->getWidth(); i < width; ++i)
                {
                    for (size_t j = 0, height = texture->getHeight(); j < height; ++j)
                    {
                        QRgb color = image.pixel(QPoint(j,i));
                        *pDest++ = qBlue(color);
                        *pDest++ = qGreen(color);
                        *pDest++ = qRed(color);
                        *pDest++ = 0;
                    }
                }
                pixelBuffer->unlock();
                texture->load();
            }

            Ogre::MaterialPtr material;
            if (Ogre::MaterialManager::getSingleton().resourceExists("CellBillboardMaterial" + iter->getId(mWorldspace)))
            {
                material = Ogre::MaterialManager::getSingleton().getByName("CellBillboardMaterial" + iter->getId(mWorldspace));
            }
            else
            {
                material = Ogre::MaterialManager::getSingleton().create(
                    "CellBillboardMaterial" + iter->getId(mWorldspace), // name
                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

                material->getTechnique(0)->getPass(0)->createTextureUnitState("CellBillboardTexture" + iter->getId(mWorldspace));
                material->getTechnique(0)->getPass(0)->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
                material->setDepthCheckEnabled(false);
                material->setDepthWriteEnabled(false);
            }

            Ogre::BillboardSet* mySet = getSceneManager()->createBillboardSet("CellBillboardSet" + iter->getId(mWorldspace));
            Ogre::Billboard* myBillboard = mySet->createBillboard(Ogre::Vector3(0, 0, 0));
            mySet->setDefaultDimensions(4000, 2000);
            mySet->setMaterial(material);
            mySet->setRenderQueueGroup(mySet->getRenderQueueGroup() + 1); // render the bilboard on top
            billboardNode->attachObject(mySet);

            mySet->setVisible(mDisplayCellCoord);

            modified = true;
        }
    }

    return modified;
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
        delete iter->second;
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

void CSVRender::PagedWorldspaceWidget::handleDrop (const std::vector< CSMWorld::UniversalId >& data)
{
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
}

CSVRender::WorldspaceWidget::dropRequirments CSVRender::PagedWorldspaceWidget::getDropRequirements (CSVRender::WorldspaceWidget::dropType type) const
{
    switch (type)
    {
        case cellsExterior:
            return canHandle;

        case cellsInterior:
            return needUnpaged;

        default:
            return ignored;
    }
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