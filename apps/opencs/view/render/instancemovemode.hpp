#ifndef CSV_RENDER_INSTANCEMOVEMODE_H
#define CSV_RENDER_INSTANCEMOVEMODE_H

#include "../widget/modebutton.hpp"

namespace CSVRender
{
    class InstanceMoveMode : public CSVWidget::ModeButton
    {
            Q_OBJECT

        public:

            InstanceMoveMode (QWidget *parent = nullptr);
    };
}

#endif
