#include "lightutil.hpp"

#include <osg/LightSource>
#include <osg/Group>
#include <osg/ComputeBoundsVisitor>
#include <osg/ValueObject>

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

    class DirectionalLightUpdater : public osg::NodeCallback
    {
    public:
        META_Object(SceneUtil, DirectionalLightUpdater)

        DirectionalLightUpdater() : osg::NodeCallback() {}
        DirectionalLightUpdater(const DirectionalLightUpdater &copy, const osg::CopyOp &copyop) : osg::NodeCallback(copy, copyop) {}

        void operator() (osg::Node* node, osg::NodeVisitor* nv)
        {
            SceneUtil::LightSource* lightsource = static_cast<SceneUtil::LightSource*>(node);
            osg::Light * light = lightsource->getLight(nv->getTraversalNumber());
            osg::Matrix ptrans = osg::computeLocalToWorld(nv->getNodePath());
            osg::Vec3 dir = osg::Matrix::transform3x3(ptrans, osg::Vec3(1,0,0));
            light->setPosition(osg::Vec4(dir, 0.f));
            traverse(node, nv);
        }
    };
    void ConvertOsgLightSourceVisitor::apply(osg::Group& node)
    {
        for(unsigned int i=0; i<node.getNumChildren(); ++i)
        {
            osg::LightSource *ls = dynamic_cast<osg::LightSource*>(node.getChild(i));
            if(ls)
            {
                //replace ls with SceneUtil ls
                SceneUtil::LightSource* nls = new SceneUtil::LightSource();
                nls->setUpdateCallback(ls->getUpdateCallback());
                nls->setUserDataContainer(ls->getUserDataContainer());

                nls->setNodeMask(mLightMask);
                osg::Light* light = new osg::Light(*ls->getLight());
                float radius = 0;
                osg::FloatValueObject* fo = dynamic_cast<osg::FloatValueObject*>(ls->getUserData());
                if(fo)
                {
                    radius = fo->getValue();
                    if(light->getPosition().w() == 0) //directional light
                    {
                        SceneUtil::configureLight(light, radius, mIsExterior);
                        nls->addUpdateCallback(new DirectionalLightUpdater());
                    }
                    nls->setRadius(radius * 10.0f);//TOFIX
                }
                nls->setLight(light);

               node.removeChild(i);
               node.insertChild(i, nls);

            }
        }
        traverse(node);
    }
}
