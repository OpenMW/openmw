#include "util.hpp"

#include <osg/Node>
#include <osg/ValueObject>

#include <components/misc/resourcehelpers.hpp>
#include <components/resource/imagemanager.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/sceneutil/texturetype.hpp>
#include <components/sceneutil/visitor.hpp>
#include <components/settings/values.hpp>

namespace MWRender
{
    namespace
    {
        class TextureOverrideVisitor : public osg::NodeVisitor
        {
        public:
            TextureOverrideVisitor(std::string_view texture, Resource::ResourceSystem* resourcesystem)
                : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
                , mTexture(texture)
                , mResourcesystem(resourcesystem)
            {
            }

            void apply(osg::Node& node) override
            {
                int index = 0;
                if (node.getUserValue("overrideFx", index))
                {
                    if (index == 1)
                        overrideTexture(mTexture, mResourcesystem, node);
                }
                traverse(node);
            }
            std::string_view mTexture;
            Resource::ResourceSystem* mResourcesystem;
        };
    }

    void overrideFirstRootTexture(std::string_view texture, Resource::ResourceSystem* resourceSystem, osg::Node& node)
    {
        TextureOverrideVisitor overrideVisitor(texture, resourceSystem);
        node.accept(overrideVisitor);
    }

    void overrideTexture(std::string_view texture, Resource::ResourceSystem* resourceSystem, osg::Node& node)
    {
        if (texture.empty())
            return;
        const VFS::Path::Normalized correctedTexture
            = Misc::ResourceHelpers::correctTexturePath(texture, resourceSystem->getVFS());
        // Not sure if wrap settings should be pulled from the overridden texture?
        osg::ref_ptr<osg::Texture2D> tex
            = new osg::Texture2D(resourceSystem->getImageManager()->getImage(correctedTexture));
        tex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        tex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);

        osg::ref_ptr<osg::StateSet> stateset;
        if (const osg::StateSet* const src = node.getStateSet())
            stateset = new osg::StateSet(*src, osg::CopyOp::SHALLOW_COPY);
        else
            stateset = new osg::StateSet;

        stateset->setTextureAttribute(0, tex, osg::StateAttribute::OVERRIDE);
        stateset->setTextureAttribute(0, new SceneUtil::TextureType("diffuseMap"), osg::StateAttribute::OVERRIDE);

        node.setStateSet(stateset);
    }

    bool shouldAddMSAAIntermediateTarget()
    {
        return Settings::shaders().mAntialiasAlphaTest && Settings::video().mAntialiasing > 1;
    }

    osg::ref_ptr<osg::LightModel> makeVFXLightModelInstance()
    {
        osg::ref_ptr<osg::LightModel> lightModel = new osg::LightModel;
        lightModel->setAmbientIntensity({ 1, 1, 1, 1 });
        return lightModel;
    }

    const osg::ref_ptr<osg::LightModel>& getVFXLightModelInstance()
    {
        static const osg::ref_ptr<osg::LightModel> lightModel = makeVFXLightModelInstance();
        return lightModel;
    }
}
