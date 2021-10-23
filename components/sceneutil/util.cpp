#include "util.hpp"

#include <algorithm>
#include <sstream>
#include <iomanip>

#include <SDL_opengl_glext.h>

#include <osg/Node>
#include <osg/NodeVisitor>
#include <osg/TexGen>
#include <osg/TexEnvCombine>
#include <osg/Version>
#include <osg/FrameBufferObject>
#include <osgUtil/RenderStage>
#include <osgUtil/CullVisitor>

#include <components/resource/imagemanager.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/settings/settings.hpp>
#include <components/debug/debuglog.hpp>
#include <components/sceneutil/nodecallback.hpp>

#ifndef GL_DEPTH32F_STENCIL8_NV
#define GL_DEPTH32F_STENCIL8_NV 0x8DAC
#endif

namespace
{

bool isReverseZSupported()
{
    if (!Settings::Manager::mDefaultSettings.count({"Camera", "reverse z"}))
        return false;
    auto ext = osg::GLExtensions::Get(0, false);
    return Settings::Manager::getBool("reverse z", "Camera") && ext && ext->isClipControlSupported;
}

}

namespace SceneUtil
{

class FindLowestUnusedTexUnitVisitor : public osg::NodeVisitor
{
public:
    FindLowestUnusedTexUnitVisitor()
        : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        , mLowestUnusedTexUnit(0)
    {
    }

