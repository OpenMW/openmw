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
            ColorPickerPopup *mColorPicker;

            QPoint calculatePopupPosition();

        public:
            ColorEditor(const QColor &color, QWidget *parent = 0);

            QColor color() const;
            void setColor(const QColor &color);

        protected:
            void paintEvent(QPaintEvent *event);

        private slots:
            void showPicker();
            void pickerHid();
            void pickerColorChanged(const QColor &color);

        signals:
            void pickingFinished();
    };
}

#endif
