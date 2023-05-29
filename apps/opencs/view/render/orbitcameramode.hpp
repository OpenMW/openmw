#ifndef CSV_RENDER_ORBITCAMERAPICKMODE_H
#define CSV_RENDER_ORBITCAMERAPICKMODE_H

#include "../widget/modebutton.hpp"

class QAction;
class QMenu;
class QObject;
class QWidget;

namespace CSVWidget
{
    class SceneToolbar;
}

namespace CSMPrefs
{
    class Shortcut;
}

namespace CSVRender
{
    class WorldspaceWidget;

    class OrbitCameraMode : public CSVWidget::ModeButton
    {
        Q_OBJECT

    public:
        OrbitCameraMode(WorldspaceWidget* worldspaceWidget, const QIcon& icon, const QString& tooltip = "",
            QWidget* parent = nullptr);
        ~OrbitCameraMode() override = default;

        void activate(CSVWidget::SceneToolbar* toolbar) override;
        void deactivate(CSVWidget::SceneToolbar* toolbar) override;
        bool createContextMenu(QMenu* menu) override;

    private:
        WorldspaceWidget* mWorldspaceWidget;
        QAction* mCenterOnSelection;
        CSMPrefs::Shortcut* mCenterShortcut;

    private slots:

        void centerSelection();
    };
}

#endif
