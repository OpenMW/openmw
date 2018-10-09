#include "lightutil.hpp"

#include <osg/Light>
#include <osg/Group>
#include <osg/ComputeBoundsVisitor>

#include <components/esm/loadligh.hpp>

#include "lightmanager.hpp"
#include "lightcontroller.hpp"
#include "util.hpp"
#include "visitor.hpp"
#include "positionattitudetransform.hpp"

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

    void addLight (osg::Group* node, const ESM::Light* esmLight, unsigned int partsysMask, unsigned int lightMask, bool isExterior, bool outQuadInLin, bool useQuadratic,
                   float quadraticValue, float quadraticRadiusMult, bool useLinear, float linearRadiusMult,
                   float linearValue)
    {
        SceneUtil::FindByNameVisitor visitor("AttachLight");
        node->accept(visitor);

        osg::Group* attachTo = nullptr;
        if (visitor.mFoundNode)
        {
            attachTo = visitor.mFoundNode;
        }
        else
        {
            osg::ComputeBoundsVisitor computeBound;
            computeBound.setTraversalMask(~partsysMask);
            // We want the bounds of all children of the node, ignoring the node's local transformation
            // So do a traverse(), not accept()
            computeBound.traverse(*node);

            // PositionAttitudeTransform seems to be slightly faster than MatrixTransform
            osg::ref_ptr<SceneUtil::PositionAttitudeTransform> trans(new SceneUtil::PositionAttitudeTransform);
            trans->setPosition(computeBound.getBoundingBox().center());

            node->addChild(trans);

            attachTo = trans;
        }

        osg::ref_ptr<LightSource> lightSource = createLightSource(esmLight, lightMask, isExterior, outQuadInLin, useQuadratic, quadraticValue,
                                                                  quadraticRadiusMult, useLinear, linearRadiusMult, linearValue);
        attachTo->addChild(lightSource);
    }

    osg::ref_ptr<LightSource> createLightSource(const ESM::Light* esmLight, unsigned int lightMask, bool isExterior, bool outQuadInLin, bool useQuadratic, float quadraticValue,
                                                float quadraticRadiusMult, bool useLinear, float linearRadiusMult, float linearValue, const osg::Vec4f& ambient)
    {
        osg::ref_ptr<SceneUtil::LightSource> lightSource (new SceneUtil::LightSource);
        osg::ref_ptr<osg::Light> light (new osg::Light);
        lightSource->setNodeMask(lightMask);

        float radius = esmLight->mData.mRadius;
        lightSource->setRadius(radius);

        configureLight(light, radius, isExterior, outQuadInLin, useQuadratic, quadraticValue,
                                  quadraticRadiusMult, useLinear, linearRadiusMult, linearValue);

        osg::Vec4f diffuse = SceneUtil::colourFromRGB(esmLight->mData.mColor);
        if (esmLight->mData.mFlags & ESM::Light::Negative)
        {
            diffuse *= -1;
            diffuse.a() = 1;
        }
        light->setDiffuse(diffuse);
        light->setAmbient(ambient);
        light->setSpecular(osg::Vec4f(0,0,0,0));

        lightSource->setLight(light);

        osg::ref_ptr<SceneUtil::LightController> ctrl (new SceneUtil::LightController);
        ctrl->setDiffuse(light->getDiffuse());
        if (esmLight->mData.mFlags & ESM::Light::Flicker)
            ctrl->setType(SceneUtil::LightController::LT_Flicker);
        if (esmLight->mData.mFlags & ESM::Light::FlickerSlow)
            ctrl->setType(SceneUtil::LightController::LT_FlickerSlow);
        if (esmLight->mData.mFlags & ESM::Light::Pulse)
            ctrl->setType(SceneUtil::LightController::LT_Pulse);
        if (esmLight->mData.mFlags & ESM::Light::PulseSlow)
            ctrl->setType(SceneUtil::LightController::LT_PulseSlow);

        lightSource->addUpdateCallback(ctrl);

        return lightSource;
    }
}
