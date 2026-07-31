#ifndef OPENMW_COMPONENTS_NIFOSG_FOG_H
#define OPENMW_COMPONENTS_NIFOSG_FOG_H

#include <osg/Vec4f>

#include <components/sceneutil/statesetupdater.hpp>

namespace NifOsg
{

    // Wrapper for NiFogProperty that autocalculates the fog start and end distance.
    class Fog : public SceneUtil::StateSetUpdater
    {
    public:
        void setDepth(float depth) { mDepth = depth; }
        float getDepth() const { return mDepth; }

        void setColor(const osg::Vec4f& color) { mColor = color; }
        const osg::Vec4f& getColor() const { return mColor; }

    private:
        void apply(osg::StateSet* stateset, osg::NodeVisitor* nv) override;

        float mDepth{ 1.f };
        osg::Vec4f mColor;
    };

}

#endif
