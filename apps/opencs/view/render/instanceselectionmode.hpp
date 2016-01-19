#ifndef CSV_RENDER_INSTANCE_SELECTION_MODE_H
#define CSV_RENDER_INSTANCE_SELECTION_MODE_H

#include "../widget/scenetoolmode.hpp"

namespace CSVRender
{
    class InstanceSelectionMode : public CSVWidget::SceneToolMode
    {
            Q_OBJECT

        public:

            InstanceSelectionMode (CSVWidget::SceneToolbar *parent);

    };
}

#endif
