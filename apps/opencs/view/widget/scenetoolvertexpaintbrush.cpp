#include "scenetoolvertexpaintbrush.hpp"

#include <QButtonGroup>
#include <QComboBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QSizePolicy>
#include <QSlider>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

#include <apps/opencs/model/prefs/category.hpp>
#include <apps/opencs/model/prefs/setting.hpp>
#include <apps/opencs/view/widget/pushbutton.hpp>

#include "brushshapes.hpp"
#include "scenetool.hpp"

#include "../../model/prefs/state.hpp"

namespace CSVWidget
{
    class SceneToolbar;
}

namespace CSMDoc
{
    class Document;
}

CSVWidget::VertexPaintBrushSizeControls::VertexPaintBrushSizeControls(const QString& title, QWidget* parent)
    : QGroupBox(title, parent)
{
    mBrushSizeSlider->setTickPosition(QSlider::TicksBothSides);
    mBrushSizeSlider->setTickInterval(10);
    mBrushSizeSlider->setRange(1, CSMPrefs::get()["3D Scene Editing"]["shapebrush-maximumsize"].toInt());
    mBrushSizeSlider->setSingleStep(1);

    mBrushSizeSpinBox->setRange(1, CSMPrefs::get()["3D Scene Editing"]["shapebrush-maximumsize"].toInt());
    mBrushSizeSpinBox->setSingleStep(1);

    QHBoxLayout* layoutSliderSize = new QHBoxLayout;
    layoutSliderSize->addWidget(mBrushSizeSlider);
    layoutSliderSize->addWidget(mBrushSizeSpinBox);

    connect(mBrushSizeSlider, &QSlider::valueChanged, mBrushSizeSpinBox, &QSpinBox::setValue);
    connect(mBrushSizeSpinBox, qOverload<int>(&QSpinBox::valueChanged), mBrushSizeSlider, &QSlider::setValue);

    setLayout(layoutSliderSize);
}

CSVWidget::VertexPaintBrushWindow::VertexPaintBrushWindow(CSMDoc::Document& document, QWidget* parent)
    : QFrame(parent, Qt::Popup)
    , mDocument(document)
{
    mButtonPoint = new QPushButton(QIcon(QPixmap(":scenetoolbar/brush-point")), "", this);
    mButtonSquare = new QPushButton(QIcon(QPixmap(":scenetoolbar/brush-square")), "", this);
    mButtonCircle = new QPushButton(QIcon(QPixmap(":scenetoolbar/brush-circle")), "", this);

    mSizeSliders = new VertexPaintBrushSizeControls("Brush size", this);

    QVBoxLayout* layoutMain = new QVBoxLayout;
    layoutMain->setSpacing(0);
    layoutMain->setContentsMargins(4, 0, 4, 4);

    QHBoxLayout* layoutHorizontal = new QHBoxLayout;
    layoutHorizontal->setSpacing(0);
    layoutHorizontal->setContentsMargins(QMargins(0, 0, 0, 0));

    configureButtonInitialSettings(mButtonPoint);
    configureButtonInitialSettings(mButtonSquare);
    configureButtonInitialSettings(mButtonCircle);

    mButtonPoint->setToolTip(toolTipPoint);
    mButtonSquare->setToolTip(toolTipSquare);
    mButtonCircle->setToolTip(toolTipCircle);

    QButtonGroup* brushButtonGroup = new QButtonGroup(this);
    brushButtonGroup->addButton(mButtonPoint);
    brushButtonGroup->addButton(mButtonSquare);
    brushButtonGroup->addButton(mButtonCircle);

    brushButtonGroup->setExclusive(true);

    layoutHorizontal->addWidget(mButtonPoint, 0, Qt::AlignTop);
    layoutHorizontal->addWidget(mButtonSquare, 0, Qt::AlignTop);
    layoutHorizontal->addWidget(mButtonCircle, 0, Qt::AlignTop);

    mHorizontalGroupBox = new QGroupBox(tr(""));
    mHorizontalGroupBox->setLayout(layoutHorizontal);

    mToolSelector = new QComboBox(this);
    mToolSelector->addItem(tr("Replace"));
    // TOOD: in the future could add types like smooth blend, multiply etc

    QLabel* colorLabel = new QLabel(this);
    colorLabel->setText("Color:");
    mColorButtonWidget = new ColorButtonWidget();

    layoutMain->addWidget(mHorizontalGroupBox);
    layoutMain->addWidget(mSizeSliders);
    layoutMain->addWidget(mToolSelector);
    layoutMain->addWidget(colorLabel);
    layoutMain->addWidget(mColorButtonWidget);

    setLayout(layoutMain);

    connect(mButtonPoint, &QPushButton::clicked, this, &VertexPaintBrushWindow::setBrushShape);
    connect(mButtonSquare, &QPushButton::clicked, this, &VertexPaintBrushWindow::setBrushShape);
    connect(mButtonCircle, &QPushButton::clicked, this, &VertexPaintBrushWindow::setBrushShape);
}

