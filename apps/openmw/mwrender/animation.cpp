#include "animation.hpp"

#include <osg/PositionAttitudeTransform>
#include <osg/TexGen>
#include <osg/TexEnvCombine>
#include <osg/ComputeBoundsVisitor>

#include <components/nifosg/nifloader.hpp>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/resource/texturemanager.hpp>

#include <components/misc/resourcehelpers.hpp>

#include <components/sceneutil/statesetupdater.hpp>
#include <components/sceneutil/visitor.hpp>
#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/util.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/class.hpp"

#include "vismask.hpp"

namespace
{

    class GlowUpdater : public SceneUtil::StateSetUpdater
    {
    public:
        GlowUpdater(osg::Vec4f color, const std::vector<osg::ref_ptr<osg::Texture2D> >& textures)
            : mTexUnit(1) // FIXME: might not always be 1
            , mColor(color)
            , mTextures(textures)
        {
        }

        virtual void setDefaults(osg::StateSet *stateset)
        {
            stateset->setTextureMode(mTexUnit, GL_TEXTURE_2D, osg::StateAttribute::ON);

            osg::TexGen* texGen = new osg::TexGen;
            texGen->setMode(osg::TexGen::SPHERE_MAP);

            stateset->setTextureAttributeAndModes(mTexUnit, texGen, osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);

            osg::TexEnvCombine* texEnv = new osg::TexEnvCombine;
            texEnv->setSource0_RGB(osg::TexEnvCombine::CONSTANT);
            texEnv->setConstantColor(mColor);
            texEnv->setCombine_RGB(osg::TexEnvCombine::INTERPOLATE);
            texEnv->setSource2_RGB(osg::TexEnvCombine::TEXTURE);
            texEnv->setOperand2_RGB(osg::TexEnvCombine::SRC_COLOR);

            stateset->setTextureAttributeAndModes(mTexUnit, texEnv, osg::StateAttribute::ON);
        }

        virtual void apply(osg::StateSet *stateset, osg::NodeVisitor *nv)
        {
            float time = nv->getFrameStamp()->getSimulationTime();
            int index = (int)(time*16) % mTextures.size();
            stateset->setTextureAttribute(mTexUnit, mTextures[index], osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
        }

    private:
        int mTexUnit;
        osg::Vec4f mColor;
        std::vector<osg::ref_ptr<osg::Texture2D> > mTextures;
    };

    class FindMaxControllerLengthVisitor : public SceneUtil::ControllerVisitor
    {
    public:
        FindMaxControllerLengthVisitor()
            : SceneUtil::ControllerVisitor()
            , mMaxLength(0)
        {
        }

        virtual void visit(osg::Node& , SceneUtil::Controller& ctrl)
        {
            if (ctrl.mFunction)
                mMaxLength = std::max(mMaxLength, ctrl.mFunction->getMaximum());
        }

        float getMaxLength() const
        {
            return mMaxLength;
        }

    private:
        float mMaxLength;
    };

}

namespace MWRender
{

    Animation::Animation(const MWWorld::Ptr &ptr, osg::ref_ptr<osg::Group> parentNode, Resource::ResourceSystem* resourceSystem)
        : mPtr(ptr)
        , mInsert(parentNode)
        , mResourceSystem(resourceSystem)
    {

    }

    Animation::~Animation()
    {
        if (mObjectRoot)
            mInsert->removeChild(mObjectRoot);
    }

    osg::Vec3f Animation::runAnimation(float duration)
    {
        updateEffects(duration);

        return osg::Vec3f();
    }

    void Animation::setObjectRoot(const std::string &model)
    {
        if (mObjectRoot)
        {
            mObjectRoot->getParent(0)->removeChild(mObjectRoot);
        }

        mObjectRoot = mResourceSystem->getSceneManager()->createInstance(model, mInsert);
    }

    osg::Group* Animation::getObjectRoot()
    {
        return static_cast<osg::Group*>(mObjectRoot.get());
    }

    osg::Group* Animation::getOrCreateObjectRoot()
    {
        if (mObjectRoot)
            return static_cast<osg::Group*>(mObjectRoot.get());

        mObjectRoot = new osg::Group;
        mInsert->addChild(mObjectRoot);
        return static_cast<osg::Group*>(mObjectRoot.get());
    }

