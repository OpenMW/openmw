#ifndef OPENMW_COMPONENTS_NIFOSG_FOG_H
#define OPENMW_COMPONENTS_NIFOSG_FOG_H

#include <osg/Fog>

namespace NifOsg
{

    // osg::Fog-based wrapper for NiFogProperty that autocalculates the fog start and end distance.
    class Fog : public osg::Fog
    {
    public:
        Fog();
        Fog(const Fog& copy, const osg::CopyOp& copyop);

        META_StateAttribute(NifOsg, Fog, FOG)

        void setDepth(float depth) { mDepth = depth; }
        float getDepth() const { return mDepth; }

        void apply(osg::State& state) const override;

    private:
        float mDepth{ 1.f };
    };

}

#endif
