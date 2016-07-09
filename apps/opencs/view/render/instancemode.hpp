#ifndef CSV_RENDER_INSTANCEMODE_H
#define CSV_RENDER_INSTANCEMODE_H

#include "editmode.hpp"

namespace CSVWidget
{
    class SceneToolMode;
}

namespace CSVRender
{
    class InstanceSelectionMode;

    class InstanceMode : public EditMode
    {
            Q_OBJECT

            enum DragMode
            {
                DragMode_None,
                DragMode_Move
            };

            CSVWidget::SceneToolMode *mSubMode;
            std::string mSubModeId;
            InstanceSelectionMode *mSelectionMode;
            DragMode mDragMode;
            int mDragAxis;
            bool mLocked;

            int getSubModeFromId (const std::string& id) const;

        public:

            InstanceMode (WorldspaceWidget *worldspaceWidget, QWidget *parent = 0);

            virtual void activate (CSVWidget::SceneToolbar *toolbar);

            virtual void deactivate (CSVWidget::SceneToolbar *toolbar);

            virtual void setEditLock (bool locked);

            virtual void primaryEditPressed (const WorldspaceHitResult& hit);

            virtual void secondaryEditPressed (const WorldspaceHitResult& hit);

            virtual void primarySelectPressed (const WorldspaceHitResult& hit);

            virtual void secondarySelectPressed (const WorldspaceHitResult& hit);

            virtual bool primaryEditStartDrag (const QPoint& pos);

            virtual bool secondaryEditStartDrag (const QPoint& pos);

            virtual void drag (const QPoint& pos, int diffX, int diffY, double speedFactor);

            virtual void dragCompleted(const QPoint& pos);

            /// \note dragAborted will not be called, if the drag is aborted via changing
            /// editing mode
            virtual void dragAborted();

            virtual void dragWheel (int diff, double speedFactor);

            virtual void dragEnterEvent (QDragEnterEvent *event);

            virtual void dropEvent (QDropEvent *event);

            virtual int getSubMode() const;

        private slots:

            void subModeChanged (const std::string& id);
    };
}

#endif
