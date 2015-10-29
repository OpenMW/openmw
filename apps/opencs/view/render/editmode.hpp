#ifndef CSV_RENDER_EDITMODE_H
#define CSV_RENDER_EDITMODE_H

#include <osg/ref_ptr>

#include "../widget/modebutton.hpp"

namespace CSVRender
{
    class WorldspaceWidget;
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

            /// Default-implementation: Do nothing.
            virtual void updateUserSetting (const QString& name, const QStringList& value);

            /// Default-implementation: Ignored.
            virtual void setEditLock (bool locked);

            /// Default-implementation: Ignored.
            virtual void primaryEditPressed (osg::ref_ptr<TagBase> tag);

            /// Default-implementation: Ignored.
            virtual void secondaryEditPressed (osg::ref_ptr<TagBase> tag);

            /// Default-implementation: Ignored.
            virtual void primarySelectPressed (osg::ref_ptr<TagBase> tag);

            /// Default-implementation: Ignored.
            virtual void secondarySelectPressed (osg::ref_ptr<TagBase> tag);

            /// Default-implementation: ignore and return false
            ///
            /// \return Drag accepted?
            virtual bool primaryEditStartDrag (osg::ref_ptr<TagBase> tag);

            /// Default-implementation: ignore and return false
            ///
            /// \return Drag accepted?
            virtual bool secondaryEditStartDrag (osg::ref_ptr<TagBase> tag);

            /// Default-implementation: ignore and return false
            ///
            /// \return Drag accepted?
            virtual bool primarySelectStartDrag (osg::ref_ptr<TagBase> tag);

            /// Default-implementation: ignore and return false
            ///
            /// \return Drag accepted?
            virtual bool secondarySelectStartDrag (osg::ref_ptr<TagBase> tag);

            /// Default-implementation: ignored
            virtual void drag (int diffX, int diffY, double speedFactor);

            /// Default-implementation: ignored
            virtual void dragCompleted();

            /// Default-implementation: ignored
            ///
            /// \note dragAborted will not be called, if the drag is aborted via changing
            /// editing mode
            virtual void dragAborted();

            /// Default-implementation: ignored
            virtual void dragWheel (int diff, double speedFactor);
    };
}

#endif
