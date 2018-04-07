// To-do: Getting Texture Id and Texture Filenames to base class Terraintexturemode
// To-do: Better data handling options for mBrushTexture
// To-do: loading texture bitmaps from virtual file system (vfs) for texture overlay icon

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

CSVRender::TextureBrushButton::TextureBrushButton (const QIcon & icon, const QString & text, QWidget * parent)
    : QPushButton(icon, text, parent)
{
}

void CSVRender::TextureBrushButton::dragEnterEvent (QDragEnterEvent *event)
{
  event->accept();
}

void CSVRender::TextureBrushButton::dropEvent (QDropEvent *event)
{
  const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData());

  if (!mime) // May happen when non-records (e.g. plain text) are dragged and dropped
      return;

  if (mime->holdsType (CSMWorld::UniversalId::Type_LandTexture))
  {
      const std::vector<CSMWorld::UniversalId> ids = mime->getData();

      for (const CSMWorld::UniversalId& uid : ids)
      {
          std::string mBrushTexture(uid.getId());
          emit passBrushTexture(mBrushTexture);
      }
  }
      else
}

CSVRender::TextureBrushWindow::TextureBrushWindow(WorldspaceWidget *worldspaceWidget, QWidget *parent)
    : QWidget(parent), mWorldspaceWidget (worldspaceWidget)
{
    //Get landtexture-data via document
    CSMDoc::Document& document = mWorldspaceWidget->getDocument();
    CSMWorld::IdTable& ltexs = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_LandTextures));

    mBrushTextureLabel = "Brush: " + mBrushTexture;
    label = new QLabel(QString::fromUtf8(mBrushTextureLabel.c_str()), this);

    const std::string& iconPoint = ":scenetoolbar/brush-point";
    const std::string& iconSquare = ":scenetoolbar/brush-square";
    const std::string& iconCircle = ":scenetoolbar/brush-circle";
    const std::string& iconCustom = ":scenetoolbar/brush-custom";

    TextureBrushButton *button1 = new TextureBrushButton(QIcon (QPixmap (iconPoint.c_str())), "", this);
    TextureBrushButton *button2 = new TextureBrushButton(QIcon (QPixmap (iconSquare.c_str())), "", this);
    TextureBrushButton *button3 = new TextureBrushButton(QIcon (QPixmap (iconCircle.c_str())), "", this);
    TextureBrushButton *button4 = new TextureBrushButton(QIcon (QPixmap (iconCustom.c_str())), "", this);

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

    connect(button1, SIGNAL(passBrushTexture(std::string)), this, SLOT(getBrushTexture(std::string)));
    connect(button2, SIGNAL(passBrushTexture(std::string)), this, SLOT(getBrushTexture(std::string)));
    connect(button3, SIGNAL(passBrushTexture(std::string)), this, SLOT(getBrushTexture(std::string)));
    connect(button4, SIGNAL(passBrushTexture(std::string)), this, SLOT(getBrushTexture(std::string)));

    setWindowFlags(Qt::Window);
}

void CSVRender::TextureBrushWindow::configureButtonInitialSettings(TextureBrushButton *button)
{
  button->setSizePolicy (QSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed));
  button->setContentsMargins (QMargins (0, 0, 0, 0));
  button->setIconSize (QSize (48-6, 48-6));
  button->setFixedSize (48, 48);
  button->setAcceptDrops(true);
  button->setCheckable(true);
}

void CSVRender::TextureBrushWindow::getBrushTexture(std::string brushTexture)
{
    mBrushTexture = brushTexture;
    mBrushTextureLabel = "Brush:" + mBrushTexture;
    label->setText(QString::fromUtf8(mBrushTextureLabel.c_str()));
}

// This function should eventually load brush texture as bitmap and set it as overlay
// Currently only holds very basic code for icon drawing and landtexture loading
QIcon CSVRender::TerrainTextureMode::drawIconTexture()
{
    // Just use brush-square until feature is developed
    QPixmap pixmapimage = QPixmap(":scenetoolbar/brush-square");
    QPainter painter(&pixmapimage);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);

    //Load Land Texture Data
    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    CSMWorld::IdTable& ltexs = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_LandTextures));

    // Sample Icon code, placeholder, to-do: texture overlay to icon
    QColor color(255,0,0,100);
    painter.setBrush(color);
    painter.setPen(color);
    painter.drawRect(pixmapimage.rect());

    // Create and return changed Icon
    QIcon brushIcon(pixmapimage);
    return brushIcon;
}

CSVRender::TerrainTextureMode::TerrainTextureMode (WorldspaceWidget *worldspaceWidget, QWidget *parent)
: EditMode (worldspaceWidget, QIcon {":scenetoolbar/editing-terrain-texture"}, Mask_Terrain | Mask_Reference, "Terrain texture editing", parent)
, textureBrushWindow(new TextureBrushWindow(worldspaceWidget, this))
{
    setAcceptDrops(true);
}

void CSVRender::TerrainTextureMode::activate(CSVWidget::SceneToolbar* toolbar)
{
    EditMode::activate(toolbar);

    textureBrushWindow->setWindowTitle("Texture brush settings");
    textureBrushWindow->show();
}

void CSVRender::TerrainTextureMode::deactivate(CSVWidget::SceneToolbar* toolbar)
{
    EditMode::deactivate(toolbar);
}

void CSVRender::TerrainTextureMode::primarySelectPressed(const WorldspaceHitResult& hit)
{
}

void CSVRender::TerrainTextureMode::secondarySelectPressed(const WorldspaceHitResult& hit)
{
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
