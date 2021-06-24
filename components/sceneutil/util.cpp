#include "util.hpp"

#include <algorithm>
#include <sstream>
#include <iomanip>

#include <osg/Node>
#include <osg/NodeVisitor>
#include <osg/TexGen>
#include <osg/TexEnvCombine>
#include <osg/Version>

#include <components/resource/imagemanager.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/settings/settings.hpp>

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

GlowUpdater::GlowUpdater(int texUnit, const osg::Vec4f& color, const std::vector<osg::ref_ptr<osg::Texture2D> >& textures,
    osg::Node* node, float duration, Resource::ResourceSystem* resourcesystem)
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

void GlowUpdater::setDefaults(osg::StateSet *stateset)
{
    if (mDone)
        removeTexture(stateset);
    else
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

void GlowUpdater::apply(osg::StateSet *stateset, osg::NodeVisitor *nv)
{
    if (mColorChanged){
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
    int index = (int)(time*16) % mTextures.size();
    stateset->setTextureAttribute(mTexUnit, mTextures[index], osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);

    if ((mDuration >= 0) && (time - mStartingTime > mDuration)) // If this is a temporary glow and it has finished its duration
    {
        if (mOriginalDuration >= 0) // if this glowupdater was a temporary glow since its creation
        {
            removeTexture(stateset);
            this->reset();
            mDone = true;
            // normally done in StateSetUpdater::operator(), but needs doing here so the shader visitor sees the right StateSet
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
    std::vector<osg::ref_ptr<osg::Texture2D> > textures;
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
        resourceSystem->getSceneManager()->applyFilterSettings(tex);
        textures.push_back(tex);
    }

    FindLowestUnusedTexUnitVisitor findLowestUnusedTexUnitVisitor;
    node->accept(findLowestUnusedTexUnitVisitor);
    int texUnit = findLowestUnusedTexUnitVisitor.mLowestUnusedTexUnit;

    osg::ref_ptr<GlowUpdater> glowUpdater = new GlowUpdater(texUnit, glowColor, textures, node, glowDuration, resourceSystem);
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
    writableStateSet->addUniform(new osg::Uniform("envMapColor", glowColor));
    resourceSystem->getSceneManager()->recreateShaders(node);

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

}
