#ifndef CSV_RENDER_ORBITCAMERAPICKMODE_H
#define CSV_RENDER_ORBITCAMERAPICKMODE_H

#include "../widget/modebutton.hpp"

namespace CSVRender
{
    class WorldspaceWidget;

    class OrbitCameraMode : public CSVWidget::ModeButton
    {
            Q_OBJECT

        public:

            OrbitCameraMode(WorldspaceWidget* worldspaceWidget, const QIcon& icon, const QString& tooltip = "",
                QWidget* parent = 0);

            virtual void activate(CSVWidget::SceneToolbar* toolbar);
            virtual bool createContextMenu(QMenu* menu);

        private:

            WorldspaceWidget* mWorldspaceWidget;
            QAction* mCenterOnSelection;

        private slots:

            void centerSelection();
    };
}

#endif
