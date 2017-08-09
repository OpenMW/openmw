#ifndef CSV_RENDER_INSTANCE_SELECTION_MODE_H
#define CSV_RENDER_INSTANCE_SELECTION_MODE_H

#include "selectionmode.hpp"

namespace CSVRender
{
    class InstanceSelectionMode : public SelectionMode
    {
            Q_OBJECT

        public:

            InstanceSelectionMode(CSVWidget::SceneToolbar* parent, WorldspaceWidget& worldspaceWidget);

        protected:

            /// Add context menu items to \a menu.
            ///
            /// \attention menu can be a 0-pointer
            ///
            /// \return Have there been any menu items to be added (if menu is 0 and there
            /// items to be added, the function must return true anyway.
            bool createContextMenu(QMenu* menu);

        private:

            QAction* mDeleteSelection;
            QAction* mSelectSame;

        private slots:

            void deleteSelection();
            void selectSame();
    };
}

#endif
