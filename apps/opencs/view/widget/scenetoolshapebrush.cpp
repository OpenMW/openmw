#include "scenetoolshapebrush.hpp"

#include <QFrame>
#include <QIcon>
#include <QTableWidget>
#include <QHBoxLayout>

#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QSlider>
#include <QEvent>
#include <QDropEvent>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QDragEnterEvent>
#include <QDrag>
#include <QTableWidget>
#include <QHeaderView>
#include <QApplication>
#include <QSizePolicy>

#include "brushshapes.hpp"
#include "scenetool.hpp"

#include "../../model/doc/document.hpp"
#include "../../model/prefs/state.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/idcollection.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/landtexture.hpp"
#include "../../model/world/universalid.hpp"


CSVWidget::ShapeBrushSizeControls::ShapeBrushSizeControls(const QString &title, QWidget *parent)
    : QGroupBox(title, parent)
{
    mBrushSizeSlider->setTickPosition(QSlider::TicksBothSides);
    mBrushSizeSlider->setTickInterval(10);
    mBrushSizeSlider->setRange(1, CSMPrefs::get()["3D Scene Editing"]["shapebrush-maximumsize"].toInt());
    mBrushSizeSlider->setSingleStep(1);

    mBrushSizeSpinBox->setRange(1, CSMPrefs::get()["3D Scene Editing"]["shapebrush-maximumsize"].toInt());
    mBrushSizeSpinBox->setSingleStep(1);

    QHBoxLayout *layoutSliderSize = new QHBoxLayout;
    layoutSliderSize->addWidget(mBrushSizeSlider);
    layoutSliderSize->addWidget(mBrushSizeSpinBox);

    connect(mBrushSizeSlider, SIGNAL(valueChanged(int)), mBrushSizeSpinBox, SLOT(setValue(int)));
    connect(mBrushSizeSpinBox, SIGNAL(valueChanged(int)), mBrushSizeSlider, SLOT(setValue(int)));

    setLayout(layoutSliderSize);
}

CSVWidget::ShapeBrushWindow::ShapeBrushWindow(CSMDoc::Document& document, QWidget *parent)
    : QFrame(parent, Qt::Popup),
    mDocument(document)
{
    mButtonPoint = new QPushButton(QIcon (QPixmap (":scenetoolbar/brush-point")), "", this);
    mButtonSquare = new QPushButton(QIcon (QPixmap (":scenetoolbar/brush-square")), "", this);
    mButtonCircle = new QPushButton(QIcon (QPixmap (":scenetoolbar/brush-circle")), "", this);
    mButtonCustom = new QPushButton(QIcon (QPixmap (":scenetoolbar/brush-custom")), "", this);

    mSizeSliders = new ShapeBrushSizeControls("Brush size", this);

    QVBoxLayout *layoutMain = new QVBoxLayout;
    layoutMain->setSpacing(0);
    layoutMain->setContentsMargins(4,0,4,4);

    QHBoxLayout *layoutHorizontal = new QHBoxLayout;
    layoutHorizontal->setSpacing(0);
    layoutHorizontal->setContentsMargins (QMargins (0, 0, 0, 0));

    configureButtonInitialSettings(mButtonPoint);
    configureButtonInitialSettings(mButtonSquare);
    configureButtonInitialSettings(mButtonCircle);
    configureButtonInitialSettings(mButtonCustom);

    mButtonPoint->setToolTip (toolTipPoint);
    mButtonSquare->setToolTip (toolTipSquare);
    mButtonCircle->setToolTip (toolTipCircle);
    mButtonCustom->setToolTip (toolTipCustom);

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

    mToolSelector = new QComboBox(this);
    mToolSelector->addItem(tr("Height (drag)"));
    mToolSelector->addItem(tr("Height, raise (paint)"));
    mToolSelector->addItem(tr("Height, lower (paint)"));
    mToolSelector->addItem(tr("Smooth (paint)"));
    mToolSelector->addItem(tr("Flatten (paint)"));

    QLabel *brushStrengthLabel = new QLabel(this);
    brushStrengthLabel->setText("Brush strength:");

    mToolStrengthSlider = new QSlider(Qt::Horizontal);
    mToolStrengthSlider->setTickPosition(QSlider::TicksBothSides);
    mToolStrengthSlider->setTickInterval(8);
    mToolStrengthSlider->setRange(8, 128);
    mToolStrengthSlider->setSingleStep(8);
    mToolStrengthSlider->setValue(8);

    layoutMain->addWidget(mHorizontalGroupBox);
    layoutMain->addWidget(mSizeSliders);
    layoutMain->addWidget(mToolSelector);
    layoutMain->addWidget(brushStrengthLabel);
    layoutMain->addWidget(mToolStrengthSlider);

    setLayout(layoutMain);

    connect(mButtonPoint, SIGNAL(clicked()), this, SLOT(setBrushShape()));
    connect(mButtonSquare, SIGNAL(clicked()), this, SLOT(setBrushShape()));
    connect(mButtonCircle, SIGNAL(clicked()), this, SLOT(setBrushShape()));
    connect(mButtonCustom, SIGNAL(clicked()), this, SLOT(setBrushShape()));
}

