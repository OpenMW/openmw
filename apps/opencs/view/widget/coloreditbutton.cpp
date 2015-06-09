#include "coloreditbutton.hpp"

#include <QColor>
#include <QPainter>
#include <QRect>

CSVWidget::ColorEditButton::ColorEditButton(const QColor &color,
                                            const QSize &coloredRectSize,
                                            QWidget *parent)
    : QPushButton(parent),
      mColor(color),
      mColoredRectSize(coloredRectSize)
      
{}

void CSVWidget::ColorEditButton::paintEvent(QPaintEvent *event)
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

QColor CSVWidget::ColorEditButton::color() const
{
    return mColor;
}

void CSVWidget::ColorEditButton::setColor(const QColor &color)
{
    mColor = color;
}

void CSVWidget::ColorEditButton::setColoredRectSize(const QSize &size)
{
    mColoredRectSize = size;
}