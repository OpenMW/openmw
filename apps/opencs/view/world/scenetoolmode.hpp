#ifndef CSV_WORLD_SCENETOOL_MODE_H
#define CSV_WORLD_SCENETOOL_MODE_H

#include "scenetool.hpp"

#include <map>

class QHBoxLayout;

namespace CSVWorld
{
    ///< \brief Mode selector tool
    class SceneToolMode : public SceneTool
    {
            Q_OBJECT

            QWidget *mPanel;
            QHBoxLayout *mLayout;
            std::map<QPushButton *, std::string> mButtons; // widget, id

        public:

            SceneToolMode (QWidget *parent = 0);

            virtual void showPanel (const QPoint& position);

            void addButton (const std::string& icon, const std::string& id);

        signals:

            void modeChanged (const std::string& id);

        private slots:

            void selected();
    };
}

#endif
