#include "lightutil.hpp"

#include <osg/Light>

namespace SceneUtil
{

    void configureLight(osg::Light *light, float radius, bool isExterior, bool outQuadInLin, bool useQuadratic,
                        float quadraticValue, float quadraticRadiusMult, bool useLinear, float linearRadiusMult, float linearValue)
    {
        bool quadratic = useQuadratic && (!outQuadInLin || isExterior);

        float quadraticAttenuation = 0;
        float linearAttenuation = 0;
        if (quadratic)
        {
            float r = radius * quadraticRadiusMult;
            quadraticAttenuation = quadraticValue / std::pow(r, 2);
        }
        if (useLinear)
        {
            float r = radius * linearRadiusMult;
            linearAttenuation = linearValue / r;
        }

        light->setLinearAttenuation(linearAttenuation);
        light->setQuadraticAttenuation(quadraticAttenuation);
        light->setConstantAttenuation(0.f);

    }

}