    void Animation::addGlow(osg::ref_ptr<osg::Node> node, osg::Vec4f glowColor)
    {
        std::vector<osg::ref_ptr<osg::Texture2D> > textures;
        for (int i=0; i<32; ++i)
        {
            std::stringstream stream;
            stream << "textures/magicitem/caust";
            stream << std::setw(2);
            stream << std::setfill('0');
            stream << i;
            stream << ".dds";

            textures.push_back(mResourceSystem->getTextureManager()->getTexture2D(stream.str(), osg::Texture2D::REPEAT, osg::Texture2D::REPEAT));
        }

        osg::ref_ptr<GlowUpdater> glowupdater (new GlowUpdater(glowColor, textures));
        node->addUpdateCallback(glowupdater);
    }

    // TODO: Should not be here
    osg::Vec4f Animation::getEnchantmentColor(MWWorld::Ptr item)
    {
        osg::Vec4f result(1,1,1,1);
        std::string enchantmentName = item.getClass().getEnchantment(item);
        if (enchantmentName.empty())
            return result;
        const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(enchantmentName);
        assert (enchantment->mEffects.mList.size());
        const ESM::MagicEffect* magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(
                enchantment->mEffects.mList.front().mEffectID);
        result.x() = magicEffect->mData.mRed / 255.f;
        result.y() = magicEffect->mData.mGreen / 255.f;
        result.z() = magicEffect->mData.mBlue / 255.f;
        return result;
    }

    void Animation::addExtraLight(osg::ref_ptr<osg::Group> parent, const ESM::Light *esmLight)
    {
        SceneUtil::FindByNameVisitor visitor("AttachLight");
        parent->accept(visitor);

        osg::Group* attachTo = NULL;
        if (visitor.mFoundNode)
        {
            attachTo = visitor.mFoundNode;
        }
        else
        {
            osg::ComputeBoundsVisitor computeBound;
            parent->accept(computeBound);

            // PositionAttitudeTransform seems to be slightly faster than MatrixTransform
            osg::ref_ptr<osg::PositionAttitudeTransform> trans(new osg::PositionAttitudeTransform);
            trans->setPosition(computeBound.getBoundingBox().center());

            parent->addChild(trans);

            attachTo = trans;
        }

        osg::ref_ptr<SceneUtil::LightSource> lightSource = new SceneUtil::LightSource;
        osg::Light* light = new osg::Light;
        lightSource->setLight(light);

        float realRadius = esmLight->mData.mRadius;

        lightSource->setRadius(realRadius);
        light->setLinearAttenuation(10.f/(esmLight->mData.mRadius*2.f));
        //light->setLinearAttenuation(0.05);
        light->setConstantAttenuation(0.f);

        light->setDiffuse(SceneUtil::colourFromRGB(esmLight->mData.mColor));
        light->setAmbient(osg::Vec4f(0,0,0,1));
        light->setSpecular(osg::Vec4f(0,0,0,0));

        attachTo->addChild(lightSource);
    }

