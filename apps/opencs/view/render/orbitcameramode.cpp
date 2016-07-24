#include "orbitcameramode.hpp"

#include <QMenu>

#include "../../model/prefs/shortcut.hpp"
#include "../../model/prefs/shortcuteventhandler.hpp"

#include "worldspacewidget.hpp"

namespace CSVRender
{
    OrbitCameraMode::OrbitCameraMode(WorldspaceWidget* worldspaceWidget, const QIcon& icon, const QString& tooltip,
        QWidget* parent)
        : ModeButton(icon, tooltip, parent)
        , mWorldspaceWidget(worldspaceWidget)
        , mCenterOnSelection(0)
    {
        mCenterShortcut.reset(new CSMPrefs::Shortcut("orbit-center-selection", worldspaceWidget));
        mCenterShortcut->enable(false);
        connect(mCenterShortcut.get(), SIGNAL(activated()), this, SLOT(centerSelection()));
    }

    OrbitCameraMode::~OrbitCameraMode()
    {
    }

    void OrbitCameraMode::activate(CSVWidget::SceneToolbar* toolbar)
    {
        mCenterOnSelection = new QAction("Center on selected object\t" + mCenterShortcut->toString(), this);
        connect(mCenterOnSelection, SIGNAL(triggered()), this, SLOT(centerSelection()));

        mCenterShortcut->enable(true);
    }

    void OrbitCameraMode::deactivate(CSVWidget::SceneToolbar* toolbar)
    {
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
