#ifndef CSV_WORLD_SCENETOOLBAR_H
#define CSV_WORLD_SCENETOOLBAR_H

#include <QWidget>

class QVBoxLayout;

namespace CSVWorld
{
    class SceneTool;

    class SceneToolbar : public QWidget
    {
            Q_OBJECT

            QVBoxLayout *mLayout;

        public:

            SceneToolbar (QWidget *parent);

            void addTool (SceneTool *tool);
    };
}

#endif
