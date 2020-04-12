#ifndef OPENCS_VIEW_TAGBASE_H
#define OPENCS_VIEW_TAGBASE_H

#include <osg/Referenced>

#include <QString>

#include <components/sceneutil/vismask.hpp>

namespace CSVRender
{
    class TagBase : public osg::Referenced
    {
            SceneUtil::VisMask mMask;

        public:

            TagBase (SceneUtil::VisMask mask);

            SceneUtil::VisMask getMask() const;

            virtual QString getToolTip (bool hideBasics) const;

    };
}

#endif
