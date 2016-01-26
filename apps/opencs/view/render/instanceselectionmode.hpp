#ifndef CSV_RENDER_INSTANCE_SELECTION_MODE_H
#define CSV_RENDER_INSTANCE_SELECTION_MODE_H

#include "../widget/scenetoolmode.hpp"

class QAction;

namespace CSVRender
{
    class WorldspaceWidget;

    class InstanceSelectionMode : public CSVWidget::SceneToolMode
    {
            Q_OBJECT

            WorldspaceWidget& mWorldspaceWidget;
            QAction *mSelectAll;
            QAction *mDeselectAll;
            QAction *mDeleteSelection;
            QAction *mSelectSame;

            /// Add context menu items to \a menu.
            ///
            /// \attention menu can be a 0-pointer
            ///
            /// \return Have there been any menu items to be added (if menu is 0 and there
            /// items to be added, the function must return true anyway.
            virtual bool createContextMenu (QMenu *menu);

        public:

            InstanceSelectionMode (CSVWidget::SceneToolbar *parent, WorldspaceWidget& worldspaceWidget);

        private slots:

            void selectAll();

            void clearSelection();

            void deleteSelection();

            void selectSame();
    };
}

#endif
