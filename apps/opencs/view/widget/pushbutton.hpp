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

            /// \param push Do not maintain a toggle state
            PushButton (const QIcon& icon, bool push = false, QWidget *parent = 0);

            /// \param push Do not maintain a toggle state
            PushButton (bool push = false, QWidget *parent = 0);

            bool hasKeepOpen() const;
    };
}

#endif
