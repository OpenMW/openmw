#ifndef CSV_RENDER_PATHGRIDMODE_H
#define CSV_RENDER_PATHGRIDMODE_H

#include <string>

#include "editmode.hpp"

namespace CSVRender
{
    class PathgridMode : public EditMode
    {
            Q_OBJECT

        public:

            PathgridMode(WorldspaceWidget* worldspace, QWidget* parent=0);

            virtual void primaryEditPressed(const WorldspaceHitResult& hit);

            virtual void secondaryEditPressed(const WorldspaceHitResult& hit);

            virtual void primarySelectPressed(const WorldspaceHitResult& hit);

            virtual void secondarySelectPressed(const WorldspaceHitResult& hit);

            virtual bool primaryEditStartDrag (const WorldspaceHitResult& hit);

            virtual bool secondaryEditStartDrag (const WorldspaceHitResult& hit);

            virtual void drag (int diffX, int diffY, double speedFactor);

            virtual void dragCompleted();

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

            std::string mLastId;
            DragMode mDragMode;
            unsigned short mFromNode;
            QString getTooltip();

    };
}

#endif
