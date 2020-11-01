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
        void mousePressEvent(QMouseEvent *event) override;
        bool eventFilter(QObject *object, QEvent *event) override;

    signals:
        void colorChanged(const QColor &color);
    };
}

#endif
