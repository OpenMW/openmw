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

            DialogueSpinBox (QWidget *parent = nullptr);

        protected:

            void focusInEvent(QFocusEvent *event) override;
            void focusOutEvent(QFocusEvent *event) override;
            void wheelEvent(QWheelEvent *event) override;
    };

    class DialogueDoubleSpinBox : public QDoubleSpinBox
    {
        Q_OBJECT

        public:

            DialogueDoubleSpinBox (QWidget *parent = nullptr);

        protected:

            void focusInEvent(QFocusEvent *event) override;
            void focusOutEvent(QFocusEvent *event) override;
            void wheelEvent(QWheelEvent *event) override;
    };
}

#endif // CSV_WORLD_DIALOGUESPINBOX_H
