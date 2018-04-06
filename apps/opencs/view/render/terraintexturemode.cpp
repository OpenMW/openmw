// To-do: Getting Texture Id and Texture Filenames to base class Terraintexturemode
// To-do: Better data handling options for mBrushTexture
// To-do: loading texture bitmaps from virtual file system (vfs) for texture overlay icon
// std::cout are for debugging, will be removed later

#include "terraintexturemode.hpp"
#include "editmode.hpp"

#include <iostream>
#include <string>

//Some includes are not needed (have to clean this up later)
#include <QWidget>
#include <QIcon>
#include <QPainter>
#include <QEvent>
#include <QDropEvent>
#include <QColor>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QDragEnterEvent>
#include <QFrame>
#include <QDrag>

#include "../widget/modebutton.hpp"
#include "../widget/scenetoolmode.hpp"
#include "../widget/scenetoolbar.hpp"

#include "../../model/doc/document.hpp"
#include "../../model/world/land.hpp"
#include "../../model/world/landtexture.hpp"
#include "../../model/world/universalid.hpp"
#include "../../model/world/tablemimedata.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/columnbase.hpp"
#include "../../model/world/resourcetable.hpp"

#include "pagedworldspacewidget.hpp"

CSVRender::TerrainTextureMode::TerrainTextureMode (WorldspaceWidget *worldspaceWidget, QWidget *parent)
: EditMode (worldspaceWidget, QIcon {":scenetoolbar/editing-terrain-texture"}, Mask_Terrain | Mask_Reference, "Terrain texture editing", parent)
, mSubMode (0), brushWindow(new BrushWindow(worldspaceWidget, this))
{
    setAcceptDrops(true);
}

void CSVRender::TerrainTextureMode::activate(CSVWidget::SceneToolbar* toolbar)
{

    QIcon testIcon;
    testIcon = CSVRender::TerrainTextureMode::drawIconTexture();

    ModeButton *squareBrushButton = new ModeButton (testIcon, "lalala", toolbar);

    if (!mSubMode)
    {
        mSubMode = new CSVWidget::SceneToolMode (toolbar, "TERRAINTEXTURESUBMODE");
        mSubMode->addButton (":scenetoolbar/brush-point", "point-brush",
            "Single point brush (no size)");
        mSubMode->addButton (squareBrushButton, "square-brush");
                //addButton (button, id);
        mSubMode->addButton (":scenetoolbar/brush-circle", "circle-brush",
            "Circle brush (size is diameter)");
        mSubMode->addButton (":scenetoolbar/brush-custom", "custom-brush",
            "Custom selection brush");
        mSubMode->setButton (mSubModeId);

        connect (mSubMode, SIGNAL (modeChanged (const std::string&)),
            this, SLOT (subModeChanged (const std::string&)));
    }

    toolbar->addTool (mSubMode);
    EditMode::activate(toolbar);

    std::string subMode = mSubMode->getCurrentId();
    std::cout << subMode << std::endl;

    brushWindow->setWindowTitle("Texture brush settings");
    brushWindow->show();

    getWorldspaceWidget().setSubMode (getSubModeFromId (subMode), Mask_Reference);
}



