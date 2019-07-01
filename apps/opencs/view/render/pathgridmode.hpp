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

            PathgridMode(WorldspaceWidget* worldspace, QWidget* parent=0);

            virtual void activate(CSVWidget::SceneToolbar* toolbar);

            virtual void deactivate(CSVWidget::SceneToolbar* toolbar);

            virtual void primaryOpenPressed(const WorldspaceHitResult& hit);

            virtual void primaryEditPressed(const WorldspaceHitResult& hit);

            virtual void secondaryEditPressed(const WorldspaceHitResult& hit);

            virtual void primarySelectPressed(const WorldspaceHitResult& hit);

            virtual void secondarySelectPressed(const WorldspaceHitResult& hit);

            virtual bool primaryEditStartDrag (const QPoint& pos);

            virtual bool secondaryEditStartDrag (const QPoint& pos);

            virtual void drag (const QPoint& pos, int diffX, int diffY, double speedFactor);

            virtual void dragCompleted(const QPoint& pos);

            /// \note dragAborted will not be called, if the drag is aborted via changing
            /// editing mode
            virtual void dragAborted();

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
