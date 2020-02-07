#ifndef CSV_RENDER_EDITMODE_H
#define CSV_RENDER_EDITMODE_H

#include <osg/ref_ptr>

#include "../widget/modebutton.hpp"

class QDragEnterEvent;
class QDropEvent;
class QDragMoveEvent;
class QPoint;

namespace CSVRender
{
    class WorldspaceWidget;
    struct WorldspaceHitResult;
    class TagBase;

    class EditMode : public CSVWidget::ModeButton
    {
            Q_OBJECT

            WorldspaceWidget *mWorldspaceWidget;
            unsigned int mMask;

        protected:

            WorldspaceWidget& getWorldspaceWidget();

        public:

            EditMode (WorldspaceWidget *worldspaceWidget, const QIcon& icon, unsigned int mask,
                const QString& tooltip = "", QWidget *parent = 0);

            unsigned int getInteractionMask() const;

            virtual void activate (CSVWidget::SceneToolbar *toolbar);

            /// Default-implementation: Ignored.
            virtual void setEditLock (bool locked);

            /// Default-implementation: Ignored.
            virtual void primaryOpenPressed (const WorldspaceHitResult& hit);

            /// Default-implementation: Ignored.
            virtual void primaryEditPressed (const WorldspaceHitResult& hit);

            /// Default-implementation: Ignored.
            virtual void secondaryEditPressed (const WorldspaceHitResult& hit);

            /// Default-implementation: Ignored.
            virtual void primarySelectPressed (const WorldspaceHitResult& hit);

            /// Default-implementation: Ignored.
            virtual void secondarySelectPressed (const WorldspaceHitResult& hit);

            /// Default-implementation: ignore and return false
            ///
            /// \return Drag accepted?
            virtual bool primaryEditStartDrag (const QPoint& pos);

            /// Default-implementation: ignore and return false
            ///
            /// \return Drag accepted?
            virtual bool secondaryEditStartDrag (const QPoint& pos);

            /// Default-implementation: ignore and return false
            ///
            /// \return Drag accepted?
            virtual bool primarySelectStartDrag (const QPoint& pos);

            /// Default-implementation: ignore and return false
            ///
            /// \return Drag accepted?
            virtual bool secondarySelectStartDrag (const QPoint& pos);

            /// Default-implementation: ignored
            virtual void drag (const QPoint& pos, int diffX, int diffY, double speedFactor);

            /// Default-implementation: ignored
            virtual void dragCompleted(const QPoint& pos);

            /// Default-implementation: ignored
            ///
            /// \note dragAborted will not be called, if the drag is aborted via changing
            /// editing mode
            virtual void dragAborted();

            /// Default-implementation: ignored
            virtual void dragWheel (int diff, double speedFactor);

            /// Default-implementation: ignored
            virtual void dragEnterEvent (QDragEnterEvent *event);

            /// Default-implementation: ignored
            virtual void dropEvent (QDropEvent *event);

            /// Default-implementation: ignored
            virtual void dragMoveEvent (QDragMoveEvent *event);

            virtual void mouseMoveEvent (QMouseEvent *event);

            /// Default: return -1
            virtual int getSubMode() const;
    };
}

#endif
