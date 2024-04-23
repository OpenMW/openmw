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

        int compare(const StateAttribute& sa) const override
        {
            COMPARE_StateAttribute_Types(Fog, sa);

            COMPARE_StateAttribute_Parameter(_mode);
            COMPARE_StateAttribute_Parameter(_density);
            // _start and _end are intentionally ignored as they go unused
            COMPARE_StateAttribute_Parameter(_color);
            COMPARE_StateAttribute_Parameter(_fogCoordinateSource);
            COMPARE_StateAttribute_Parameter(_useRadialFog);
            COMPARE_StateAttribute_Parameter(mDepth);

            return 0;
        }

        void setDepth(float depth) { mDepth = depth; }
        float getDepth() const { return mDepth; }

        void apply(osg::State& state) const override;

    private:
        float mDepth{ 1.f };
    };

}

#endif
