#ifndef OPENCS_VIEW_WORLDSPACEWIDGET_H
#define OPENCS_VIEW_WORLDSPACEWIDGET_H

#include "scenewidget.hpp"

#include "navigation1st.hpp"
#include "navigationfree.hpp"
#include "navigationorbit.hpp"

namespace CSVWorld
{
    class SceneToolMode;
    class SceneToolbar;
}

namespace CSVRender
{
    class WorldspaceWidget : public SceneWidget
    {
            Q_OBJECT

            CSVRender::Navigation1st m1st;
            CSVRender::NavigationFree mFree;
            CSVRender::NavigationOrbit mOrbit;

        public:

            WorldspaceWidget (QWidget *parent = 0);

            CSVWorld::SceneToolMode *makeNavigationSelector (CSVWorld::SceneToolbar *parent);
            ///< \attention The created tool is not added to the toolbar (via addTool). Doing that
            /// is the responsibility of the calling function.

            void selectDefaultNavigationMode();

        private slots:

            void selectNavigationMode (const std::string& mode);

        signals:

            void closeRequest();
    };
}

#endif