void CSVWidget::ShapeBrushWindow::configureButtonInitialSettings(QPushButton *button)
{
  button->setSizePolicy (QSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed));
  button->setContentsMargins (QMargins (0, 0, 0, 0));
  button->setIconSize (QSize (48-6, 48-6));
  button->setFixedSize (48, 48);
  button->setCheckable(true);
}

void CSVWidget::ShapeBrushWindow::setBrushSize(int brushSize)
{
    mBrushSize = brushSize;
    emit passBrushSize(mBrushSize);
}

void CSVWidget::ShapeBrushWindow::setBrushShape()
{
    if(mButtonPoint->isChecked()) mBrushShape = BrushShape_Point;
    if(mButtonSquare->isChecked()) mBrushShape = BrushShape_Square;
    if(mButtonCircle->isChecked()) mBrushShape = BrushShape_Circle;
    if(mButtonCustom->isChecked()) mBrushShape = BrushShape_Custom;
    emit passBrushShape(mBrushShape);
}

void CSVWidget::SceneToolShapeBrush::adjustToolTips()
{
}

CSVWidget::SceneToolShapeBrush::SceneToolShapeBrush (SceneToolbar *parent, const QString& toolTip, CSMDoc::Document& document)
: SceneTool (parent, Type_TopAction),
    mToolTip (toolTip),
    mDocument (document),
    mShapeBrushWindow(new ShapeBrushWindow(document, this))
{
    setAcceptDrops(true);
    connect(mShapeBrushWindow, SIGNAL(passBrushShape(CSVWidget::BrushShape)), this, SLOT(setButtonIcon(CSVWidget::BrushShape)));
    setButtonIcon(mShapeBrushWindow->mBrushShape);

    mPanel = new QFrame (this, Qt::Popup);

    QHBoxLayout *layout = new QHBoxLayout (mPanel);

    layout->setContentsMargins (QMargins (0, 0, 0, 0));

    mTable = new QTableWidget (0, 2, this);

    mTable->setShowGrid (true);
    mTable->verticalHeader()->hide();
    mTable->horizontalHeader()->hide();
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    mTable->horizontalHeader()->setSectionResizeMode (0, QHeaderView::Stretch);
    mTable->horizontalHeader()->setSectionResizeMode (1, QHeaderView::Stretch);
#else
    mTable->horizontalHeader()->setResizeMode (0, QHeaderView::Stretch);
    mTable->horizontalHeader()->setResizeMode (1, QHeaderView::Stretch);
#endif
    mTable->setSelectionMode (QAbstractItemView::NoSelection);

    layout->addWidget (mTable);

    connect (mTable, SIGNAL (clicked (const QModelIndex&)),
        this, SLOT (clicked (const QModelIndex&)));

}

void CSVWidget::SceneToolShapeBrush::setButtonIcon (CSVWidget::BrushShape brushShape)
{
    QString tooltip = "Change brush settings <p>Currently selected: ";

    switch (brushShape)
    {
        case BrushShape_Point:

            setIcon (QIcon (QPixmap (":scenetoolbar/brush-point")));
            tooltip += mShapeBrushWindow->toolTipPoint;
            break;

        case BrushShape_Square:

            setIcon (QIcon (QPixmap (":scenetoolbar/brush-square")));
            tooltip += mShapeBrushWindow->toolTipSquare;
            break;

        case BrushShape_Circle:

            setIcon (QIcon (QPixmap (":scenetoolbar/brush-circle")));
            tooltip += mShapeBrushWindow->toolTipCircle;
            break;

        case BrushShape_Custom:

            setIcon (QIcon (QPixmap (":scenetoolbar/brush-custom")));
            tooltip += mShapeBrushWindow->toolTipCustom;
            break;
    }

    setToolTip (tooltip);
}

void CSVWidget::SceneToolShapeBrush::showPanel (const QPoint& position)
{
}

void CSVWidget::SceneToolShapeBrush::updatePanel ()
{
}

void CSVWidget::SceneToolShapeBrush::clicked (const QModelIndex& index)
{
}

void CSVWidget::SceneToolShapeBrush::activate ()
{
    QPoint position = QCursor::pos();
    mShapeBrushWindow->mSizeSliders->mBrushSizeSlider->setRange(1, CSMPrefs::get()["3D Scene Editing"]["shapebrush-maximumsize"].toInt());
    mShapeBrushWindow->mSizeSliders->mBrushSizeSpinBox->setRange(1, CSMPrefs::get()["3D Scene Editing"]["shapebrush-maximumsize"].toInt());
    mShapeBrushWindow->move (position);
    mShapeBrushWindow->show();
}

void CSVWidget::SceneToolShapeBrush::dragEnterEvent (QDragEnterEvent *event)
{
    emit passEvent(event);
    event->accept();
}
void CSVWidget::SceneToolShapeBrush::dropEvent (QDropEvent *event)
{
    emit passEvent(event);
    event->accept();
}