    void apply(osg::Node& node) override
    {
        if (osg::StateSet* stateset = node.getStateSet())
            mLowestUnusedTexUnit = std::max(mLowestUnusedTexUnit, int(stateset->getTextureAttributeList().size()));

        traverse(node);
    }
    int mLowestUnusedTexUnit;
};

GlowUpdater::GlowUpdater(int texUnit, const osg::Vec4f& color, float duration, Resource::ResourceSystem* resourcesystem)
    : mTexUnit(texUnit)
    , mColor(color)
    , mOriginalColor(color)
    , mDuration(duration)
    , mOriginalDuration(duration)
    , mStartingTime(0)
    , mResourceSystem(resourcesystem)
    , mDone(false)
{
}

osg::StateSet* GlowUpdater::getStateSet(int index)
{
    CachedState& cs = getCachedState();
    if (!cs.mTextures)
    {
        for (int i=0; i<32; ++i)
        {
            std::stringstream stream;
            stream << "textures/magicitem/caust";
            stream << std::setw(2);
            stream << std::setfill('0');
            stream << i;
            stream << ".dds";

            osg::ref_ptr<osg::Image> image = resourceSystem->getImageManager()->getImage(stream.str());
            osg::ref_ptr<osg::Texture2D> tex (new osg::Texture2D(image));
            tex->setName("envMap");
            tex->setWrap(osg::Texture::WRAP_S, osg::Texture2D::REPEAT);
            tex->setWrap(osg::Texture::WRAP_T, osg::Texture2D::REPEAT);
            mResourceSystem->getSceneManager()->applyFilterSettings(tex);
            cs.mTextures.push_back(tex);
        }
    }
    std::vector<osg::ref_ptr<osg::StateSet>>& sequence = cs.mStateSets[std::make_pair(mTexUnit, mColor)];
    if (sequence.empty())
    {
        sequence.resize(cs.mTextures.size());
        for (int i=0; i<cs.mTextures.size(); ++i)
        {
            sequence[i] = new osg::StateSet;
            osg::StateSet* stateset = sequence[i];
            stateset->setTextureAttributeAndModes(mTexUnit, cs.mTextures[i], osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
            osg::ref_ptr<osg::TexGen> texGen = new osg::TexGen;
            texGen->setMode(osg::TexGen::SPHERE_MAP);
            stateset->setTextureAttributeAndModes(mTexUnit, texGen, osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
            osg::ref_ptr<osg::TexEnvCombine> texEnv = new osg::TexEnvCombine;
            texEnv->setSource0_RGB(osg::TexEnvCombine::CONSTANT);
            texEnv->setConstantColor(mColor);
            texEnv->setCombine_RGB(osg::TexEnvCombine::INTERPOLATE);
            texEnv->setSource2_RGB(osg::TexEnvCombine::TEXTURE);
            texEnv->setOperand2_RGB(osg::TexEnvCombine::SRC_COLOR);
            stateset->setTextureAttributeAndModes(mTexUnit, texEnv, osg::StateAttribute::ON);
            stateset->addUniform(new osg::Uniform("envMapColor", mColor));
            stateset->setDefine("GLOW");
        }
    }
    return sequence[index];
}

void GlowUpdater::operator()(osg::Node* node, osgUtil::CullVisitor *cv)
{
    // Set the starting time to measure glow duration from if this is a temporary glow
    if ((mDuration >= 0) && mStartingTime == 0)
        mStartingTime = nv->getFrameStamp()->getSimulationTime();

    if ((mDuration >= 0) && (time - mStartingTime > mDuration)) // If this is a temporary glow and it has finished its duration
    {
        if (mOriginalDuration >= 0) // if this glowupdater was a temporary glow since its creation
        {
            mDone = true;
        }
        if (mOriginalDuration < 0) // if this glowupdater was originally a permanent glow
        {
            mDuration = mOriginalDuration;
            mStartingTime = 0;
            mColor = mOriginalColor;
        }
    }

    if (mDone)
    {
        traverse(node, cv);
        return;
    }

    float time = nv->getFrameStamp()->getSimulationTime();
    int index = (int)(time*16) % mTextures.size();
    cv->pushStateSet(getStateSet(index));
    traverse(node, cv);
    cv->popStateSet();
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
}

void GlowUpdater::setDuration(float duration)
{
    mDuration = duration;
}

// Allows camera to render to a color and floating point depth texture with a multisampled framebuffer.
class AttachMultisampledDepthColorCallback : public SceneUtil::NodeCallback<AttachMultisampledDepthColorCallback, osg::Node*, osgUtil::CullVisitor*>
{
public:
    AttachMultisampledDepthColorCallback(osg::Texture2D* colorTex, osg::Texture2D* depthTex, int samples, int colorSamples)
    {
        int width = colorTex->getTextureWidth();
        int height = colorTex->getTextureHeight();

        osg::ref_ptr<osg::RenderBuffer> rbColor = new osg::RenderBuffer(width, height, colorTex->getInternalFormat(), samples, colorSamples);
        osg::ref_ptr<osg::RenderBuffer> rbDepth = new osg::RenderBuffer(width, height, depthTex->getInternalFormat(), samples, colorSamples);

        mMsaaFbo = new osg::FrameBufferObject;
        mMsaaFbo->setAttachment(osg::Camera::COLOR_BUFFER0, osg::FrameBufferAttachment(rbColor));
        mMsaaFbo->setAttachment(osg::Camera::DEPTH_BUFFER, osg::FrameBufferAttachment(rbDepth));

        mFbo = new osg::FrameBufferObject;
        mFbo->setAttachment(osg::Camera::COLOR_BUFFER0, osg::FrameBufferAttachment(colorTex));
        mFbo->setAttachment(osg::Camera::DEPTH_BUFFER, osg::FrameBufferAttachment(depthTex));
    }

    void operator()(osg::Node* node, osgUtil::CullVisitor* cv)
    {
        osgUtil::RenderStage* renderStage = cv->getCurrentRenderStage();

        renderStage->setMultisampleResolveFramebufferObject(mFbo);
        renderStage->setFrameBufferObject(mMsaaFbo);

        traverse(node, cv);
    }

private:
    osg::ref_ptr<osg::FrameBufferObject> mFbo;
    osg::ref_ptr<osg::FrameBufferObject> mMsaaFbo;
};

void transformBoundingSphere (const osg::Matrixf& matrix, osg::BoundingSphere& bsphere)
{
    osg::BoundingSphere::vec_type xdash = bsphere._center;
    xdash.x() += bsphere._radius;
    xdash = xdash*matrix;

    osg::BoundingSphere::vec_type ydash = bsphere._center;
    ydash.y() += bsphere._radius;
    ydash = ydash*matrix;

    osg::BoundingSphere::vec_type zdash = bsphere._center;
    zdash.z() += bsphere._radius;
    zdash = zdash*matrix;

    bsphere._center = bsphere._center*matrix;

    xdash -= bsphere._center;
    osg::BoundingSphere::value_type sqrlen_xdash = xdash.length2();

    ydash -= bsphere._center;
    osg::BoundingSphere::value_type sqrlen_ydash = ydash.length2();

    zdash -= bsphere._center;
    osg::BoundingSphere::value_type sqrlen_zdash = zdash.length2();

    bsphere._radius = sqrlen_xdash;
    if (bsphere._radius<sqrlen_ydash) bsphere._radius = sqrlen_ydash;
    if (bsphere._radius<sqrlen_zdash) bsphere._radius = sqrlen_zdash;
    bsphere._radius = sqrtf(bsphere._radius);
}

osg::Vec4f colourFromRGB(unsigned int clr)
{
    osg::Vec4f colour(((clr >> 0) & 0xFF) / 255.0f,
                      ((clr >> 8) & 0xFF) / 255.0f,
                      ((clr >> 16) & 0xFF) / 255.0f, 1.f);
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

bool hasUserDescription(const osg::Node* node, const std::string& pattern)
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

osg::ref_ptr<GlowUpdater> addEnchantedGlow(osg::ref_ptr<osg::Node> node, Resource::ResourceSystem* resourceSystem, const osg::Vec4f& glowColor, float glowDuration)
{
    FindLowestUnusedTexUnitVisitor findLowestUnusedTexUnitVisitor;
    node->accept(findLowestUnusedTexUnitVisitor);
    int texUnit = findLowestUnusedTexUnitVisitor.mLowestUnusedTexUnit;

    osg::ref_ptr<GlowUpdater> glowUpdater = new GlowUpdater(texUnit, glowColor, glowDuration, resourceSystem);
    node->addCullCallback(glowUpdater);

    return glowUpdater;
}

bool attachAlphaToCoverageFriendlyFramebufferToCamera(osg::Camera* camera, osg::Camera::BufferComponent buffer, osg::Texture * texture, unsigned int level, unsigned int face, bool mipMapGeneration)
{
#if OSG_VERSION_LESS_THAN(3, 6, 6)
    // hack fix for https://github.com/openscenegraph/OpenSceneGraph/issues/1028
    osg::ref_ptr<osg::GLExtensions> extensions = osg::GLExtensions::Get(0, false);
    if (extensions)
        extensions->glRenderbufferStorageMultisampleCoverageNV = nullptr;
#endif
    unsigned int samples = 0;
    unsigned int colourSamples = 0;
    bool addMSAAIntermediateTarget = Settings::Manager::getBool("antialias alpha test", "Shaders") && Settings::Manager::getInt("antialiasing", "Video") > 1;
    if (addMSAAIntermediateTarget)
    {
        // Alpha-to-coverage requires a multisampled framebuffer.
        // OSG will set that up automatically and resolve it to the specified single-sample texture for us.
        // For some reason, two samples are needed, at least with some drivers.
        samples = 2;
        colourSamples = 1;
    }
    camera->attach(buffer, texture, level, face, mipMapGeneration, samples, colourSamples);
    return addMSAAIntermediateTarget;
}

void attachAlphaToCoverageFriendlyDepthColor(osg::Camera* camera, osg::Texture2D* colorTex, osg::Texture2D* depthTex, GLenum depthFormat)
{
    bool addMSAAIntermediateTarget = Settings::Manager::getBool("antialias alpha test", "Shaders") && Settings::Manager::getInt("antialiasing", "Video") > 1;

    if (isFloatingPointDepthFormat(depthFormat) && addMSAAIntermediateTarget)
    {
        camera->attach(osg::Camera::COLOR_BUFFER0, colorTex);
        camera->attach(osg::Camera::DEPTH_BUFFER, depthTex);
        camera->addCullCallback(new AttachMultisampledDepthColorCallback(colorTex, depthTex, 2, 1));
    }
    else
    {
        attachAlphaToCoverageFriendlyFramebufferToCamera(camera, osg::Camera::COLOR_BUFFER, colorTex);
        camera->attach(osg::Camera::DEPTH_BUFFER, depthTex);
    }
}

bool getReverseZ()
{
    static bool reverseZ = isReverseZSupported();
    return reverseZ;
}

void setCameraClearDepth(osg::Camera* camera)
{
    camera->setClearDepth(getReverseZ() ? 0.0 : 1.0);
}

osg::ref_ptr<osg::Depth> createDepth()
{
    return new osg::Depth(getReverseZ() ? osg::Depth::GEQUAL : osg::Depth::LEQUAL);
}

osg::Matrix getReversedZProjectionMatrixAsPerspectiveInf(double fov, double aspect, double near)
{
    double A = 1.0/std::tan(osg::DegreesToRadians(fov)/2.0);
    return osg::Matrix(
        A/aspect,   0,      0,      0,
        0,          A,      0,      0,
        0,          0,      0,      -1,
        0,          0,      near,   0
    );
}

osg::Matrix getReversedZProjectionMatrixAsPerspective(double fov, double aspect, double near, double far)
{
    double A = 1.0/std::tan(osg::DegreesToRadians(fov)/2.0);
    return osg::Matrix(
        A/aspect,   0,      0,                          0,
        0,          A,      0,                          0,
        0,          0,      near/(far-near),            -1,
        0,          0,      (far*near)/(far - near),    0
    );
}

osg::Matrix getReversedZProjectionMatrixAsOrtho(double left, double right, double bottom, double top, double near, double far)
{
    return osg::Matrix(
        2/(right-left),             0,                          0,                  0,
        0,                          2/(top-bottom),             0,                  0,
        0,                          0,                          1/(far-near),       0,
        (right+left)/(left-right),  (top+bottom)/(bottom-top),  far/(far-near),     1
    );
}

bool isFloatingPointDepthFormat(GLenum format)
{
    constexpr std::array<GLenum, 4> formats = {
        GL_DEPTH_COMPONENT32F,
        GL_DEPTH_COMPONENT32F_NV,
        GL_DEPTH32F_STENCIL8,
        GL_DEPTH32F_STENCIL8_NV,
    };

    return std::find(formats.cbegin(), formats.cend(), format) != formats.cend();
}

}
