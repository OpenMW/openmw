#include "util.hpp"

#include <algorithm>
#include <array>
#include <iomanip>
#include <sstream>

#include <osg/FrameBufferObject>
#include <osg/Node>
#include <osg/NodeVisitor>
#include <osg/TexEnvCombine>
#include <osg/TexGen>
#include <osgUtil/CullVisitor>
#include <osgUtil/RenderStage>

#include <components/resource/imagemanager.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/texturetype.hpp>

namespace SceneUtil
{
    namespace
    {
        std::array<std::string, 32> generateGlowTextureNames()
        {
            std::array<std::string, 32> result;
            for (std::size_t i = 0; i < result.size(); ++i)
            {
                std::stringstream stream;
                stream << "textures/magicitem/caust";
                stream << std::setw(2);
                stream << std::setfill('0');
                stream << i;
                stream << ".dds";
                result[i] = std::move(stream).str();
            }
            return result;
        }

        const std::array<std::string, 32> glowTextureNames = generateGlowTextureNames();

        struct FindLowestUnusedTexUnitVisitor : public osg::NodeVisitor
        {
            FindLowestUnusedTexUnitVisitor()
                : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
            {
            }

            void apply(osg::Node& node) override
            {
                if (osg::StateSet* stateset = node.getStateSet())
                    mLowestUnusedTexUnit
                        = std::max(mLowestUnusedTexUnit, int(stateset->getTextureAttributeList().size()));

                traverse(node);
            }

            int mLowestUnusedTexUnit = 0;
        };
    }

    GlowUpdater::GlowUpdater(int texUnit, const osg::Vec4f& color,
        const std::vector<osg::ref_ptr<osg::Texture2D>>& textures, osg::Node* node, float duration,
        Resource::ResourceSystem* resourcesystem)
        : mTexUnit(texUnit)
        , mColor(color)
        , mOriginalColor(color)
        , mTextures(textures)
        , mNode(node)
        , mDuration(duration)
        , mOriginalDuration(duration)
        , mStartingTime(0)
        , mResourceSystem(resourcesystem)
        , mColorChanged(false)
        , mDone(false)
    {
    }

