#include "animation.hpp"

#include <osg/PositionAttitudeTransform>
#include <osg/TexGen>
#include <osg/TexEnvCombine>

#include <components/nifosg/nifloader.hpp>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/resource/texturemanager.hpp>

#include <components/sceneutil/statesetupdater.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/class.hpp"

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

    // --------------------------------------------------------------------------------

    ObjectAnimation::ObjectAnimation(const MWWorld::Ptr &ptr, const std::string &model, Resource::ResourceSystem* resourceSystem)
        : Animation(ptr, osg::ref_ptr<osg::Group>(ptr.getRefData().getBaseNode()), resourceSystem)
    {
        if (!model.empty())
        {
            setObjectRoot(model);

            if (!ptr.getClass().getEnchantment(ptr).empty())
                addGlow(mObjectRoot, getEnchantmentColor(ptr));
        }
        else
        {
            // No model given. Create an object root anyway, so that lights can be added to it if needed.
            //mObjectRoot = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
        }
    }

}