void CSVWidget::VertexPaintBrushWindow::configureButtonInitialSettings(QPushButton* button)
{
    button->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    button->setContentsMargins(QMargins(0, 0, 0, 0));
    button->setIconSize(QSize(48 - 6, 48 - 6));
    button->setFixedSize(48, 48);
    button->setCheckable(true);
}

void CSVWidget::VertexPaintBrushWindow::setBrushSize(int brushSize)
{
    mBrushSize = brushSize;
    emit passBrushSize(mBrushSize);
}

void CSVWidget::VertexPaintBrushWindow::setBrushShape()
{
    if (mButtonPoint->isChecked())
        mBrushShape = BrushShape_Point;
    if (mButtonSquare->isChecked())
        mBrushShape = BrushShape_Square;
    if (mButtonCircle->isChecked())
        mBrushShape = BrushShape_Circle;
    emit passBrushShape(mBrushShape);
}

void CSVWidget::SceneToolVertexPaintBrush::adjustToolTips() {}

CSVWidget::SceneToolVertexPaintBrush::SceneToolVertexPaintBrush(
    SceneToolbar* parent, const QString& toolTip, CSMDoc::Document& document)
    : SceneTool(parent, Type_TopAction)
    , mToolTip(toolTip)
    , mDocument(document)
    , mVertexPaintBrushWindow(new VertexPaintBrushWindow(document, this))
{
    setAcceptDrops(true);
    connect(mVertexPaintBrushWindow, &VertexPaintBrushWindow::passBrushShape, this,
        &SceneToolVertexPaintBrush::setButtonIcon);
    setButtonIcon(mVertexPaintBrushWindow->mBrushShape);

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

    connect(mTable, &QTableWidget::clicked, this, &SceneToolVertexPaintBrush::clicked);
}

void CSVWidget::SceneToolVertexPaintBrush::setButtonIcon(CSVWidget::BrushShape brushShape)
{
    QString tooltip = "Change brush settings <p>Currently selected: ";

    switch (brushShape)
    {
        case BrushShape_Point:

            setIcon(QIcon(QPixmap(":scenetoolbar/brush-point")));
            tooltip += mVertexPaintBrushWindow->toolTipPoint;
            break;

        case BrushShape_Square:

            setIcon(QIcon(QPixmap(":scenetoolbar/brush-square")));
            tooltip += mVertexPaintBrushWindow->toolTipSquare;
            break;

        case BrushShape_Circle:

            setIcon(QIcon(QPixmap(":scenetoolbar/brush-circle")));
            tooltip += mVertexPaintBrushWindow->toolTipCircle;
            break;
    }

    setToolTip(tooltip);
}

void CSVWidget::SceneToolVertexPaintBrush::showPanel(const QPoint& position) {}

void CSVWidget::SceneToolVertexPaintBrush::updatePanel() {}

void CSVWidget::SceneToolVertexPaintBrush::clicked(const QModelIndex& index) {}

void CSVWidget::SceneToolVertexPaintBrush::activate()
{
    QPoint position = QCursor::pos();
    mVertexPaintBrushWindow->mSizeSliders->mBrushSizeSlider->setRange(
        1, CSMPrefs::get()["3D Scene Editing"]["shapebrush-maximumsize"].toInt());
    mVertexPaintBrushWindow->mSizeSliders->mBrushSizeSpinBox->setRange(
        1, CSMPrefs::get()["3D Scene Editing"]["shapebrush-maximumsize"].toInt());
    mVertexPaintBrushWindow->move(position);
    mVertexPaintBrushWindow->show();
}

void CSVWidget::SceneToolVertexPaintBrush::dragEnterEvent(QDragEnterEvent* event)
{
    emit passEvent(event);
    event->accept();
}
void CSVWidget::SceneToolVertexPaintBrush::dropEvent(QDropEvent* event)
{
    emit passEvent(event);
    event->accept();
}
