#ifndef CSVWIDGET_COLORPICKERPOPUP_HPP
#define CSVWIDGET_COLORPICKERPOPUP_HPP

#include <QFrame>

class QColorDialog;

namespace CSVWidget
{
    class ColorPickerPopup : public QFrame
    {
        Q_OBJECT

        QColorDialog *mColorPicker;

    public:
        explicit ColorPickerPopup(QWidget *parent);
    
        void showPicker(const QPoint &position, const QColor &initialColor);

    protected:
        virtual void mousePressEvent(QMouseEvent *event);
        virtual void hideEvent(QHideEvent *event);
        virtual bool eventFilter(QObject *object, QEvent *event);

    signals:
        void hid();
        void colorChanged(const QColor &color);
    };
}

#endif
