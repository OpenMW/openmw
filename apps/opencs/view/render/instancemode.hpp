#ifndef CSV_RENDER_INSTANCEMODE_H
#define CSV_RENDER_INSTANCEMODE_H

#include "editmode.hpp"

namespace CSVRender
{
    class InstanceMode : public EditMode
    {
            Q_OBJECT

            bool mContextSelect;

        public:

            InstanceMode (WorldspaceWidget *worldspaceWidget, QWidget *parent = 0);

            virtual void activate (CSVWidget::SceneToolbar *toolbar);

            virtual void updateUserSetting (const QString& name, const QStringList& value);

            virtual void primaryEditPressed (osg::ref_ptr<TagBase> tag);

            virtual void secondaryEditPressed (osg::ref_ptr<TagBase> tag);

            virtual void primarySelectPressed (osg::ref_ptr<TagBase> tag);

            virtual void secondarySelectPressed (osg::ref_ptr<TagBase> tag);
    };
}

#endif
