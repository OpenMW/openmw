// To-do: Getting Texture Id and Texture Filenames to base class Terraintexturemode
// To-do: Better data handling options for mBrushTexture
// To-do: loading texture bitmaps from virtual file system (vfs) for texture overlay icon

#include "terraintexturemode.hpp"
#include "editmode.hpp"

#include <string>

#include <QWidget>
#include <QIcon>
#include <QSpinBox>
#include <QGroupBox>
#include <QSlider>
#include <QEvent>
#include <QDropEvent>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDragEnterEvent>
#include <QDrag>

#include "../widget/modebutton.hpp"
#include "../widget/scenetoolbar.hpp"

#include "../../model/world/universalid.hpp"
#include "../../model/world/tablemimedata.hpp"

#include "pagedworldspacewidget.hpp"


CSVRender::BrushSizeControls::BrushSizeControls(const QString &title, QWidget *parent)
    : QGroupBox(title, parent)
{
    brushSizeSlider = new QSlider(Qt::Horizontal);
    brushSizeSlider->setTickPosition(QSlider::TicksBothSides);
    brushSizeSlider->setTickInterval(10);
    brushSizeSlider->setSingleStep(1);

    brushSizeSpinBox = new QSpinBox;
    brushSizeSpinBox->setRange(1, 100);
    brushSizeSpinBox->setSingleStep(1);

    layoutSliderSize = new QHBoxLayout;
    layoutSliderSize->addWidget(brushSizeSlider);
    layoutSliderSize->addWidget(brushSizeSpinBox);

    connect(brushSizeSlider, SIGNAL(valueChanged(int)), brushSizeSpinBox, SLOT(setValue(int)));
    connect(brushSizeSpinBox, SIGNAL(valueChanged(int)), brushSizeSlider, SLOT(setValue(int)));

    setLayout(layoutSliderSize);
}

CSVRender::TextureBrushButton::TextureBrushButton (const QIcon & icon, const QString & text, QWidget * parent)
    : QPushButton(icon, text, parent)
{
}

CSVRender::TextureBrushWindow::TextureBrushWindow(WorldspaceWidget *worldspaceWidget, QWidget *parent)
    : QFrame(parent, Qt::Popup), mWorldspaceWidget (worldspaceWidget)
{
    mBrushTextureLabel = "Brush: " + mBrushTexture;
    selectedBrush = new QLabel(QString::fromUtf8(mBrushTextureLabel.c_str()), this);

    const std::string& iconPoint = ":scenetoolbar/brush-point";
    const std::string& iconSquare = ":scenetoolbar/brush-square";
    const std::string& iconCircle = ":scenetoolbar/brush-circle";
    const std::string& iconCustom = ":scenetoolbar/brush-custom";

    TextureBrushButton *buttonPoint = new TextureBrushButton(QIcon (QPixmap (iconPoint.c_str())), "", this);
    TextureBrushButton *buttonSquare = new TextureBrushButton(QIcon (QPixmap (iconSquare.c_str())), "", this);
    TextureBrushButton *buttonCircle = new TextureBrushButton(QIcon (QPixmap (iconCircle.c_str())), "", this);
    TextureBrushButton *buttonCustom = new TextureBrushButton(QIcon (QPixmap (iconCustom.c_str())), "", this);

    QVBoxLayout *layoutMain = new QVBoxLayout;
    layoutMain->setSpacing(0);

    QHBoxLayout *layoutHorizontal = new QHBoxLayout;
    layoutHorizontal->setContentsMargins (QMargins (0, 0, 0, 0));
    layoutHorizontal->setSpacing(0);

    configureButtonInitialSettings(buttonPoint);
    configureButtonInitialSettings(buttonSquare);
    configureButtonInitialSettings(buttonCircle);
    configureButtonInitialSettings(buttonCustom);

    QButtonGroup* brushButtonGroup = new QButtonGroup(this);
    brushButtonGroup->addButton(buttonPoint);
    brushButtonGroup->addButton(buttonSquare);
    brushButtonGroup->addButton(buttonCircle);
    brushButtonGroup->addButton(buttonCustom);

    brushButtonGroup->setExclusive(true);

    layoutHorizontal->addWidget(buttonPoint);
    layoutHorizontal->addWidget(buttonSquare);
    layoutHorizontal->addWidget(buttonCircle);
    layoutHorizontal->addWidget(buttonCustom);

    horizontalGroupBox = new QGroupBox(tr(""));
    horizontalGroupBox->setLayout(layoutHorizontal);

    BrushSizeControls* sizeSliders = new BrushSizeControls(tr(""), this);

    layoutMain->addWidget(horizontalGroupBox);
    layoutMain->addWidget(sizeSliders);
    layoutMain->addWidget(selectedBrush);

    setLayout(layoutMain);

    connect(parent, SIGNAL(passBrushTexture(std::string)), this, SLOT(getBrushTexture(std::string)));
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
    selectedBrush->setText(QString::fromUtf8(mBrushTextureLabel.c_str()));
}

CSVRender::TerrainTextureMode::TerrainTextureMode (WorldspaceWidget *worldspaceWidget, QWidget *parent)
: EditMode (worldspaceWidget, QIcon {":scenetoolbar/editing-terrain-texture"}, Mask_Terrain | Mask_Reference, "Terrain texture editing", parent)
, textureBrushWindow(new TextureBrushWindow(worldspaceWidget, this))
{
    connect(parent, SIGNAL(passEvent(QDragEnterEvent*)), this, SLOT(handleDragEnterEvent(QDragEnterEvent*)));
    connect(parent, SIGNAL(passEvent(QDropEvent*)), this, SLOT(handleDropEvent(QDropEvent*)));
}

void CSVRender::TerrainTextureMode::activate(CSVWidget::SceneToolbar* toolbar)
{
    EditMode::activate(toolbar);
}

void CSVRender::TerrainTextureMode::deactivate(CSVWidget::SceneToolbar* toolbar)
{
    EditMode::deactivate(toolbar);
}

void CSVRender::TerrainTextureMode::primarySelectPressed(const WorldspaceHitResult& hit)
{
      QPoint position = QCursor::pos();
      textureBrushWindow->move (position);
      textureBrushWindow->show();
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

void CSVRender::TerrainTextureMode::handleDragEnterEvent (QDragEnterEvent *event) {
    event->accept();
}

void CSVRender::TerrainTextureMode::handleDropEvent (QDropEvent *event) {
  const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData());

  if (!mime) // May happen when non-records (e.g. plain text) are dragged and dropped
      return;

  if (mime->holdsType (CSMWorld::UniversalId::Type_LandTexture))
  {
      const std::vector<CSMWorld::UniversalId> ids = mime->getData();

      for (const CSMWorld::UniversalId& uid : ids)
      {
          mBrushTexture = uid.getId();
          emit passBrushTexture(mBrushTexture);
      }
  }
}

void CSVRender::TerrainTextureMode::dragMoveEvent (QDragMoveEvent *event) {
}
