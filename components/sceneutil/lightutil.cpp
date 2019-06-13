#include "lightutil.hpp"

#include <osg/Light>
#include <osg/Group>
#include <osg/ComputeBoundsVisitor>

#include <components/esm/loadligh.hpp>
#include <components/fallback/fallback.hpp>

#include "lightmanager.hpp"
#include "lightcontroller.hpp"
#include "util.hpp"
#include "visitor.hpp"
#include "positionattitudetransform.hpp"

namespace SceneUtil
{

    void configureLight(osg::Light *light, float radius, bool isExterior)
    {
        float quadraticAttenuation = 0.f;
        float linearAttenuation = 0.f;
        float constantAttenuation = 0.f;

        const bool useConstant = Fallback::Map::getBool("LightAttenuation_UseConstant");
        if (useConstant)
        {
            constantAttenuation = Fallback::Map::getFloat("LightAttenuation_ConstantValue");
        }

        const bool useLinear = Fallback::Map::getBool("LightAttenuation_UseLinear");
        if (useLinear)
        {
            const float linearValue = Fallback::Map::getFloat("LightAttenuation_LinearValue");
            const float linearRadiusMult = Fallback::Map::getFloat("LightAttenuation_LinearRadiusMult");
            float r = radius * linearRadiusMult;
            if (r) linearAttenuation = linearValue / r;
        }

        const bool useQuadratic = Fallback::Map::getBool("LightAttenuation_UseQuadratic");
        const bool outQuadInLin = Fallback::Map::getBool("LightAttenuation_OutQuadInLin");
        if (useQuadratic && (!outQuadInLin || isExterior))
        {
            const float quadraticValue = Fallback::Map::getFloat("LightAttenuation_QuadraticValue");
            const float quadraticRadiusMult = Fallback::Map::getFloat("LightAttenuation_QuadraticRadiusMult");
            float r = radius * quadraticRadiusMult;
            if (r) quadraticAttenuation = quadraticValue / std::pow(r, 2);
        }

        light->setConstantAttenuation(constantAttenuation);
        light->setLinearAttenuation(linearAttenuation);
        light->setQuadraticAttenuation(quadraticAttenuation);
    }

    void addLight (osg::Group* node, const ESM::Light* esmLight, unsigned int partsysMask, unsigned int lightMask, bool isExterior)
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

        osg::ref_ptr<LightSource> lightSource = createLightSource(esmLight, lightMask, isExterior);
        attachTo->addChild(lightSource);
    }

    osg::ref_ptr<LightSource> createLightSource(const ESM::Light* esmLight, unsigned int lightMask, bool isExterior, const osg::Vec4f& ambient)
    {
        osg::ref_ptr<SceneUtil::LightSource> lightSource (new SceneUtil::LightSource);
        osg::ref_ptr<osg::Light> light (new osg::Light);
        lightSource->setNodeMask(lightMask);

        float radius = esmLight->mData.mRadius;
        lightSource->setRadius(radius);

        configureLight(light, radius, isExterior);

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
