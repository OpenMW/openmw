#ifndef CSV_WIDGET_SCENETOOL_MODE_H
#define CSV_WIDGET_SCENETOOL_MODE_H

#include "scenetool.hpp"

#include <map>

class QHBoxLayout;

namespace CSVWidget
{
    class SceneToolbar;
    class PushButton;

    ///< \brief Mode selector tool
    class SceneToolMode : public SceneTool
    {
            Q_OBJECT

            QWidget *mPanel;
            QHBoxLayout *mLayout;
            std::map<PushButton *, std::string> mButtons; // widget, id
            int mButtonSize;
            int mIconSize;
            QString mToolTip;
            PushButton *mFirst;

            void adjustToolTip (const PushButton *activeMode);

        public:

            SceneToolMode (SceneToolbar *parent, const QString& toolTip);

            virtual void showPanel (const QPoint& position);

            void addButton (const std::string& icon, const std::string& id,
                const QString& tooltip = "");

        signals:

            void modeChanged (const std::string& id);

        private slots:

            void selected();
    };
}

#endif
