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
            bool mPopupOnStart;

            QPoint calculatePopupPosition();

        public:
            ColorEditor(const QColor &color, QWidget *parent = 0, bool popupOnStart = false);

            QColor color() const;
            void setColor(const QColor &color);

        protected:
            virtual void paintEvent(QPaintEvent *event);
            virtual void showEvent(QShowEvent *event);

        private slots:
            void showPicker();
            void pickerHid();
            void pickerColorChanged(const QColor &color);

        signals:
            void pickingFinished();
    };
}

#endif
