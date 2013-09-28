#ifndef CSV_WORLD_SCENETOOL_H
#define CSV_WORLD_SCENETOOL_H

#include <QPushButton>

namespace CSVWorld
{
    ///< \brief Tool base class
    class SceneTool : public QPushButton
    {
            Q_OBJECT

        public:

            SceneTool (QWidget *parent = 0);

    };
}

#endif
