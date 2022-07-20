#ifndef CSV_WIDGET_MODEBUTTON_H
#define CSV_WIDGET_MODEBUTTON_H

#include "pushbutton.hpp"

class QMenu;

namespace CSVWidget
{
    class SceneToolbar;

    /// \brief Specialist PushButton of Type_Mode for use in SceneToolMode
    class ModeButton : public PushButton
    {
            Q_OBJECT

        public:

            ModeButton (const QIcon& icon, const QString& tooltip = "",
                QWidget *parent = nullptr);

            /// Default-Implementation: do nothing
            virtual void activate (SceneToolbar *toolbar);

            /// Default-Implementation: do nothing
            virtual void deactivate (SceneToolbar *toolbar);

            /// Add context menu items to \a menu. Default-implementation: return false
            ///
            /// \attention menu can be a 0-pointer
            ///
            /// \return Have there been any menu items to be added (if menu is 0 and there
            /// items to be added, the function must return true anyway.
            virtual bool createContextMenu (QMenu *menu);
    };
}

#endif
