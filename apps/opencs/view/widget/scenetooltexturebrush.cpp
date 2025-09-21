#include "scenetooltexturebrush.hpp"

#include <QButtonGroup>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QMargins>
#include <QModelIndex>
#include <QSizePolicy>
#include <QSlider>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <limits>
#include <memory>

#include "scenetool.hpp"

#include <apps/opencs/model/prefs/category.hpp>
#include <apps/opencs/model/prefs/setting.hpp>
#include <apps/opencs/model/world/columns.hpp>
#include <apps/opencs/model/world/commandmacro.hpp>
#include <apps/opencs/model/world/record.hpp>
#include <apps/opencs/view/widget/brushshapes.hpp>
#include <apps/opencs/view/widget/pushbutton.hpp>

#include <components/misc/scalableicon.hpp>

#include "../../model/doc/document.hpp"
#include "../../model/prefs/state.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/idcollection.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/universalid.hpp"

namespace CSVWidget
{
    class SceneToolbar;
}

CSVWidget::BrushSizeControls::BrushSizeControls(const QString& title, QWidget* parent)
    : QGroupBox(title, parent)
    , mLayoutSliderSize(new QHBoxLayout)
    , mBrushSizeSlider(new QSlider(Qt::Horizontal))
    , mBrushSizeSpinBox(new QSpinBox)
{
    mBrushSizeSlider->setTickPosition(QSlider::TicksBothSides);
    mBrushSizeSlider->setTickInterval(10);
    mBrushSizeSlider->setRange(1, CSMPrefs::get()["3D Scene Editing"]["texturebrush-maximumsize"].toInt());
    mBrushSizeSlider->setSingleStep(1);

    mBrushSizeSpinBox->setRange(1, CSMPrefs::get()["3D Scene Editing"]["texturebrush-maximumsize"].toInt());
    mBrushSizeSpinBox->setSingleStep(1);

    mLayoutSliderSize->addWidget(mBrushSizeSlider);
    mLayoutSliderSize->addWidget(mBrushSizeSpinBox);

    connect(mBrushSizeSlider, &QSlider::valueChanged, mBrushSizeSpinBox, &QSpinBox::setValue);
    connect(mBrushSizeSpinBox, qOverload<int>(&QSpinBox::valueChanged), mBrushSizeSlider, &QSlider::setValue);

    setLayout(mLayoutSliderSize);
}

