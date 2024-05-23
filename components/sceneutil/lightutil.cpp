#include "lightutil.hpp"

#include <osg/Group>
#include <osg/Light>

#include <osgParticle/ParticleSystem>

#include <components/esm3/loadligh.hpp>
#include <components/fallback/fallback.hpp>
#include <components/sceneutil/lightcommon.hpp>

#include "lightcontroller.hpp"
#include "lightmanager.hpp"
#include "visitor.hpp"

namespace
{
    class CheckEmptyLightVisitor : public osg::NodeVisitor
    {
    public:
        CheckEmptyLightVisitor()
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        {
        }

        void apply(osg::Drawable& drawable) override
        {
            if (!mEmpty)
                return;

            if (dynamic_cast<const osgParticle::ParticleSystem*>(&drawable))
                mEmpty = false;
            else
                traverse(drawable);
        }

        void apply(osg::Geometry& geometry) override { mEmpty = false; }

        bool mEmpty = true;
    };
}

namespace SceneUtil
{

    void configureLight(osg::Light* light, float radius, bool isExterior)
    {
        float quadraticAttenuation = 0.f;
        float linearAttenuation = 0.f;
        float constantAttenuation = 0.f;

        static const bool useConstant = Fallback::Map::getBool("LightAttenuation_UseConstant");
        static const bool useLinear = Fallback::Map::getBool("LightAttenuation_UseLinear");
        static const bool useQuadratic = Fallback::Map::getBool("LightAttenuation_UseQuadratic");
        static const float constantValue = Fallback::Map::getFloat("LightAttenuation_ConstantValue");
        static const float linearValue = Fallback::Map::getFloat("LightAttenuation_LinearValue");
        static const float quadraticValue = Fallback::Map::getFloat("LightAttenuation_QuadraticValue");
        static const float linearRadiusMult = Fallback::Map::getFloat("LightAttenuation_LinearRadiusMult");
        static const float quadraticRadiusMult = Fallback::Map::getFloat("LightAttenuation_QuadraticRadiusMult");
        static const int linearMethod = Fallback::Map::getInt("LightAttenuation_LinearMethod");
        static const int quadraticMethod = Fallback::Map::getInt("LightAttenuation_QuadraticMethod");
        static const bool outQuadInLin = Fallback::Map::getBool("LightAttenuation_OutQuadInLin");

        if (useConstant)
            constantAttenuation = constantValue;

        if (useLinear)
        {
            linearAttenuation = linearMethod == 0 ? linearValue : 0.01f;
            float r = radius * linearRadiusMult;
            if (r && (linearMethod == 1 || linearMethod == 2))
                linearAttenuation = linearValue / std::pow(r, linearMethod);
        }

        if (useQuadratic && (!outQuadInLin || isExterior))
        {
            quadraticAttenuation = quadraticMethod == 0 ? quadraticValue : 0.01f;
            float r = radius * quadraticRadiusMult;
            if (r && (quadraticMethod == 1 || quadraticMethod == 2))
                quadraticAttenuation = quadraticValue / std::pow(r, quadraticMethod);
        }

        light->setConstantAttenuation(constantAttenuation);
        light->setLinearAttenuation(linearAttenuation);
        light->setQuadraticAttenuation(quadraticAttenuation);
    }

    osg::ref_ptr<LightSource> addLight(
        osg::Group* node, const SceneUtil::LightCommon& esmLight, unsigned int lightMask, bool isExterior)
    {
        SceneUtil::FindByNameVisitor visitor("AttachLight");
        node->accept(visitor);

        osg::Group* attachTo = visitor.mFoundNode ? visitor.mFoundNode : node;
        osg::ref_ptr<LightSource> lightSource
            = createLightSource(esmLight, lightMask, isExterior, osg::Vec4f(0, 0, 0, 1));
        attachTo->addChild(lightSource);

        CheckEmptyLightVisitor emptyVisitor;
        node->accept(emptyVisitor);

        lightSource->setEmpty(emptyVisitor.mEmpty);

        return lightSource;
    }

    osg::ref_ptr<LightSource> createLightSource(
        const SceneUtil::LightCommon& esmLight, unsigned int lightMask, bool isExterior, const osg::Vec4f& ambient)
    {
        osg::ref_ptr<SceneUtil::LightSource> lightSource(new SceneUtil::LightSource);
        osg::ref_ptr<osg::Light> light(new osg::Light);
        lightSource->setNodeMask(lightMask);

        float radius = esmLight.mRadius;
        lightSource->setRadius(radius);

        configureLight(light, radius, isExterior);

        osg::Vec4f diffuse = esmLight.mColor;
        osg::Vec4f specular = esmLight.mColor; // ESM format doesn't provide specular
        if (esmLight.mNegative)
        {
            diffuse *= -1;
            diffuse.a() = 1;
            // Using specular lighting for negative lights is unreasonable
            specular = osg::Vec4f();
        }
        light->setDiffuse(diffuse);
        light->setAmbient(ambient);
        light->setSpecular(specular);

        lightSource->setLight(light);

        osg::ref_ptr<SceneUtil::LightController> ctrl(new SceneUtil::LightController);
        ctrl->setDiffuse(light->getDiffuse());
        ctrl->setSpecular(light->getSpecular());
        if (esmLight.mFlicker)
            ctrl->setType(SceneUtil::LightController::LT_Flicker);
        if (esmLight.mFlickerSlow)
            ctrl->setType(SceneUtil::LightController::LT_FlickerSlow);
        if (esmLight.mPulse)
            ctrl->setType(SceneUtil::LightController::LT_Pulse);
        if (esmLight.mPulseSlow)
            ctrl->setType(SceneUtil::LightController::LT_PulseSlow);

        lightSource->addUpdateCallback(ctrl);

        return lightSource;
    }
}
