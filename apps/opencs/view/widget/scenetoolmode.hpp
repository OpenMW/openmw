#ifndef CSV_WIDGET_SCENETOOL_MODE_H
#define CSV_WIDGET_SCENETOOL_MODE_H

#include "scenetool.hpp"

#include <map>

class QHBoxLayout;
class QMenu;
class QEvent;

namespace CSVWidget
{
    class SceneToolbar;
    class ModeButton;

    ///< \brief Mode selector tool
    class SceneToolMode : public SceneTool
    {
            Q_OBJECT

            QWidget *mPanel;
            QHBoxLayout *mLayout;
            std::map<ModeButton *, std::string> mButtons; // widget, id
            int mButtonSize;
            int mIconSize;
            QString mToolTip;
            PushButton *mFirst;
            ModeButton *mCurrent;
            SceneToolbar *mToolbar;

            void adjustToolTip (const ModeButton *activeMode);

            void contextMenuEvent (QContextMenuEvent *event) override;

            /// Add context menu items to \a menu. Default-implementation: Pass on request to
            /// current mode button or return false, if there is no current mode button.
            ///
            /// \attention menu can be a 0-pointer
            ///
            /// \return Have there been any menu items to be added (if menu is 0 and there
            /// items to be added, the function must return true anyway.
            virtual bool createContextMenu (QMenu *menu);

            void setButton (std::map<ModeButton *, std::string>::iterator iter);

        protected:

            bool event(QEvent* event) override;

        public:

            SceneToolMode (SceneToolbar *parent, const QString& toolTip);

            void showPanel (const QPoint& position) override;

            void addButton (const std::string& icon, const std::string& id,
                const QString& tooltip = "");

            /// The ownership of \a button is transferred to *this.
            void addButton (ModeButton *button, const std::string& id);

            /// Will return a 0-pointer only if the mode does not have any buttons yet.
            ModeButton *getCurrent();

            /// Must not be called if there aren't any buttons yet.
            std::string getCurrentId() const;

            /// Manually change the current mode
            void setButton (const std::string& id);

        signals:

            void modeChanged (const std::string& id);

        private slots:

            void selected();
    };
}

#endif
