#ifndef CSV_WORLD_SCENETOOL_GRID_H
#define CSV_WORLD_SCENETOOL_GRID_H

#include "scenetool.hpp"

namespace CSVWorld
{
    class SceneToolbar;

    ///< \brief Cell grid selector tool
    class SceneToolGrid : public SceneTool
    {
            Q_OBJECT

            int mIconSize;

        public:

            SceneToolGrid (SceneToolbar *parent);

            virtual void showPanel (const QPoint& position);

        public slots:

            void cellIndexChanged (const std::pair<int, int>& min, const std::pair<int, int>& max);
    };
}

#endif
