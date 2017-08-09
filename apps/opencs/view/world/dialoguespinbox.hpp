#ifndef CSV_WORLD_DIALOGUESPINBOX_H
#define CSV_WORLD_DIALOGUESPINBOX_H

#include <QSpinBox>
#include <QDoubleSpinBox>

namespace CSVWorld
{
    class DialogueSpinBox : public QSpinBox
    {
        Q_OBJECT

        public:

            DialogueSpinBox (QWidget *parent = 0);

        protected:

            virtual void focusInEvent(QFocusEvent *event);
            virtual void focusOutEvent(QFocusEvent *event);
            virtual void wheelEvent(QWheelEvent *event);
    };

    class DialogueDoubleSpinBox : public QDoubleSpinBox
    {
        Q_OBJECT

        public:

            DialogueDoubleSpinBox (QWidget *parent = 0);

        protected:

            virtual void focusInEvent(QFocusEvent *event);
            virtual void focusOutEvent(QFocusEvent *event);
            virtual void wheelEvent(QWheelEvent *event);
    };
}

#endif // CSV_WORLD_DIALOGUESPINBOX_H
