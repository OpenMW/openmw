#include "orbitcameramode.hpp"

#include <QMenu>

#include "../../model/prefs/shortcut.hpp"

#include "worldspacewidget.hpp"

namespace CSVRender
{
    OrbitCameraMode::OrbitCameraMode(WorldspaceWidget* worldspaceWidget, const QIcon& icon, const QString& tooltip,
        QWidget* parent)
        : ModeButton(icon, tooltip, parent)
        , mWorldspaceWidget(worldspaceWidget)
        , mCenterOnSelection(nullptr)
    {
        mCenterShortcut = new CSMPrefs::Shortcut("orbit-center-selection", worldspaceWidget);
        mCenterShortcut->enable(false);
        connect(mCenterShortcut, SIGNAL(activated()), this, SLOT(centerSelection()));
    }

    OrbitCameraMode::~OrbitCameraMode()
    {
    }

    void OrbitCameraMode::activate(CSVWidget::SceneToolbar* toolbar)
    {
        mCenterOnSelection = new QAction("Center on selected object", this);
        mCenterShortcut->associateAction(mCenterOnSelection);
        connect(mCenterOnSelection, SIGNAL(triggered()), this, SLOT(centerSelection()));

        mCenterShortcut->enable(true);
    }

    void OrbitCameraMode::deactivate(CSVWidget::SceneToolbar* toolbar)
    {
        mCenterShortcut->associateAction(nullptr);
        mCenterShortcut->enable(false);
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
