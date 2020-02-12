#ifndef CSV_RENDER_SELECTION_MODE_H
#define CSV_RENDER_SELECTION_MODE_H

#include "../widget/scenetoolmode.hpp"

class QAction;

namespace CSVRender
{
    class WorldspaceWidget;

    class SelectionMode : public CSVWidget::SceneToolMode
    {
            Q_OBJECT

        public:

            SelectionMode(CSVWidget::SceneToolbar* parent, WorldspaceWidget& worldspaceWidget,
                unsigned int interactionMask);

        protected:

            WorldspaceWidget& getWorldspaceWidget();

            /// Add context menu items to \a menu.
            ///
            /// \attention menu can be a 0-pointer
            ///
            /// \return Have there been any menu items to be added (if menu is 0 and there
            /// items to be added, the function must return true anyway.
            virtual bool createContextMenu (QMenu* menu);

        private:

            WorldspaceWidget& mWorldspaceWidget;
            unsigned int mInteractionMask;
            QAction* mSelectAll;
            QAction* mDeselectAll;
            QAction* mInvertSelection;

        protected slots:

            virtual void selectAll();
            virtual void clearSelection();
            virtual void invertSelection();
    };
}

#endif
