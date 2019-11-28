#include "coloreditor.hpp"

#include <QApplication>
#include <QColorDialog>
#include <QDesktopWidget>
#include <QPainter>
#include <QShowEvent>

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#include <QScreen>
#endif

#include "colorpickerpopup.hpp"

CSVWidget::ColorEditor::ColorEditor(const QColor &color, QWidget *parent, const bool popupOnStart)
    : ColorEditor(parent, popupOnStart)
{
    setColor(color);
}

CSVWidget::ColorEditor::ColorEditor(const int colorInt, QWidget *parent, const bool popupOnStart)
    : ColorEditor(parent, popupOnStart)
{
    setColor(colorInt);
}

CSVWidget::ColorEditor::ColorEditor(QWidget *parent, const bool popupOnStart)
    : QPushButton(parent),
      mColorPicker(new ColorPickerPopup(this)),
      mPopupOnStart(popupOnStart)
{
    connect(this, SIGNAL(clicked()), this, SLOT(showPicker()));
    connect(mColorPicker, SIGNAL(colorChanged(const QColor &)), this, SLOT(pickerColorChanged(const QColor &)));
}

void CSVWidget::ColorEditor::paintEvent(QPaintEvent *event)
{
    QPushButton::paintEvent(event);

    QRect buttonRect = rect();
    QRect coloredRect(buttonRect.x() + qRound(buttonRect.width() / 4.0),
                      buttonRect.y() + qRound(buttonRect.height() / 4.0),
                      buttonRect.width() / 2,
                      buttonRect.height() / 2);
    QPainter painter(this);
    painter.fillRect(coloredRect, mColor);
    painter.setPen(Qt::black);
    painter.drawRect(coloredRect);
}

void CSVWidget::ColorEditor::showEvent(QShowEvent *event)
{
    QPushButton::showEvent(event);
    if (isVisible() && mPopupOnStart)
    {
        setChecked(true);
        showPicker();
        mPopupOnStart = false;
    }
}

QColor CSVWidget::ColorEditor::color() const
{
    return mColor;
}

int CSVWidget::ColorEditor::colorInt() const
{
    return (mColor.blue() << 16) | (mColor.green() << 8) | (mColor.red());
}

void CSVWidget::ColorEditor::setColor(const QColor &color)
{
    mColor = color;
    update();
}

void CSVWidget::ColorEditor::setColor(const int colorInt)
{
    // Color RGB values are stored in given integer.
    // First byte is red, second byte is green, third byte is blue.
    QColor color = QColor(colorInt & 0xff, (colorInt >> 8) & 0xff, (colorInt >> 16) & 0xff);
    setColor(color);
}

void CSVWidget::ColorEditor::showPicker()
{
    mColorPicker->showPicker(calculatePopupPosition(), mColor);
    emit pickingFinished();
}

void CSVWidget::ColorEditor::pickerColorChanged(const QColor &color)
{
    mColor = color;
    update();
}

QPoint CSVWidget::ColorEditor::calculatePopupPosition()
{
    QRect editorGeometry = geometry();
    QRect popupGeometry = mColorPicker->geometry();
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
#else
    QRect screenGeometry = QApplication::desktop()->screenGeometry();
#endif

    // Center the popup horizontally relative to the editor
    int localPopupX = (editorGeometry.width() - popupGeometry.width()) / 2;
    // Popup position need to be specified in global coords
    QPoint popupPosition = mapToGlobal(QPoint(localPopupX, editorGeometry.height()));

    // Make sure that the popup isn't out of the screen
    if (popupPosition.x() < screenGeometry.left())
    {
        popupPosition.setX(screenGeometry.left() + 1);
    }
    else if (popupPosition.x() + popupGeometry.width() > screenGeometry.right())
    {
        popupPosition.setX(screenGeometry.right() - popupGeometry.width() - 1);
    }
    if (popupPosition.y() + popupGeometry.height() > screenGeometry.bottom())
    {
        // Place the popup above the editor
        popupPosition.setY(popupPosition.y() - popupGeometry.height() - editorGeometry.height() - 1);
    }

    return popupPosition;
}
