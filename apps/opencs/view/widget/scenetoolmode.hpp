#ifndef CSV_WIDGET_SCENETOOL_MODE_H
#define CSV_WIDGET_SCENETOOL_MODE_H

#include "scenetool.hpp"

#include <map>

class QHBoxLayout;

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

        public:

            SceneToolMode (SceneToolbar *parent, const QString& toolTip);

            virtual void showPanel (const QPoint& position);

            void addButton (const std::string& icon, const std::string& id,
                const QString& tooltip = "");

            /// The ownership of \a button is transferred to *this.
            void addButton (ModeButton *button, const std::string& id);

        signals:

            void modeChanged (const std::string& id);

        private slots:

            void selected();
    };
}

#endif