CSVRender::BrushWindow::BrushWindow(WorldspaceWidget *worldspaceWidget, QWidget *parent)
    : QWidget(parent), mWorldspaceWidget (worldspaceWidget)
{
    //Get landtexture-data via document
    CSMDoc::Document& document = mWorldspaceWidget->getDocument();
    CSMWorld::IdTable& ltexs = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_LandTextures));

    label = new QLabel("Drag texture to button", this);

    const std::string& iconPoint = ":scenetoolbar/brush-point";
    const std::string& iconSquare = ":scenetoolbar/brush-square";
    const std::string& iconCircle = ":scenetoolbar/brush-circle";
    const std::string& iconCustom = ":scenetoolbar/brush-custom";

    BrushButton *button1 = new BrushButton(QIcon (QPixmap (iconPoint.c_str())), "", this);
    BrushButton *button2 = new BrushButton(QIcon (QPixmap (iconSquare.c_str())), "", this);
    BrushButton *button3 = new BrushButton(QIcon (QPixmap (iconCircle.c_str())), "", this);
    BrushButton *button4 = new BrushButton(QIcon (QPixmap (iconCustom.c_str())), "", this);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins (QMargins (0, 0, 0, 0));

    configureButtonInitialSettings(button1);
    configureButtonInitialSettings(button2);
    configureButtonInitialSettings(button3);
    configureButtonInitialSettings(button4);

    layout->addWidget(label);
    layout->addWidget(button1);
    layout->addWidget(button2);
    layout->addWidget(button3);
    layout->addWidget(button4);

    setLayout(layout);

    QButtonGroup* brushButtonGroup = new QButtonGroup(this);
    brushButtonGroup->addButton(button1);
    brushButtonGroup->addButton(button2);
    brushButtonGroup->addButton(button3);
    brushButtonGroup->addButton(button4);

    brushButtonGroup->setExclusive(true);

    setWindowFlags(Qt::Window);
}

CSVRender::BrushWindow::BrushButton::BrushButton (const QIcon & icon, const QString & text, QWidget * parent)
    : QPushButton(icon, text, parent)
{
}

void CSVRender::BrushWindow::configureButtonInitialSettings(BrushButton *button)
{
  button->setSizePolicy (QSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed));
  button->setContentsMargins (QMargins (0, 0, 0, 0));
  button->setIconSize (QSize (48-6, 48-6));
  button->setFixedSize (48, 48);
  button->setAcceptDrops(true);
  button->setCheckable(true);
}

void CSVRender::BrushWindow::BrushButton::dragEnterEvent (QDragEnterEvent *event)
{
  event->accept();
}

void CSVRender::BrushWindow::BrushButton::dropEvent (QDropEvent *event)
{
  const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData());

  if (!mime) // May happen when non-records (e.g. plain text) are dragged and dropped
      return;

  if (mime->holdsType (CSMWorld::UniversalId::Type_LandTexture))
  {
      std::cout << "Texture data dropped!" << std::endl;

      const std::vector<CSMWorld::UniversalId> ids = mime->getData();

      for (const CSMWorld::UniversalId& uid : ids)
      {
          std::cout << "Id: " << uid.getId() << std::endl;
          std::cout << "ArgumentType: " << uid.getArgumentType() << " " << uid.toString() << std::endl; // = 1
          std::cout << "Type: " << uid.getType() << std::endl; // = 90
          std::cout << "Class: " << uid.getClass() << std::endl; // = 1 (Record)
          std::cout << "Typename: " << uid.getTypeName() << std::endl; // = LandTexture
          std::string mBrushTexture(uid.getId());
          std::cout << "mBrushTexture: " << mBrushTexture << std::endl;
      }
  }
      else
          std::cout << "Dropped data not recognized as texture!" << std::endl;
}

void CSVRender::TerrainTextureMode::deactivate(CSVWidget::SceneToolbar* toolbar)
{

    if (mSubMode)
    {
        toolbar->removeTool (mSubMode);
        delete mSubMode;
        mSubMode = 0;
    }

    EditMode::deactivate(toolbar);
}

void CSVRender::TerrainTextureMode::primarySelectPressed(const WorldspaceHitResult& hit)
{
}

void CSVRender::TerrainTextureMode::secondarySelectPressed(const WorldspaceHitResult& hit)
{
}

