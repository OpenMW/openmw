#ifndef OPENMW_MWRENDER_SHADOW_H
#define OPENMW_MWRENDER_SHADOW_H

#include <osgShadow/LightSpacePerspectiveShadowMap>

namespace MWRender
{
    class MWShadow : public osgShadow::LightSpacePerspectiveShadowMapDB
    {
    protected:
        struct ViewData : public LightSpacePerspectiveShadowMapDB::ViewData
        {
            virtual void init(MWShadow * st, osgUtil::CullVisitor * cv);
        };

        virtual ViewDependentShadowTechnique::ViewData * initViewDependentData(osgUtil::CullVisitor *cv, ViewDependentShadowTechnique::ViewData * vd)
        {
            MWShadow::ViewData* td = dynamic_cast<MWShadow::ViewData*>(vd);
            if (!td)
                td = new MWShadow::ViewData;
            td->init(this, cv);
            return td;
        }
    };
}

#endif //OPENMW_MWRENDER_SHADOW_H