CSVWidget::TextureBrushWindow::TextureBrushWindow(CSMDoc::Document& document, QWidget* parent)
    : QFrame(parent, Qt::Popup)
    , mDocument(document)
{
    mBrushTextureLabel = "Selected texture: " + mBrushTexture.getRefIdString() + " ";

    CSMWorld::IdCollection<ESM::LandTexture>& landtexturesCollection = mDocument.getData().getLandTextures();

    int landTextureFilename = landtexturesCollection.findColumnIndex(CSMWorld::Columns::ColumnId_Texture);
    const int index = landtexturesCollection.searchId(mBrushTexture);

    if (index != -1 && !landtexturesCollection.getRecord(index).isDeleted())
    {
        mSelectedBrush = new QLabel(QString::fromStdString(mBrushTextureLabel)
            + landtexturesCollection.getData(index, landTextureFilename).value<QString>());
    }
    else
    {
        mBrushTextureLabel = "No selected texture or invalid texture";
        mSelectedBrush = new QLabel(QString::fromStdString(mBrushTextureLabel));
    }

    mButtonPoint = new QPushButton(Misc::ScalableIcon::load(":scenetoolbar/brush-point"), "", this);
    mButtonSquare = new QPushButton(Misc::ScalableIcon::load(":scenetoolbar/brush-square"), "", this);
    mButtonCircle = new QPushButton(Misc::ScalableIcon::load(":scenetoolbar/brush-circle"), "", this);
    mButtonCustom = new QPushButton(Misc::ScalableIcon::load(":scenetoolbar/brush-custom"), "", this);

    mSizeSliders = new BrushSizeControls("Brush size", this);

    QVBoxLayout* layoutMain = new QVBoxLayout;
    layoutMain->setSpacing(0);
    layoutMain->setContentsMargins(4, 0, 4, 4);

    QHBoxLayout* layoutHorizontal = new QHBoxLayout;
    layoutHorizontal->setSpacing(0);
    layoutHorizontal->setContentsMargins(QMargins(0, 0, 0, 0));

    configureButtonInitialSettings(mButtonPoint);
    configureButtonInitialSettings(mButtonSquare);
    configureButtonInitialSettings(mButtonCircle);
    configureButtonInitialSettings(mButtonCustom);

    mButtonPoint->setToolTip(toolTipPoint);
    mButtonSquare->setToolTip(toolTipSquare);
    mButtonCircle->setToolTip(toolTipCircle);
    mButtonCustom->setToolTip(toolTipCustom);

    QButtonGroup* brushButtonGroup = new QButtonGroup(this);
    brushButtonGroup->addButton(mButtonPoint);
    brushButtonGroup->addButton(mButtonSquare);
    brushButtonGroup->addButton(mButtonCircle);
    brushButtonGroup->addButton(mButtonCustom);

    brushButtonGroup->setExclusive(true);

    layoutHorizontal->addWidget(mButtonPoint, 0, Qt::AlignTop);
    layoutHorizontal->addWidget(mButtonSquare, 0, Qt::AlignTop);
    layoutHorizontal->addWidget(mButtonCircle, 0, Qt::AlignTop);
    layoutHorizontal->addWidget(mButtonCustom, 0, Qt::AlignTop);

    mHorizontalGroupBox = new QGroupBox(tr(""));
    mHorizontalGroupBox->setLayout(layoutHorizontal);

    layoutMain->addWidget(mHorizontalGroupBox);
    layoutMain->addWidget(mSizeSliders);
    layoutMain->addWidget(mSelectedBrush);

    setLayout(layoutMain);

    connect(mButtonPoint, &QPushButton::clicked, this, &TextureBrushWindow::setBrushShape);
    connect(mButtonSquare, &QPushButton::clicked, this, &TextureBrushWindow::setBrushShape);
    connect(mButtonCircle, &QPushButton::clicked, this, &TextureBrushWindow::setBrushShape);
    connect(mButtonCustom, &QPushButton::clicked, this, &TextureBrushWindow::setBrushShape);
}

void CSVWidget::TextureBrushWindow::configureButtonInitialSettings(QPushButton* button)
{
    button->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    button->setContentsMargins(QMargins(0, 0, 0, 0));
    button->setIconSize(QSize(48 - 6, 48 - 6));
    button->setFixedSize(48, 48);
    button->setCheckable(true);
}

void CSVWidget::TextureBrushWindow::setBrushTexture(ESM::RefId brushTexture)
{
    CSMWorld::IdTable& ltexTable = dynamic_cast<CSMWorld::IdTable&>(
        *mDocument.getData().getTableModel(CSMWorld::UniversalId::Type_LandTextures));
    QUndoStack& undoStack = mDocument.getUndoStack();

    CSMWorld::IdCollection<ESM::LandTexture>& landtexturesCollection = mDocument.getData().getLandTextures();
    int landTextureFilename = landtexturesCollection.findColumnIndex(CSMWorld::Columns::ColumnId_Texture);

    int row = landtexturesCollection.getIndex(brushTexture);
    const auto& record = landtexturesCollection.getRecord(row);

    if (!record.isDeleted())
    {
        // Ensure the texture is defined by the current plugin
        if (!record.isModified())
        {
            CSMWorld::CommandMacro macro(undoStack);
            macro.push(new CSMWorld::TouchCommand(ltexTable, brushTexture.getRefIdString()));
        }
        mBrushTextureLabel = "Selected texture: " + brushTexture.getRefIdString() + " ";
        mSelectedBrush->setText(QString::fromStdString(mBrushTextureLabel)
            + landtexturesCollection.getData(row, landTextureFilename).value<QString>());
    }
    else
    {
        brushTexture = {};
        mBrushTextureLabel = "No selected texture or invalid texture";
        mSelectedBrush->setText(QString::fromStdString(mBrushTextureLabel));
    }

    mBrushTexture = brushTexture;

    emit passTextureId(mBrushTexture);
    emit passBrushShape(mBrushShape); // updates the icon tooltip
}

