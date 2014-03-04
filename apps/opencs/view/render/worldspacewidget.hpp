#ifndef OPENCS_VIEW_WORLDSPACEWIDGET_H
#define OPENCS_VIEW_WORLDSPACEWIDGET_H

#include "scenewidget.hpp"

namespace CSVRender
{
    class WorldspaceWidget : public SceneWidget
    {
            Q_OBJECT

        public:

            WorldspaceWidget (QWidget *parent = 0);
    };
}

#endif