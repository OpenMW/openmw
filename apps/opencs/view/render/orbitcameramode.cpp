#include "orbitcameramode.hpp"

#include <QMenu>

#include "worldspacewidget.hpp"

namespace CSVRender
{
    OrbitCameraMode::OrbitCameraMode(WorldspaceWidget* worldspaceWidget, const QIcon& icon, const QString& tooltip,
        QWidget* parent)
        : ModeButton(icon, tooltip, parent)
        , mWorldspaceWidget(worldspaceWidget)
        , mCenterOnSelection(0)
    {
    }

    void OrbitCameraMode::activate(CSVWidget::SceneToolbar* toolbar)
    {
        mCenterOnSelection = new QAction("Center on selected object", this);
        connect(mCenterOnSelection, SIGNAL(triggered()), this, SLOT(centerSelection()));
    }

    bool OrbitCameraMode::createContextMenu(QMenu* menu)
    {
        if (menu)
        {
            menu->addAction(mCenterOnSelection);
        }

        return true;
    }

    void OrbitCameraMode::centerSelection()
    {
        mWorldspaceWidget->centerOrbitCameraOnSelection();
    }
}