    void GlowUpdater::setDefaults(osg::StateSet* stateset)
    {
        if (mDone)
            removeTexture(stateset);
        else
        {
            stateset->setTextureMode(mTexUnit, GL_TEXTURE_2D, osg::StateAttribute::ON);
            osg::TexGen* texGen = new osg::TexGen;
            texGen->setMode(osg::TexGen::SPHERE_MAP);

            stateset->setTextureAttributeAndModes(
                mTexUnit, texGen, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

            osg::TexEnvCombine* texEnv = new osg::TexEnvCombine;
            texEnv->setSource0_RGB(osg::TexEnvCombine::CONSTANT);
            texEnv->setConstantColor(mColor);
            texEnv->setCombine_RGB(osg::TexEnvCombine::INTERPOLATE);
            texEnv->setSource2_RGB(osg::TexEnvCombine::TEXTURE);
            texEnv->setOperand2_RGB(osg::TexEnvCombine::SRC_COLOR);

            stateset->setTextureAttributeAndModes(mTexUnit, texEnv, osg::StateAttribute::ON);
            stateset->addUniform(new osg::Uniform("envMapColor", mColor));
        }
    }

    void GlowUpdater::removeTexture(osg::StateSet* stateset)
    {
        stateset->removeTextureAttribute(mTexUnit, osg::StateAttribute::TEXTURE);
        stateset->removeTextureAttribute(mTexUnit, osg::StateAttribute::TEXGEN);
        stateset->removeTextureAttribute(mTexUnit, osg::StateAttribute::TEXENV);
        stateset->removeTextureMode(mTexUnit, GL_TEXTURE_2D);
        stateset->removeUniform("envMapColor");

        osg::StateSet::TextureAttributeList& list = stateset->getTextureAttributeList();
        while (list.size() && list.rbegin()->empty())
            list.pop_back();
    }

    void GlowUpdater::apply(osg::StateSet* stateset, osg::NodeVisitor* nv)
    {
        if (mColorChanged)
        {
            this->reset();
            setDefaults(stateset);
            mColorChanged = false;
        }
        if (mDone)
            return;

        // Set the starting time to measure glow duration from if this is a temporary glow
        if ((mDuration >= 0) && mStartingTime == 0)
            mStartingTime = nv->getFrameStamp()->getSimulationTime();

        float time = nv->getFrameStamp()->getSimulationTime();
        int index = (int)(time * 16) % mTextures.size();
        stateset->setTextureAttribute(
            mTexUnit, mTextures[index], osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

        if ((mDuration >= 0)
            && (time - mStartingTime > mDuration)) // If this is a temporary glow and it has finished its duration
        {
            if (mOriginalDuration >= 0) // if this glowupdater was a temporary glow since its creation
            {
                removeTexture(stateset);
                this->reset();
                mDone = true;
                // normally done in StateSetUpdater::operator(), but needs doing here so the shader visitor sees the
                // right StateSet
                mNode->setStateSet(stateset);
                mResourceSystem->getSceneManager()->recreateShaders(mNode);
            }
            if (mOriginalDuration < 0) // if this glowupdater was originally a permanent glow
            {
                mDuration = mOriginalDuration;
                mStartingTime = 0;
                mColor = mOriginalColor;
                this->reset();
                setDefaults(stateset);
            }
        }
    }

    bool GlowUpdater::isPermanentGlowUpdater()
    {
        return (mDuration < 0);
    }

    bool GlowUpdater::isDone()
    {
        return mDone;
    }

    void GlowUpdater::setColor(const osg::Vec4f& color)
    {
        mColor = color;
        mColorChanged = true;
    }

    void GlowUpdater::setDuration(float duration)
    {
        mDuration = duration;
    }

    osg::Vec4f colourFromRGB(unsigned int clr)
    {
        osg::Vec4f colour(
            ((clr >> 0) & 0xFF) / 255.0f, ((clr >> 8) & 0xFF) / 255.0f, ((clr >> 16) & 0xFF) / 255.0f, 1.f);
        return colour;
    }

    osg::Vec4f colourFromRGBA(unsigned int value)
    {
        return osg::Vec4f(makeOsgColorComponent(value, 0), makeOsgColorComponent(value, 8),
            makeOsgColorComponent(value, 16), makeOsgColorComponent(value, 24));
    }

    float makeOsgColorComponent(unsigned int value, unsigned int shift)
    {
        return float((value >> shift) & 0xFFu) / 255.0f;
    }

    bool hasUserDescription(const osg::Node* node, std::string_view pattern)
    {
        if (node == nullptr)
            return false;

        const osg::UserDataContainer* udc = node->getUserDataContainer();
        if (udc && udc->getNumDescriptions() > 0)
        {
            for (auto& descr : udc->getDescriptions())
            {
                if (descr == pattern)
                    return true;
            }
        }

        return false;
    }

    osg::ref_ptr<GlowUpdater> addEnchantedGlow(osg::ref_ptr<osg::Node> node, Resource::ResourceSystem* resourceSystem,
        const osg::Vec4f& glowColor, float glowDuration)
    {
        std::vector<osg::ref_ptr<osg::Texture2D>> textures;
        for (const std::string& name : glowTextureNames)
        {
            osg::ref_ptr<osg::Image> image = resourceSystem->getImageManager()->getImage(VFS::Path::toNormalized(name));
            osg::ref_ptr<osg::Texture2D> tex(new osg::Texture2D(image));
            tex->setWrap(osg::Texture::WRAP_S, osg::Texture2D::REPEAT);
            tex->setWrap(osg::Texture::WRAP_T, osg::Texture2D::REPEAT);
            resourceSystem->getSceneManager()->applyFilterSettings(tex);
            textures.push_back(tex);
        }

        FindLowestUnusedTexUnitVisitor findLowestUnusedTexUnitVisitor;
        node->accept(findLowestUnusedTexUnitVisitor);
        int texUnit = findLowestUnusedTexUnitVisitor.mLowestUnusedTexUnit;

        osg::ref_ptr<GlowUpdater> glowUpdater
            = new GlowUpdater(texUnit, glowColor, textures, node, glowDuration, resourceSystem);
        node->addUpdateCallback(glowUpdater);

        // set a texture now so that the ShaderVisitor can find it
        osg::ref_ptr<osg::StateSet> writableStateSet = nullptr;
        if (!node->getStateSet())
            writableStateSet = node->getOrCreateStateSet();
        else
        {
            writableStateSet = new osg::StateSet(*node->getStateSet(), osg::CopyOp::SHALLOW_COPY);
            node->setStateSet(writableStateSet);
        }
        writableStateSet->setTextureAttributeAndModes(texUnit, textures.front(), osg::StateAttribute::ON);
        writableStateSet->setTextureAttributeAndModes(texUnit, new TextureType("envMap"), osg::StateAttribute::ON);
        writableStateSet->addUniform(new osg::Uniform("envMapColor", glowColor));
        resourceSystem->getSceneManager()->recreateShaders(std::move(node));

        return glowUpdater;
    }

    void attachAlphaToCoverageFriendlyFramebufferToCamera(osg::Camera* camera, osg::Camera::BufferComponent buffer,
        osg::Texture* texture, unsigned int level, unsigned int face, bool mipMapGeneration,
        bool addMSAAIntermediateTarget)
    {
        unsigned int samples = 0;
        unsigned int colourSamples = 0;
        if (addMSAAIntermediateTarget)
        {
            // Alpha-to-coverage requires a multisampled framebuffer.
            // OSG will set that up automatically and resolve it to the specified single-sample texture for us.
            // For some reason, two samples are needed, at least with some drivers.
            samples = 2;
            colourSamples = 1;
        }
        camera->attach(buffer, texture, level, face, mipMapGeneration, samples, colourSamples);
    }

    OperationSequence::OperationSequence(bool keep)
        : Operation("OperationSequence", keep)
        , mOperationQueue(new osg::OperationQueue())
    {
    }

    void OperationSequence::operator()(osg::Object* object)
    {
        mOperationQueue->runOperations(object);
    }

    void OperationSequence::add(osg::Operation* operation)
    {
        mOperationQueue->add(operation);
    }

    GLenum computeUnsizedPixelFormat(GLenum format)
    {
        switch (format)
        {
            // Try compressed formats first, they're more likely to be used

            // Generic
            case GL_COMPRESSED_ALPHA_ARB:
                return GL_ALPHA;
            case GL_COMPRESSED_INTENSITY_ARB:
                return GL_INTENSITY;
            case GL_COMPRESSED_LUMINANCE_ALPHA_ARB:
                return GL_LUMINANCE_ALPHA;
            case GL_COMPRESSED_LUMINANCE_ARB:
                return GL_LUMINANCE;
            case GL_COMPRESSED_RGB_ARB:
                return GL_RGB;
            case GL_COMPRESSED_RGBA_ARB:
                return GL_RGBA;

            // S3TC
            case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
            case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
                return GL_RGB;
            case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
            case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
            case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
                return GL_RGBA;

            // RGTC
            case GL_COMPRESSED_RED_RGTC1_EXT:
            case GL_COMPRESSED_SIGNED_RED_RGTC1_EXT:
                return GL_RED;
            case GL_COMPRESSED_RED_GREEN_RGTC2_EXT:
            case GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT:
                return GL_RG;

            // PVRTC
            case GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG:
            case GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG:
                return GL_RGB;
            case GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG:
            case GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG:
                return GL_RGBA;

            // ETC
            case GL_COMPRESSED_R11_EAC:
            case GL_COMPRESSED_SIGNED_R11_EAC:
                return GL_RED;
            case GL_COMPRESSED_RG11_EAC:
            case GL_COMPRESSED_SIGNED_RG11_EAC:
                return GL_RG;
            case GL_ETC1_RGB8_OES:
            case GL_COMPRESSED_RGB8_ETC2:
            case GL_COMPRESSED_SRGB8_ETC2:
                return GL_RGB;
            case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
            case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
            case GL_COMPRESSED_RGBA8_ETC2_EAC:
            case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
                return GL_RGBA;

            // ASTC
            case GL_COMPRESSED_RGBA_ASTC_4x4_KHR:
            case GL_COMPRESSED_RGBA_ASTC_5x4_KHR:
            case GL_COMPRESSED_RGBA_ASTC_5x5_KHR:
            case GL_COMPRESSED_RGBA_ASTC_6x5_KHR:
            case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:
            case GL_COMPRESSED_RGBA_ASTC_8x5_KHR:
            case GL_COMPRESSED_RGBA_ASTC_8x6_KHR:
            case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:
            case GL_COMPRESSED_RGBA_ASTC_10x5_KHR:
            case GL_COMPRESSED_RGBA_ASTC_10x6_KHR:
            case GL_COMPRESSED_RGBA_ASTC_10x8_KHR:
            case GL_COMPRESSED_RGBA_ASTC_10x10_KHR:
            case GL_COMPRESSED_RGBA_ASTC_12x10_KHR:
            case GL_COMPRESSED_RGBA_ASTC_12x12_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR:
            case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR:
                return GL_RGBA;

            // Plug in some holes computePixelFormat has, you never know when these could come in handy
            case GL_INTENSITY4:
            case GL_INTENSITY8:
            case GL_INTENSITY12:
            case GL_INTENSITY16:
                return GL_INTENSITY;

            case GL_LUMINANCE4:
            case GL_LUMINANCE8:
            case GL_LUMINANCE12:
            case GL_LUMINANCE16:
                return GL_LUMINANCE;

            case GL_LUMINANCE4_ALPHA4:
            case GL_LUMINANCE6_ALPHA2:
            case GL_LUMINANCE8_ALPHA8:
            case GL_LUMINANCE12_ALPHA4:
            case GL_LUMINANCE12_ALPHA12:
            case GL_LUMINANCE16_ALPHA16:
                return GL_LUMINANCE_ALPHA;
        }

        return osg::Image::computePixelFormat(format);
    }

    const std::string& getTextureType(const osg::StateSet& stateset, const osg::Texture& texture, unsigned int texUnit)
    {
        const osg::StateAttribute* type = stateset.getTextureAttribute(texUnit, SceneUtil::TextureType::AttributeType);
        if (type)
            return static_cast<const SceneUtil::TextureType*>(type)->getName();

        return texture.getName();
    }
}
