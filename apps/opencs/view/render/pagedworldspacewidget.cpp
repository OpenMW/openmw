
#include "pagedworldspacewidget.hpp"

#include <sstream>

#include <OgreCamera.h>
#include <OgreTextureManager.h>
#include <OgreTechnique.h>
#include <OgreMaterialManager.h>
#include <OgreBillboardSet.h>
#include <OgreBillboard.h>
#include <Overlay/OgreFontManager.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>

#include <QtGui/qevent.h>

#include "../../model/world/tablemimedata.hpp"
#include "../../model/world/idtable.hpp"

void WriteToTexture(const Ogre::String &str, Ogre::TexturePtr destTexture, Ogre::Image::Box destRectangle, Ogre::Font* font, const Ogre::ColourValue &color, char justify = 'l', bool wordwrap = true)
{
	using namespace Ogre;

	if (destTexture->getHeight() < destRectangle.bottom)
		destRectangle.bottom = destTexture->getHeight();
	if (destTexture->getWidth() < destRectangle.right)
		destRectangle.right = destTexture->getWidth();

	if (!font->isLoaded())
		font->load();

	TexturePtr fontTexture = (TexturePtr)TextureManager::getSingleton().getByName(font->getMaterial()->getTechnique(0)->getPass(0)->getTextureUnitState(0)->getTextureName());

	HardwarePixelBufferSharedPtr fontBuffer = fontTexture->getBuffer();
	HardwarePixelBufferSharedPtr destBuffer = destTexture->getBuffer();

	PixelBox destPb = destBuffer->lock(destRectangle, HardwareBuffer::HBL_NORMAL);

	// The font texture buffer was created write only...so we cannot read it back :o). One solution is to copy the buffer  instead of locking it. (Maybe there is a way to create a font texture which is not write_only ?)

	// create a buffer
	size_t nBuffSize = fontBuffer->getSizeInBytes();
	uint8* buffer = (uint8*)calloc(nBuffSize, sizeof(uint8));

	// create pixel box using the copy of the buffer
	PixelBox fontPb(fontBuffer->getWidth(), fontBuffer->getHeight(), fontBuffer->getDepth(), fontBuffer->getFormat(), buffer);
	fontBuffer->blitToMemory(fontPb);

	uint8* fontData = static_cast<uint8*>(fontPb.data);
	uint8* destData = static_cast<uint8*>(destPb.data);

	const size_t fontPixelSize = PixelUtil::getNumElemBytes(fontPb.format);
	const size_t destPixelSize = PixelUtil::getNumElemBytes(destPb.format);

	const size_t fontRowPitchBytes = fontPb.rowPitch * fontPixelSize;
	const size_t destRowPitchBytes = destPb.rowPitch * destPixelSize;

	Box *GlyphTexCoords;
	GlyphTexCoords = new Box[str.size()];

	Font::UVRect glypheTexRect;
	size_t charheight = 0;
	size_t charwidth = 0;

	for (unsigned int i = 0; i < str.size(); i++)
	{
		if ((str[i] != '\t') && (str[i] != '\n') && (str[i] != ' '))
		{
			glypheTexRect = font->getGlyphTexCoords(str[i]);
			GlyphTexCoords[i].left = glypheTexRect.left * fontTexture->getSrcWidth();
			GlyphTexCoords[i].top = glypheTexRect.top * fontTexture->getSrcHeight();
			GlyphTexCoords[i].right = glypheTexRect.right * fontTexture->getSrcWidth();
			GlyphTexCoords[i].bottom = glypheTexRect.bottom * fontTexture->getSrcHeight();

			if (GlyphTexCoords[i].getHeight() > charheight)
				charheight = GlyphTexCoords[i].getHeight();
			if (GlyphTexCoords[i].getWidth() > charwidth)
				charwidth = GlyphTexCoords[i].getWidth();
		}

	}

	size_t cursorX = 0;
	size_t cursorY = 0;
	size_t lineend = destRectangle.getWidth();
	bool carriagreturn = true;
	for (unsigned int strindex = 0; strindex < str.size(); strindex++)
	{
		switch (str[strindex])
		{
		case ' ': cursorX += charwidth;  break;
		case '\t':cursorX += charwidth * 3; break;
		case '\n':cursorY += charheight; carriagreturn = true; break;
		default:
		{
				   //wrapping
				   if ((cursorX + GlyphTexCoords[strindex].getWidth()> lineend) && !carriagreturn)
				   {
					   cursorY += charheight;
					   carriagreturn = true;
				   }

				   //justify
				   if (carriagreturn)
				   {
					   size_t l = strindex;
					   size_t textwidth = 0;
					   size_t wordwidth = 0;

					   while ((l < str.size()) && (str[l] != '\n)'))
					   {
						   wordwidth = 0;

						   switch (str[l])
						   {
						   case ' ': wordwidth = charwidth; ++l; break;
						   case '\t': wordwidth = charwidth * 3; ++l; break;
						   case '\n': l = str.size();
						   }

						   if (wordwrap)
						   while ((l < str.size()) && (str[l] != ' ') && (str[l] != '\t') && (str[l] != '\n'))
						   {
							   wordwidth += GlyphTexCoords[l].getWidth();
							   ++l;
						   }
						   else
						   {
							   wordwidth += GlyphTexCoords[l].getWidth();
							   l++;
						   }

						   if ((textwidth + wordwidth) <= destRectangle.getWidth())
							   textwidth += (wordwidth);
						   else
							   break;
					   }

					   if ((textwidth == 0) && (wordwidth > destRectangle.getWidth()))
						   textwidth = destRectangle.getWidth();

					   switch (justify)
					   {
					   case 'c':    cursorX = (destRectangle.getWidth() - textwidth) / 2;
						   lineend = destRectangle.getWidth() - cursorX;
						   break;

					   case 'r':    cursorX = (destRectangle.getWidth() - textwidth);
						   lineend = destRectangle.getWidth();
						   break;

					   default:    cursorX = 0;
						   lineend = textwidth;
						   break;
					   }

					   carriagreturn = false;
				   }

				   //abort - net enough space to draw
				   if ((cursorY + charheight) > destRectangle.getHeight())
					   goto stop;

				   //draw pixel by pixel
				   for (size_t i = 0; i < GlyphTexCoords[strindex].getHeight(); i++)
				   for (size_t j = 0; j < GlyphTexCoords[strindex].getWidth(); j++)
				   {
					   float alpha = color.a * (fontData[(i + GlyphTexCoords[strindex].top) * fontRowPitchBytes + (j + GlyphTexCoords[strindex].left) * fontPixelSize + 1] / 255.0);
					   float invalpha = 1.0 - alpha;
					   size_t offset = (i + cursorY) * destRowPitchBytes + (j + cursorX) * destPixelSize;
					   ColourValue pix;
					   PixelUtil::unpackColour(&pix, destPb.format, &destData[offset]);
					   pix = (pix * invalpha) + (color * alpha);
					   PixelUtil::packColour(pix, destPb.format, &destData[offset]);
				   }

				   cursorX += GlyphTexCoords[strindex].getWidth();
		}//default
		}//switch
	}//for

stop:
	delete[] GlyphTexCoords;

	destBuffer->unlock();

	// Free the memory allocated for the buffer
	free(buffer); buffer = 0;
}

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
				getSceneManager()->getBillboardSet("CellBillboardSet" + iter->first.getId(mWorldspace))->removeBillboard(
					getSceneManager()->getBillboardSet("CellBillboardSet" + iter->first.getId(mWorldspace))->getBillboard(0));
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

			Ogre::Font* font;
			if (Ogre::FontManager::getSingletonPtr()->getByName("CellBillboardFont" + iter->getId(mWorldspace)).isNull())
			{
				font = Ogre::FontManager::getSingletonPtr()->create("CellBillboardFont" + iter->getId(mWorldspace), "Data00000001").getPointer();
				font->setType(Ogre::FT_TRUETYPE);
				font->setSource("Comic.ttf");
				font->setTrueTypeSize(256);
				font->load();
			}
			else
			{
				font = Ogre::FontManager::getSingletonPtr()->getByName("CellBillboardFont" + iter->getId(mWorldspace)).getPointer();
			}

			Ogre::TexturePtr texture;
			if (Ogre::TextureManager::getSingleton().resourceExists("CellBillboardTexture" + iter->getId(mWorldspace)))
			{
				texture = Ogre::TextureManager::getSingleton().getByName("CellBillboardTexture" + iter->getId(mWorldspace));
			}
			else
			{
				texture = Ogre::TextureManager::getSingleton().createManual("CellBillboardTexture" + iter->getId(mWorldspace), "Data00000001", Ogre::TEX_TYPE_2D, 1024, 512, Ogre::MIP_UNLIMITED, Ogre::PF_X8R8G8B8, Ogre::TU_STATIC | Ogre::TU_AUTOMIPMAP);
				WriteToTexture(std::to_string(iter->getX()) + ";" + std::to_string(iter->getY()), texture, Ogre::Image::Box(0, 100, 1024, 512), font, Ogre::ColourValue(1.0, 1.0, 1.0, 1.0), 'c');
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
			mySet->setMaterial(material);
			myBillboard->setDimensions(4000, 2000);
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