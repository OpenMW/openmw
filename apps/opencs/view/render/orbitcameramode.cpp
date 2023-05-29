#include "orbitcameramode.hpp"

#include <QMenu>

#include <memory>

#include "../../model/prefs/shortcut.hpp"

#include <apps/opencs/view/widget/modebutton.hpp>

#include "worldspacewidget.hpp"

namespace CSVWidget
{
    class SceneToolbar;
}

namespace CSVRender
{
    OrbitCameraMode::OrbitCameraMode(
        WorldspaceWidget* worldspaceWidget, const QIcon& icon, const QString& tooltip, QWidget* parent)
        : ModeButton(icon, tooltip, parent)
        , mWorldspaceWidget(worldspaceWidget)
        , mCenterOnSelection(nullptr)
    {
        mCenterShortcut = new CSMPrefs::Shortcut("orbit-center-selection", worldspaceWidget);
        mCenterShortcut->enable(false);
        connect(mCenterShortcut, qOverload<>(&CSMPrefs::Shortcut::activated), this, &OrbitCameraMode::centerSelection);
    }

    void OrbitCameraMode::activate(CSVWidget::SceneToolbar* toolbar)
    {
        mCenterOnSelection = new QAction("Center on selected object", this);
        mCenterShortcut->associateAction(mCenterOnSelection);
        connect(mCenterOnSelection, &QAction::triggered, this, &OrbitCameraMode::centerSelection);

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
