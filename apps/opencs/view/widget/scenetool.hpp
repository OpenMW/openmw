#ifndef CSV_WIDGET_SCENETOOL_H
#define CSV_WIDGET_SCENETOOL_H

#include "pushbutton.hpp"

namespace CSVWidget
{
    class SceneToolbar;

    ///< \brief Tool base class
    class SceneTool : public PushButton
    {
            Q_OBJECT

        public:

            SceneTool (SceneToolbar *parent);

            virtual void showPanel (const QPoint& position) = 0;

        private slots:

            void openRequest();
    };
}

#endif