    void Animation::addEffect (const std::string& model, int effectId, bool loop, const std::string& bonename, std::string texture)
    {
        // Early out if we already have this effect
        for (std::vector<EffectParams>::iterator it = mEffects.begin(); it != mEffects.end(); ++it)
            if (it->mLoop && loop && it->mEffectId == effectId && it->mBoneName == bonename)
                return;

        EffectParams params;
        params.mModelName = model;
        osg::ref_ptr<osg::Group> parentNode;
        if (bonename.empty())
            parentNode = mObjectRoot->asGroup();
        else
        {
            SceneUtil::FindByNameVisitor visitor(bonename);
            mObjectRoot->accept(visitor);
            if (!visitor.mFoundNode)
                throw std::runtime_error("Can't find bone " + bonename);
            parentNode = visitor.mFoundNode;
        }
        osg::ref_ptr<osg::Node> node = mResourceSystem->getSceneManager()->createInstance(model, parentNode);
        params.mObjects = PartHolderPtr(new PartHolder(node));

        FindMaxControllerLengthVisitor findMaxLengthVisitor;
        node->accept(findMaxLengthVisitor);

        params.mMaxControllerLength = findMaxLengthVisitor.getMaxLength();

        node->setNodeMask(Mask_Effect);

        params.mLoop = loop;
        params.mEffectId = effectId;
        params.mBoneName = bonename;

        params.mAnimTime = boost::shared_ptr<EffectAnimationTime>(new EffectAnimationTime);

        SceneUtil::AssignControllerSourcesVisitor assignVisitor(boost::shared_ptr<SceneUtil::ControllerSource>(params.mAnimTime));
        node->accept(assignVisitor);

        if (!texture.empty())
        {
            std::string correctedTexture = Misc::ResourceHelpers::correctTexturePath(texture, mResourceSystem->getVFS());
            // Not sure if wrap settings should be pulled from the overridden texture?
            osg::ref_ptr<osg::Texture2D> tex = mResourceSystem->getTextureManager()->getTexture2D(correctedTexture, osg::Texture2D::CLAMP,
                                                                                                  osg::Texture2D::CLAMP);
            osg::ref_ptr<osg::StateSet> stateset;
            if (node->getStateSet())
                stateset = static_cast<osg::StateSet*>(node->getStateSet()->clone(osg::CopyOp::SHALLOW_COPY));
            else
                stateset = new osg::StateSet;

            stateset->setTextureAttribute(0, tex, osg::StateAttribute::OVERRIDE);

            node->setStateSet(stateset);
        }

        // TODO: in vanilla morrowind the effect is scaled based on the host object's bounding box.

        mEffects.push_back(params);
    }

    void Animation::removeEffect(int effectId)
    {
        for (std::vector<EffectParams>::iterator it = mEffects.begin(); it != mEffects.end(); ++it)
        {
            if (it->mEffectId == effectId)
            {
                mEffects.erase(it);
                return;
            }
        }
    }

    void Animation::getLoopingEffects(std::vector<int> &out)
    {
        for (std::vector<EffectParams>::iterator it = mEffects.begin(); it != mEffects.end(); ++it)
        {
            if (it->mLoop)
                out.push_back(it->mEffectId);
        }
    }

    void Animation::updateEffects(float duration)
    {
        for (std::vector<EffectParams>::iterator it = mEffects.begin(); it != mEffects.end(); )
        {
            it->mAnimTime->addTime(duration);

            if (it->mAnimTime->getTime() >= it->mMaxControllerLength)
            {
                if (it->mLoop)
                {
                    // Start from the beginning again; carry over the remainder
                    // Not sure if this is actually needed, the controller function might already handle loops
                    float remainder = it->mAnimTime->getTime() - it->mMaxControllerLength;
                    it->mAnimTime->resetTime(remainder);
                }
                else
                {
                    it = mEffects.erase(it);
                    continue;
                }
            }
            ++it;
        }
    }

    float Animation::EffectAnimationTime::getValue(osg::NodeVisitor*)
    {
        return mTime;
    }

    void Animation::EffectAnimationTime::addTime(float duration)
    {
        mTime += duration;
    }

    void Animation::EffectAnimationTime::resetTime(float time)
    {
        mTime = time;
    }

    float Animation::EffectAnimationTime::getTime() const
    {
        return mTime;
    }

    // --------------------------------------------------------------------------------

    ObjectAnimation::ObjectAnimation(const MWWorld::Ptr &ptr, const std::string &model, Resource::ResourceSystem* resourceSystem, bool allowLight)
        : Animation(ptr, osg::ref_ptr<osg::Group>(ptr.getRefData().getBaseNode()), resourceSystem)
    {
        if (!model.empty())
        {
            setObjectRoot(model);

            if (!ptr.getClass().getEnchantment(ptr).empty())
                addGlow(mObjectRoot, getEnchantmentColor(ptr));

            if (ptr.getTypeName() == typeid(ESM::Light).name() && allowLight)
                addExtraLight(getOrCreateObjectRoot(), ptr.get<ESM::Light>()->mBase);
        }
    }

}
