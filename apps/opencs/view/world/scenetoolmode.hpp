#ifndef CSV_WORLD_SCENETOOL_MODE_H
#define CSV_WORLD_SCENETOOL_MODE_H

#include "scenetool.hpp"

namespace CSVWorld
{
    ///< \brief Mode selector tool
    class SceneToolMode : public SceneTool
    {
            Q_OBJECT

        public:

            SceneToolMode (QWidget *parent = 0);
    };
}

#endif
