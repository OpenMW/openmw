#ifndef CSV_RENDER_PATHGRIDMODE_H
#define CSV_RENDER_PATHGRIDMODE_H

#include <string>

#include "editmode.hpp"

namespace CSVRender
{
    class PathgridSelectionMode;

    class PathgridMode : public EditMode
    {
            Q_OBJECT

        public:

            PathgridMode(WorldspaceWidget* worldspace, QWidget* parent=nullptr);

            void activate(CSVWidget::SceneToolbar* toolbar) override;

            void deactivate(CSVWidget::SceneToolbar* toolbar) override;

            void primaryOpenPressed(const WorldspaceHitResult& hit) override;

            void primaryEditPressed(const WorldspaceHitResult& hit) override;

            void secondaryEditPressed(const WorldspaceHitResult& hit) override;

            void primarySelectPressed(const WorldspaceHitResult& hit) override;

            void secondarySelectPressed(const WorldspaceHitResult& hit) override;

            bool primaryEditStartDrag (const QPoint& pos) override;

            bool secondaryEditStartDrag (const QPoint& pos) override;

            void drag (const QPoint& pos, int diffX, int diffY, double speedFactor) override;

            void dragCompleted(const QPoint& pos) override;

            /// \note dragAborted will not be called, if the drag is aborted via changing
            /// editing mode
            void dragAborted() override;

        private:

            enum DragMode
            {
                DragMode_None,
                DragMode_Move,
                DragMode_Edge
            };

            DragMode mDragMode;
            std::string mLastId, mEdgeId;
            unsigned short mFromNode;

            PathgridSelectionMode* mSelectionMode;

            QString getTooltip();
    };
}

#endif
