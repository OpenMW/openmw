#include "colorpickerpopup.hpp"

#include <QColorDialog>
#include <QPushButton>
#include <QEvent>
#include <QKeyEvent>
#include <QLayout>
#include <QStyleOption>

CSVWidget::ColorPickerPopup::ColorPickerPopup(QWidget *parent) 
    : QFrame(parent)
{
    setWindowFlags(Qt::Popup);
    setFrameStyle(QFrame::Box | QFrame::Plain);
    hide();

    mColorPicker = new QColorDialog(this);
    mColorPicker->setWindowFlags(Qt::Widget);
    mColorPicker->setOptions(QColorDialog::NoButtons | QColorDialog::DontUseNativeDialog);
    mColorPicker->installEventFilter(this);
    connect(mColorPicker,
            SIGNAL(currentColorChanged(const QColor &)),
            this,
            SIGNAL(colorChanged(const QColor &)));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(mColorPicker);
    layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);
    setFixedSize(mColorPicker->size());
}

void CSVWidget::ColorPickerPopup::showPicker(const QPoint &position, const QColor &initialColor)
{
    QRect geometry = this->geometry();
    geometry.moveTo(position);
    setGeometry(geometry);

    // Calling getColor() creates a blocking dialog that will continue execution once the user chooses OK or Cancel
    QColor color = mColorPicker->getColor(initialColor);
    if (color.isValid())
        mColorPicker->setCurrentColor(color);
}

void CSVWidget::ColorPickerPopup::mousePressEvent(QMouseEvent *event)
{
    QPushButton *button = qobject_cast<QPushButton *>(parentWidget());
    if (button != nullptr)
    {
        QStyleOptionButton option;
        option.init(button);
        QRect buttonRect = option.rect;
        buttonRect.moveTo(button->mapToGlobal(buttonRect.topLeft()));

        // If the mouse is pressed above the pop-up parent,
        // the pop-up will be hidden and the pressed signal won't be repeated for the parent
        if (buttonRect.contains(event->globalPos()) || buttonRect.contains(event->pos()))
        {
            setAttribute(Qt::WA_NoMouseReplay);
        }
    }
    QFrame::mousePressEvent(event);
}

bool CSVWidget::ColorPickerPopup::eventFilter(QObject *object, QEvent *event)
{
    if (object == mColorPicker && event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        // Prevent QColorDialog from closing when Escape is pressed.
        // Instead, hide the popup.
        if (keyEvent->key() == Qt::Key_Escape)
        {
            hide();
            return true;
        }
    }
    return QFrame::eventFilter(object, event);
}