// This function is messy, has very basic icon drawing code and some unrelated landtexture-code that doesn't need to be here,
// it's however a reference for developer (Nelsson).
QIcon CSVRender::TerrainTextureMode::drawIconTexture()
{
    // Just work on brush-square until feature is developed
    QPixmap pixmapimage = QPixmap(":scenetoolbar/brush-square");
    QPainter painter(&pixmapimage);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);

    //Load Land Texture Data
    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    CSMWorld::IdTable& ltexs = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_LandTextures));

    //Temporary code for debugging purposes
    std::cout << "rowCount: " << ltexs.rowCount() << std::endl;
    std::cout << "columnCount: " << ltexs.columnCount() << std::endl;

    for( int i = 0; i < ltexs.rowCount(); i = i + 1 ) {
      for( int j = 0; j < ltexs.columnCount(); j = j + 1 ) {
        QModelIndex index = ltexs.index(i,j);

        QVariant qv = ltexs.data(index, 0);   //return mColumns.at (column)->get (mRecords.at (index));
        QString qs = qv.toString();
        std::string utf8_text = qs.toUtf8().constData();

        std::cout << "Typename: " << qv.typeName() << ", i = " << i << ", j = " << j << ", value = " << utf8_text << std::endl;;
      }
    }

    //Load Land Texture Data
    CSMWorld::ResourceTable& ltexsdata = dynamic_cast<CSMWorld::ResourceTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_Textures)); // Filenames for all textures

    //Temporary code for debugging purposes
    std::cout << "rowCount: " << ltexsdata.rowCount() << std::endl;
    std::cout << "columnCount: " << ltexsdata.columnCount() << std::endl;

    for( int i = 0; i < ltexsdata.rowCount(); i = i + 1 ) {
      for( int j = 0; j < ltexsdata.columnCount(); j = j + 1 ) {
        QModelIndex index = ltexsdata.index(i,j);

        QVariant qv = ltexsdata.data(index, 0);   //return mColumns.at (column)->get (mRecords.at (index));
        QString qs = qv.toString();
        std::string utf8_text = qs.toUtf8().constData();

        std::cout << "TYPENAME: " << qv.typeName() << ", i = " << i << ", j = " << j << ", value = " << utf8_text << std::endl;;
      }
    }

    // Sample Icon code, placeholder, to-do: texture overlay to icon
    QColor color(255,0,0,100);
    painter.setBrush(color);
    painter.setPen(color);
    painter.drawRect(pixmapimage.rect());

    // Create and return changed Icon
    QIcon brushIcon(pixmapimage);
    return brushIcon;
}

bool CSVRender::TerrainTextureMode::primaryEditStartDrag (const QPoint& pos)
{
    return false;
}

bool CSVRender::TerrainTextureMode::secondaryEditStartDrag (const QPoint& pos)
{
    return false;
}

bool CSVRender::TerrainTextureMode::primarySelectStartDrag (const QPoint& pos)
{
    return false;
}

bool CSVRender::TerrainTextureMode::secondarySelectStartDrag (const QPoint& pos)
{
    return false;
}

void CSVRender::TerrainTextureMode::drag (const QPoint& pos, int diffX, int diffY, double speedFactor) {
}

void CSVRender::TerrainTextureMode::dragCompleted(const QPoint& pos) {
}

void CSVRender::TerrainTextureMode::dragAborted() {
}

void CSVRender::TerrainTextureMode::dragWheel (int diff, double speedFactor) {}

void CSVRender::TerrainTextureMode::dragEnterEvent (QDragEnterEvent *event) {
}

void CSVRender::TerrainTextureMode::dropEvent (QDropEvent *event) {
}

void CSVRender::TerrainTextureMode::dragMoveEvent (QDragMoveEvent *event) {
}

int CSVRender::TerrainTextureMode::getSubModeFromId (const std::string& id) const
{
    return id=="point-brush" ? 0 :
    id=="square-brush" ? 1 :
    id=="circle-brush" ? 2 :
    (id=="custom-brush" ? 3 : 4);
}

void CSVRender::TerrainTextureMode::subModeChanged (const std::string& id)
{
    mSubModeId = id;
    getWorldspaceWidget().abortDrag();
    getWorldspaceWidget().setSubMode (getSubModeFromId (id), Mask_Reference);
}
