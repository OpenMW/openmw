#ifndef OPENCS_VIEW_TAGBASE_H
#define OPENCS_VIEW_TAGBASE_H

#include <osg/Referenced>

#include <QString>

#include "mask.hpp"

namespace CSVRender
{
    struct WorldspaceHitResult;

    class TagBase : public osg::Referenced
    {
            Mask mMask;

        public:

            TagBase (Mask mask);

            Mask getMask() const;

            virtual QString getToolTip (bool hideBasics, const WorldspaceHitResult& hit) const;

    };
}

#endif
