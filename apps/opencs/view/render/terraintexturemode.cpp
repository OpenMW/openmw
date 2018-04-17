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

#include "pagedworldspacewidget.hpp"
#include "mask.hpp"
#include "object.hpp" // Something small needed regarding pointers from here ()
#include "worldspacewidget.hpp"

#include "../../model/doc/document.hpp"
#include "../../model/world/columnbase.hpp"
#include "../../model/world/commandmacro.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/land.hpp"
#include "../../model/world/landtexture.hpp"
#include "../../model/world/resourcetable.hpp"
#include "../../model/world/tablemimedata.hpp"
#include "../../model/world/universalid.hpp"


#include <components/esm/loadland.hpp>


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

    connect(buttonPoint, SIGNAL(clicked()), this, SLOT(setBrushShape()));
    connect(buttonSquare, SIGNAL(clicked()), this, SLOT(setBrushShape()));
    connect(buttonCircle, SIGNAL(clicked()), this, SLOT(setBrushShape()));
    connect(buttonCustom, SIGNAL(clicked()), this, SLOT(setBrushShape()));

    connect(sizeSliders->brushSizeSlider, SIGNAL(valueChanged(int)), parent, SLOT(setBrushSize(int)));

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

void CSVRender::TextureBrushWindow::setBrushSize(int brushSize)
{
    mBrushSize = brushSize;
    emit passBrushSize(mBrushSize);
}

void CSVRender::TextureBrushWindow::setBrushShape()
{
    if(buttonPoint->isChecked()) mBrushShape = 0;
    if(buttonSquare->isChecked()) mBrushShape = 1;
    if(buttonCircle->isChecked()) mBrushShape = 2;
    if(buttonCustom->isChecked()) mBrushShape = 3;
    emit passBrushShape(mBrushShape);
}

CSVRender::TerrainTextureMode::TerrainTextureMode (WorldspaceWidget *worldspaceWidget, QWidget *parent)
: EditMode (worldspaceWidget, QIcon {":scenetoolbar/editing-terrain-texture"}, Mask_Terrain | Mask_Reference, "Terrain texture editing", parent)
, textureBrushWindow(new TextureBrushWindow(worldspaceWidget, this))
{
    connect(parent, SIGNAL(passEvent(QDragEnterEvent*)), this, SLOT(handleDragEnterEvent(QDragEnterEvent*)));
    connect(parent, SIGNAL(passEvent(QDropEvent*)), this, SLOT(handleDropEvent(QDropEvent*)));
    connect(textureBrushWindow, SIGNAL(passBrushSize(int)), this, SLOT(setBrushSize(int)));
    connect(textureBrushWindow, SIGNAL(passBrushShape(int)), this, SLOT(setBrushShape(int)));
}

void CSVRender::TerrainTextureMode::activate(CSVWidget::SceneToolbar* toolbar)
{
    EditMode::activate(toolbar);
}

void CSVRender::TerrainTextureMode::deactivate(CSVWidget::SceneToolbar* toolbar)
{
    EditMode::deactivate(toolbar);
}

void CSVRender::TerrainTextureMode::primaryEditPressed(const WorldspaceHitResult& hit) // Apply changes here
{
    editTerrainTextureGrid(hit);
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
    WorldspaceHitResult hit = getWorldspaceWidget().mousePick (pos, getWorldspaceWidget().getInteractionMask());
    editTerrainTextureGrid(hit);
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

void CSVRender::TerrainTextureMode::editTerrainTextureGrid(const WorldspaceHitResult& hit)
{
    mCellId = getWorldspaceWidget().getCellId (hit.worldPos);
    std::string hash = "#";
    std::string space = " ";
    std::size_t hashlocation = mCellId.find(hash);
    std::size_t spacelocation = mCellId.find(space);
    std::string slicedX = mCellId.substr (hashlocation+1, spacelocation-hashlocation);
    std::string slicedY = mCellId.substr (spacelocation+1);
    CSMWorld::CellCoordinates mCoordinates(stoi(slicedX), stoi(slicedY));

    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    CSMWorld::IdTable& landTable = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_Land));
    CSMWorld::IdTable& ltexTable = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_LandTextures));

    int textureColumn = landTable.findColumnIndex(CSMWorld::Columns::ColumnId_LandTexturesIndex);
    CSMWorld::LandTexturesColumn::DataType mPointer = landTable.data(landTable.getModelIndex(mCellId, textureColumn)).value<CSMWorld::LandTexturesColumn::DataType>();
    CSMWorld::LandTexturesColumn::DataType mNew(mPointer);

    int xInCell ((hit.worldPos.x() - (stoi(slicedX)* cellSize)) * landTextureSize / cellSize);
    int yInCell ((hit.worldPos.y() - (stoi(slicedY)* cellSize)) * landTextureSize / cellSize);

    hashlocation = mBrushTexture.find(hash);
    std::string mBrushTextureInt = mBrushTexture.substr (hashlocation+1);
    int brushInt = stoi(mBrushTexture.substr (hashlocation+1))+1; // All indices are offset by +1

    if (mBrushShape == 0) mNew[yInCell*landTextureSize+xInCell] = brushInt;
    if (mBrushShape == 1)
    {
        for(int i=-mBrushSize/2;i<mBrushSize/2;i++)
        {
            for(int j=-mBrushSize/2;j<mBrushSize/2;j++)
            {
                if (xInCell+i >= 0 && yInCell+j >= 0 && xInCell+i <= 15 && yInCell+j <= 15)
                    mNew[(yInCell+j)*landTextureSize+(xInCell+i)] = brushInt;
            }
        }
    }
    float distance = 0;
    if (mBrushShape == 2)
    {
        float rf = mBrushSize/2;
        int r = (mBrushSize/2)+1;
        for(int i = -r; i < r; i++)
        {
            for(int j = -r; j < r; j++)
            {
                distance = std::round(sqrt(pow((xInCell+i)-xInCell, 2) + pow((yInCell+j)-yInCell, 2)));
                if (xInCell+i >= 0 && yInCell+j >= 0 && xInCell+i <= 15 && yInCell+j <= 15 && distance <= rf)
                    mNew[(yInCell+j)*landTextureSize+(xInCell+i)] = brushInt;
            }
        }
    }
    if (mBrushShape == 3)
    {
      // Not implemented
    }

    QVariant changedLand;
    changedLand.setValue(mNew);

    CSMWorld::CommandMacro macro (document.getUndoStack(), "Edit texture records");
    QModelIndex index(landTable.getModelIndex (mCellId, landTable.findColumnIndex (CSMWorld::Columns::ColumnId_LandTexturesIndex)));

    CSMWorld::TouchLandCommand* ltexTouch = new CSMWorld::TouchLandCommand(landTable, ltexTable, mCellId);
    CSMWorld::ModifyCommand* ltexModify = new CSMWorld::ModifyCommand(landTable, index, changedLand);
    macro.push (ltexTouch);
    macro.push (ltexModify);
}

void CSVRender::TerrainTextureMode::dragMoveEvent (QDragMoveEvent *event) {
}

void CSVRender::TerrainTextureMode::setBrushSize(int brushSize)
{
    mBrushSize = brushSize;
}

void CSVRender::TerrainTextureMode::setBrushShape(int brushShape)
{
    mBrushShape = brushShape;
}