void CSVWidget::TextureBrushWindow::setBrushSize(int brushSize)
{
    mBrushSize = brushSize;
    emit passBrushSize(mBrushSize);
}

void CSVWidget::TextureBrushWindow::setBrushShape()
{
    if (mButtonPoint->isChecked())
        mBrushShape = CSVWidget::BrushShape_Point;
    if (mButtonSquare->isChecked())
        mBrushShape = CSVWidget::BrushShape_Square;
    if (mButtonCircle->isChecked())
        mBrushShape = CSVWidget::BrushShape_Circle;
    if (mButtonCustom->isChecked())
        mBrushShape = CSVWidget::BrushShape_Custom;
    emit passBrushShape(mBrushShape);
}

void CSVWidget::SceneToolTextureBrush::adjustToolTips() {}

CSVWidget::SceneToolTextureBrush::SceneToolTextureBrush(
    SceneToolbar* parent, const QString& toolTip, CSMDoc::Document& document)
    : SceneTool(parent, Type_TopAction)
    , mToolTip(toolTip)
    , mDocument(document)
    , mTextureBrushWindow(new TextureBrushWindow(document, this))
{
    mBrushHistory.resize(1);

    setAcceptDrops(true);
    connect(mTextureBrushWindow, &TextureBrushWindow::passBrushShape, this, &SceneToolTextureBrush::setButtonIcon);
    setButtonIcon(mTextureBrushWindow->mBrushShape);

    mPanel = new QFrame(this, Qt::Popup);

    QHBoxLayout* layout = new QHBoxLayout(mPanel);

    layout->setContentsMargins(QMargins(0, 0, 0, 0));

    mTable = new QTableWidget(0, 2, this);

    mTable->setShowGrid(true);
    mTable->verticalHeader()->hide();
    mTable->horizontalHeader()->hide();
    mTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    mTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    mTable->setSelectionMode(QAbstractItemView::NoSelection);

    layout->addWidget(mTable);

    connect(mTable, &QTableWidget::clicked, this, &SceneToolTextureBrush::clicked);
}

void CSVWidget::SceneToolTextureBrush::setButtonIcon(CSVWidget::BrushShape brushShape)
{
    QString tooltip = "Change brush settings <p>Currently selected: ";

    switch (brushShape)
    {
        case BrushShape_Point:

            setIcon(Misc::ScalableIcon::load(":scenetoolbar/brush-point"));
            tooltip += mTextureBrushWindow->toolTipPoint;
            break;

        case BrushShape_Square:

            setIcon(Misc::ScalableIcon::load(":scenetoolbar/brush-square"));
            tooltip += mTextureBrushWindow->toolTipSquare;
            break;

        case BrushShape_Circle:

            setIcon(Misc::ScalableIcon::load(":scenetoolbar/brush-circle"));
            tooltip += mTextureBrushWindow->toolTipCircle;
            break;

        case BrushShape_Custom:

            setIcon(Misc::ScalableIcon::load(":scenetoolbar/brush-custom"));
            tooltip += mTextureBrushWindow->toolTipCustom;
            break;
    }

    tooltip += "<p>(right click to access of previously used brush settings)";

    CSMWorld::IdCollection<ESM::LandTexture>& landtexturesCollection = mDocument.getData().getLandTextures();

    int landTextureFilename = landtexturesCollection.findColumnIndex(CSMWorld::Columns::ColumnId_Texture);
    const int index = landtexturesCollection.searchId(mTextureBrushWindow->mBrushTexture);

    if (index != -1 && !landtexturesCollection.getRecord(index).isDeleted())
    {
        tooltip += "<p>Selected texture: " + QString::fromStdString(mTextureBrushWindow->mBrushTexture.getRefIdString())
            + " ";

        tooltip += landtexturesCollection.getData(index, landTextureFilename).value<QString>();
    }
    else
    {
        tooltip += "<p>No selected texture or invalid texture";
    }

    tooltip += "<br>(drop texture here to change)";
    setToolTip(tooltip);
}

