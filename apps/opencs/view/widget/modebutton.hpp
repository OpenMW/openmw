#ifndef CSV_WIDGET_MODEBUTTON_H
#define CSV_WIDGET_MODEBUTTON_H

#include "pushbutton.hpp"

namespace CSVWidget
{
    class SceneToolbar;

    /// \brief Specialist PushButton of Type_Mode for use in SceneToolMode
    class ModeButton : public PushButton
    {
            Q_OBJECT

        public:

            ModeButton (const QIcon& icon, const QString& tooltip = "",
                QWidget *parent = 0);

            /// Default-Implementation: do nothing
            virtual void activate (SceneToolbar *toolbar);

            /// Default-Implementation: do nothing
            virtual void deactivate (SceneToolbar *toolbar);
    };
}

#endif
