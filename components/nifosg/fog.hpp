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
            if (const int base = osg::Fog::compare(sa); base != 0)
                return base;
            const Fog& rhs = static_cast<const Fog&>(sa);
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
