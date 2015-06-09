#ifndef CSV_WIDGET_COLOREDITBUTTON_HPP
#define CSV_WIDGET_COLOREDITBUTTON_HPP

#include <QPushButton>

class QColor;
class QSize;

namespace CSVWidget
{
    class ColorEditButton : public QPushButton
    {
            QColor mColor;
            QSize mColoredRectSize;

        public:
            ColorEditButton(const QColor &color,
                            const QSize &coloredRectSize,
                            QWidget *parent = 0);

            QColor color() const;
            void setColor(const QColor &color);
            void setColoredRectSize(const QSize &size);

        protected:
            void paintEvent(QPaintEvent *event);
    };
}

#endif
