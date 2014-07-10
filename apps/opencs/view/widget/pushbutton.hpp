#ifndef CSV_WIDGET_PUSHBUTTON_H
#define CSV_WIDGET_PUSHBUTTON_H

#include <QPushButton>

namespace CSVWidget
{
    class PushButton : public QPushButton
    {
            Q_OBJECT

            bool mKeepOpen;

        protected:

            virtual void keyPressEvent (QKeyEvent *event);

            virtual void keyReleaseEvent (QKeyEvent *event);

            virtual void mouseReleaseEvent (QMouseEvent *event);

        public:

            PushButton (const QIcon& icon, QWidget *parent = 0);

            bool hasKeepOpen() const;
    };
}

#endif
