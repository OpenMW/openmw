#include "coloreditor.hpp"

#include <QApplication>
#include <QColor>
#include <QColorDialog>
#include <QDesktopWidget>
#include <QPainter>
#include <QRect>

#include "colorpickerpopup.hpp"

CSVWidget::ColorEditor::ColorEditor(const QColor &color,
                                    const QSize &coloredRectSize,
                                    QWidget *parent)
    : QPushButton(parent),
      mColor(color),
      mColoredRectSize(coloredRectSize),
      mColorPicker(new ColorPickerPopup(this))
{
    setCheckable(true);
    connect(this, SIGNAL(clicked()), this, SLOT(showPicker()));
    connect(mColorPicker, SIGNAL(hid()), this, SLOT(pickerHid()));
    connect(mColorPicker, SIGNAL(colorChanged(const QColor &)), this, SLOT(pickerColorChanged(const QColor &)));
}

void CSVWidget::ColorEditor::paintEvent(QPaintEvent *event)
{
    QPushButton::paintEvent(event);

    QRect buttonRect = rect();
    QRect coloredRect(buttonRect.x() + (buttonRect.width() - mColoredRectSize.width()) / 2,
                      buttonRect.y() + (buttonRect.height() - mColoredRectSize.height()) / 2,
                      mColoredRectSize.width(),
                      mColoredRectSize.height());
    QPainter painter(this);
    painter.fillRect(coloredRect, mColor);
}

QColor CSVWidget::ColorEditor::color() const
{
    return mColor;
}

void CSVWidget::ColorEditor::setColor(const QColor &color)
{
    mColor = color;
}

void CSVWidget::ColorEditor::setColoredRectSize(const QSize &size)
{
    mColoredRectSize = size;
}

void CSVWidget::ColorEditor::showPicker()
{
    if (isChecked())
    {
        mColorPicker->showPicker(calculatePopupPosition(), mColor);
    }
    else
    {
        mColorPicker->hide();
    }
}

void CSVWidget::ColorEditor::pickerHid()
{
    // If the popup is hidden and mouse isn't above the editor,
    // reset the editor checked state manually
    QPoint globalEditorPosition = mapToGlobal(QPoint(0, 0));
    QRect globalEditorRect(globalEditorPosition, geometry().size());
    if (!globalEditorRect.contains(QCursor::pos()))
    {
        setChecked(false);
    }
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
    QRect screenGeometry = QApplication::desktop()->screenGeometry();

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