void CSVWidget::SceneToolTextureBrush::showPanel(const QPoint& position)
{
    updatePanel();
    mPanel->move(position);
    mPanel->show();
}

void CSVWidget::SceneToolTextureBrush::updatePanel()
{
    mTable->setRowCount(static_cast<int>(mBrushHistory.size()));

    for (size_t i = mBrushHistory.size(); i > 0; --i)
    {
        CSMWorld::IdCollection<ESM::LandTexture>& landtexturesCollection = mDocument.getData().getLandTextures();
        int landTextureFilename = landtexturesCollection.findColumnIndex(CSMWorld::Columns::ColumnId_Texture);
        const int index = landtexturesCollection.searchId(mBrushHistory[i - 1]);
        const int row = static_cast<int>(i - 1);

        if (index != -1 && !landtexturesCollection.getRecord(index).isDeleted())
        {
            mTable->setItem(row, 1,
                new QTableWidgetItem(landtexturesCollection.getData(index, landTextureFilename).value<QString>()));
            mTable->setItem(
                row, 0, new QTableWidgetItem(QString::fromStdString(mBrushHistory[i - 1].getRefIdString())));
        }
        else
        {
            mTable->setItem(row, 1, new QTableWidgetItem("Invalid/deleted texture"));
            mTable->setItem(
                row, 0, new QTableWidgetItem(QString::fromStdString(mBrushHistory[i - 1].getRefIdString())));
        }
    }
}

void CSVWidget::SceneToolTextureBrush::updateBrushHistory(ESM::RefId brushTexture)
{
    mBrushHistory.insert(mBrushHistory.begin(), brushTexture);
    if (mBrushHistory.size() > 5)
        mBrushHistory.pop_back();
}

void CSVWidget::SceneToolTextureBrush::clicked(const QModelIndex& index)
{
    if (index.column() == 0 || index.column() == 1)
    {
        ESM::RefId brushTexture = mBrushHistory[index.row()];
        std::swap(mBrushHistory[index.row()], mBrushHistory[0]);
        mTextureBrushWindow->setBrushTexture(brushTexture);
        emit passTextureId(brushTexture);
        updatePanel();
        mPanel->hide();
    }
}

void CSVWidget::SceneToolTextureBrush::activate()
{
    QPoint position = QCursor::pos();
    mTextureBrushWindow->mSizeSliders->mBrushSizeSlider->setRange(
        1, CSMPrefs::get()["3D Scene Editing"]["texturebrush-maximumsize"].toInt());
    mTextureBrushWindow->mSizeSliders->mBrushSizeSpinBox->setRange(
        1, CSMPrefs::get()["3D Scene Editing"]["texturebrush-maximumsize"].toInt());
    mTextureBrushWindow->move(position);
    mTextureBrushWindow->show();
}

void CSVWidget::SceneToolTextureBrush::dragEnterEvent(QDragEnterEvent* event)
{
    emit passEvent(event);
    event->accept();
}
void CSVWidget::SceneToolTextureBrush::dropEvent(QDropEvent* event)
{
    emit passEvent(event);
    event->accept();
}
