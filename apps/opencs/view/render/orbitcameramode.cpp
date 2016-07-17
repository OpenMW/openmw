#include "orbitcameramode.hpp"

#include <QMenu>

#include "../../model/prefs/shortcut.hpp"
#include "../../model/prefs/shortcuteventhandler.hpp"

#include "worldspacewidget.hpp"

namespace CSVRender
{
    OrbitCameraMode::OrbitCameraMode(WorldspaceWidget* worldspaceWidget, CSMPrefs::ShortcutEventHandler* handler,
        const QIcon& icon, const QString& tooltip, QWidget* parent)
        : ModeButton(icon, tooltip, parent)
        , mWorldspaceWidget(worldspaceWidget)
        , mShortcutHandler(handler)
        , mCenterOnSelection(0)
    {
        mCenterShortcut = new CSMPrefs::Shortcut("orbit-center-selection", this);
        mCenterShortcut->enable(false);
        mShortcutHandler->addShortcut(mCenterShortcut);
        connect(mCenterShortcut, SIGNAL(activated()), this, SLOT(centerSelection()));
    }

    OrbitCameraMode::~OrbitCameraMode()
    {
        mShortcutHandler->removeShortcut(mCenterShortcut);
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
