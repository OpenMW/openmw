#ifndef CSV_RENDER_ORBITCAMERAPICKMODE_H
#define CSV_RENDER_ORBITCAMERAPICKMODE_H

#include "../widget/modebutton.hpp"

namespace CSMPrefs
{
    class Shortcut;
    class ShortcutEventHandler;
}

namespace CSVRender
{
    class WorldspaceWidget;

    class OrbitCameraMode : public CSVWidget::ModeButton
    {
            Q_OBJECT

        public:

            OrbitCameraMode(WorldspaceWidget* worldspaceWidget, CSMPrefs::ShortcutEventHandler* shortcutHandler,
                const QIcon& icon, const QString& tooltip = "", QWidget* parent = 0);
            ~OrbitCameraMode();

            virtual void activate(CSVWidget::SceneToolbar* toolbar);
            virtual void deactivate(CSVWidget::SceneToolbar* toolbar);
            virtual bool createContextMenu(QMenu* menu);

        private:

            WorldspaceWidget* mWorldspaceWidget;
            CSMPrefs::ShortcutEventHandler* mShortcutHandler;
            QAction* mCenterOnSelection;
            CSMPrefs::Shortcut* mCenterShortcut;

        private slots:

            void centerSelection();
    };
}

#endif
