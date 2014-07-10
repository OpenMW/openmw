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

        public:

            SceneToolMode (SceneToolbar *parent);

            virtual void showPanel (const QPoint& position);

            void addButton (const std::string& icon, const std::string& id);

        signals:

            void modeChanged (const std::string& id);

        private slots:

            void selected();
    };
}

#endif
