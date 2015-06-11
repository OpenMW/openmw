#ifndef CSV_WIDGET_COLOREDITOR_HPP
#define CSV_WIDGET_COLOREDITOR_HPP

#include <QPushButton>

class QColor;
class QPoint;
class QSize;

namespace CSVWidget
{
    class ColorPickerPopup;

    class ColorEditor : public QPushButton
    {
            Q_OBJECT

            QColor mColor;
            QSize mColoredRectSize;
            ColorPickerPopup *mColorPicker;

            QPoint calculatePopupPosition();

        public:
            ColorEditor(const QColor &color,
                        const QSize &coloredRectSize,
                        QWidget *parent = 0);

            QColor color() const;
            void setColor(const QColor &color);
            void setColoredRectSize(const QSize &size);

        protected:
            void paintEvent(QPaintEvent *event);

        private slots:
            void showPicker();
            void pickerHid();
            void pickerColorChanged(const QColor &color);
    };
}

#endif
