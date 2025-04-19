#include "shadervisitor.hpp"

#include <set>
#include <unordered_map>
#include <unordered_set>

#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/ColorMaski>
#include <osg/GLExtensions>
#include <osg/Geometry>
#include <osg/Material>
#include <osg/Multisample>
#include <osg/Texture>
#include <osg/ValueObject>

#include <osgParticle/ParticleSystem>

#include <osgUtil/TangentSpaceGenerator>

#include <components/debug/debuglog.hpp>
#include <components/misc/osguservalues.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/resource/imagemanager.hpp>
#include <components/sceneutil/glextensions.hpp>
#include <components/sceneutil/morphgeometry.hpp>
#include <components/sceneutil/riggeometry.hpp>
#include <components/sceneutil/riggeometryosgaextension.hpp>
#include <components/sceneutil/texturetype.hpp>
#include <components/sceneutil/util.hpp>
#include <components/settings/settings.hpp>
#include <components/stereo/stereomanager.hpp>
#include <components/vfs/manager.hpp>

#include "removedalphafunc.hpp"
#include "shadermanager.hpp"

namespace Shader
{
    /**
     * Miniature version of osg::StateSet used to track state added by the shader visitor which should be ignored when
     * it's applied a second time, and removed when shaders are removed.
     * Actual StateAttributes aren't kept as they're recoverable from the StateSet this is attached to - we just want
     * the TypeMemberPair as that uniquely identifies which of those StateAttributes it was we're tracking.
     * Not all StateSet features have been added yet - we implement an equivalently-named method to each of the StateSet
     * methods called in createProgram, and implement new ones as they're needed.
     * When expanding tracking to cover new things, ensure they're accounted for in ensureFFP.
     */
    class AddedState : public osg::Object
    {
    public:
        AddedState() = default;
        AddedState(const AddedState& rhs, const osg::CopyOp& copyOp)
            : osg::Object(rhs, copyOp)
            , mUniforms(rhs.mUniforms)
            , mModes(rhs.mModes)
            , mAttributes(rhs.mAttributes)
            , mTextureModes(rhs.mTextureModes)
        {
        }

        void addUniform(const std::string& name) { mUniforms.emplace(name); }
        void setMode(osg::StateAttribute::GLMode mode) { mModes.emplace(mode); }
        void setAttribute(osg::StateAttribute::TypeMemberPair typeMemberPair) { mAttributes.emplace(typeMemberPair); }

        void setAttribute(const osg::StateAttribute* attribute) { mAttributes.emplace(attribute->getTypeMemberPair()); }
        template <typename T>
        void setAttribute(osg::ref_ptr<T> attribute)
        {
            setAttribute(attribute.get());
        }

        void setAttributeAndModes(const osg::StateAttribute* attribute)
        {
            setAttribute(attribute);
            InterrogateModesHelper helper(this);
            attribute->getModeUsage(helper);
        }
        template <typename T>
        void setAttributeAndModes(osg::ref_ptr<T> attribute)
        {
            setAttributeAndModes(attribute.get());
        }

        void setTextureMode(unsigned int unit, osg::StateAttribute::GLMode mode) { mTextureModes[unit].emplace(mode); }
        void setTextureAttribute(int unit, osg::StateAttribute::TypeMemberPair typeMemberPair)
        {
            mTextureAttributes[unit].emplace(typeMemberPair);
        }

        void setTextureAttribute(unsigned int unit, const osg::StateAttribute* attribute)
        {
            mTextureAttributes[unit].emplace(attribute->getTypeMemberPair());
        }
        template <typename T>
        void setTextureAttribute(unsigned int unit, osg::ref_ptr<T> attribute)
        {
            setTextureAttribute(unit, attribute.get());
        }

        void setTextureAttributeAndModes(unsigned int unit, const osg::StateAttribute* attribute)
        {
            setTextureAttribute(unit, attribute);
            InterrogateModesHelper helper(this, unit);
            attribute->getModeUsage(helper);
        }
        template <typename T>
        void setTextureAttributeAndModes(unsigned int unit, osg::ref_ptr<T> attribute)
        {
            setTextureAttributeAndModes(unit, attribute.get());
        }

        bool hasUniform(const std::string& name) { return mUniforms.count(name); }
        bool hasMode(osg::StateAttribute::GLMode mode) { return mModes.count(mode); }
        bool hasAttribute(const osg::StateAttribute::TypeMemberPair& typeMemberPair)
        {
            return mAttributes.count(typeMemberPair);
        }
        bool hasAttribute(osg::StateAttribute::Type type, unsigned int member)
        {
            return hasAttribute(osg::StateAttribute::TypeMemberPair(type, member));
        }
        bool hasTextureMode(int unit, osg::StateAttribute::GLMode mode)
        {
            auto it = mTextureModes.find(unit);
            if (it == mTextureModes.cend())
                return false;

            return it->second.count(mode);
        }

        const std::set<osg::StateAttribute::TypeMemberPair>& getAttributes() { return mAttributes; }
        const std::unordered_map<unsigned int, std::set<osg::StateAttribute::TypeMemberPair>>& getTextureAttributes()
        {
            return mTextureAttributes;
        }

        bool empty()
        {
            return mUniforms.empty() && mModes.empty() && mAttributes.empty() && mTextureModes.empty()
                && mTextureAttributes.empty();
        }

        META_Object(Shader, AddedState)

    private:
        class InterrogateModesHelper : public osg::StateAttribute::ModeUsage
        {
        public:
            InterrogateModesHelper(AddedState* tracker, unsigned int textureUnit = 0)
                : mTracker(tracker)
                , mTextureUnit(textureUnit)
            {
            }
            void usesMode(osg::StateAttribute::GLMode mode) override { mTracker->setMode(mode); }
            void usesTextureMode(osg::StateAttribute::GLMode mode) override
            {
                mTracker->setTextureMode(mTextureUnit, mode);
            }

        private:
            AddedState* mTracker;
            unsigned int mTextureUnit;
        };

        using ModeSet = std::unordered_set<osg::StateAttribute::GLMode>;
        using AttributeSet = std::set<osg::StateAttribute::TypeMemberPair>;

        std::unordered_set<std::string> mUniforms;
        ModeSet mModes;
        AttributeSet mAttributes;
        std::unordered_map<unsigned int, ModeSet> mTextureModes;
        std::unordered_map<unsigned int, AttributeSet> mTextureAttributes;
    };

    ShaderVisitor::ShaderRequirements::ShaderRequirements()
        : mShaderRequired(false)
        , mColorMode(0)
        , mMaterialOverridden(false)
        , mAlphaTestOverridden(false)
        , mAlphaBlendOverridden(false)
        , mAlphaFunc(GL_ALWAYS)
        , mAlphaRef(1.0)
        , mAlphaBlend(false)
        , mBlendFuncOverridden(false)
        , mAdditiveBlending(false)
        , mDiffuseHeight(false)
        , mNormalHeight(false)
        , mReconstructNormalZ(false)
        , mTexStageRequiringTangents(-1)
        , mSoftParticles(false)
        , mNode(nullptr)
    {
    }

    ShaderVisitor::ShaderVisitor(
        ShaderManager& shaderManager, Resource::ImageManager& imageManager, const std::string& defaultShaderPrefix)
        : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        , mForceShaders(false)
        , mAllowedToModifyStateSets(true)
        , mAutoUseNormalMaps(false)
        , mAutoUseSpecularMaps(false)
        , mApplyLightingToEnvMaps(false)
        , mConvertAlphaTestToAlphaToCoverage(false)
        , mAdjustCoverageForAlphaTest(false)
        , mSupportsNormalsRT(false)
        , mShaderManager(shaderManager)
        , mImageManager(imageManager)
        , mDefaultShaderPrefix(defaultShaderPrefix)
    {
    }

    void ShaderVisitor::setForceShaders(bool force)
    {
        mForceShaders = force;
    }

    void ShaderVisitor::apply(osg::Node& node)
    {
        bool needPop = false;
        if (node.getStateSet() || mRequirements.empty())
        {
            needPop = true;
            pushRequirements(node);
            if (node.getStateSet())
                applyStateSet(node.getStateSet(), node);
        }
        traverse(node);
        if (needPop)
            popRequirements();
    }

    osg::StateSet* getWritableStateSet(osg::Node& node)
    {
        if (!node.getStateSet())
            return node.getOrCreateStateSet();

        osg::ref_ptr<osg::StateSet> newStateSet = new osg::StateSet(*node.getStateSet(), osg::CopyOp::SHALLOW_COPY);
        node.setStateSet(newStateSet);
        return newStateSet.get();
    }

    osg::UserDataContainer* getWritableUserDataContainer(osg::Object& object)
    {
        if (!object.getUserDataContainer())
            return object.getOrCreateUserDataContainer();

        osg::ref_ptr<osg::UserDataContainer> newUserData
            = static_cast<osg::UserDataContainer*>(object.getUserDataContainer()->clone(osg::CopyOp::SHALLOW_COPY));
        object.setUserDataContainer(newUserData);
        return newUserData.get();
    }

    osg::StateSet* getRemovedState(osg::StateSet& stateSet)
    {
        if (!stateSet.getUserDataContainer())
            return nullptr;

        return static_cast<osg::StateSet*>(stateSet.getUserDataContainer()->getUserObject("removedState"));
    }

    void updateRemovedState(osg::UserDataContainer& userData, osg::StateSet* removedState)
    {
        unsigned int index = userData.getUserObjectIndex("removedState");
        if (index < userData.getNumUserObjects())
            userData.setUserObject(index, removedState);
        else
            userData.addUserObject(removedState);
        removedState->setName("removedState");
    }

    AddedState* getAddedState(osg::StateSet& stateSet)
    {
        if (!stateSet.getUserDataContainer())
            return nullptr;

        return static_cast<AddedState*>(stateSet.getUserDataContainer()->getUserObject("addedState"));
    }

    void updateAddedState(osg::UserDataContainer& userData, AddedState* addedState)
    {
        unsigned int index = userData.getUserObjectIndex("addedState");
        if (index < userData.getNumUserObjects())
            userData.setUserObject(index, addedState);
        else
            userData.addUserObject(addedState);
        addedState->setName("addedState");
    }

    const char* defaultTextures[] = { "diffuseMap", "normalMap", "normalHeightMap", "emissiveMap", "darkMap",
        "detailMap", "envMap", "specularMap", "decalMap", "bumpMap", "glossMap" };
    bool isTextureNameRecognized(std::string_view name)
    {
        return std::find(std::begin(defaultTextures), std::end(defaultTextures), name) != std::end(defaultTextures);
    }

    void ShaderVisitor::applyStateSet(osg::ref_ptr<osg::StateSet> stateset, osg::Node& node)
    {
        osg::StateSet* writableStateSet = nullptr;
        if (mAllowedToModifyStateSets)
            writableStateSet = node.getStateSet();
        const osg::StateSet::TextureAttributeList& texAttributes = stateset->getTextureAttributeList();
        bool shaderRequired = false;
        if (node.getUserValue("shaderRequired", shaderRequired) && shaderRequired)
            mRequirements.back().mShaderRequired = true;

        bool softEffect = false;
        if (node.getUserValue(Misc::OsgUserValues::sXSoftEffect, softEffect) && softEffect)
            mRequirements.back().mSoftParticles = true;

        // Make sure to disregard any state that came from a previous call to createProgram
        osg::ref_ptr<AddedState> addedState = getAddedState(*stateset);

        if (!texAttributes.empty())
        {
            const osg::Texture* diffuseMap = nullptr;
            const osg::Texture* normalMap = nullptr;
            const osg::Texture* specularMap = nullptr;
            const osg::Texture* bumpMap = nullptr;
            for (unsigned int unit = 0; unit < texAttributes.size(); ++unit)
            {
                const osg::StateAttribute* attr = stateset->getTextureAttribute(unit, osg::StateAttribute::TEXTURE);
                if (attr)
                {
                    // If textures ever get removed in createProgram, expand this to check we're operating on main
                    // texture attribute list rather than the removed list
                    if (addedState && addedState->hasTextureMode(unit, GL_TEXTURE_2D))
                        continue;

                    const osg::Texture* texture = attr->asTexture();
                    if (texture)
                    {
                        std::string texName = SceneUtil::getTextureType(*stateset, *texture, unit);
                        if ((texName.empty() || !isTextureNameRecognized(texName)) && unit == 0)
                            texName = "diffuseMap";

                        if (texName == "normalHeightMap")
                        {
                            mRequirements.back().mNormalHeight = true;
                            texName = "normalMap";
                        }

                        if (!texName.empty())
                        {
                            mRequirements.back().mTextures[unit] = texName;
                            if (texName == "normalMap")
                            {
                                mRequirements.back().mTexStageRequiringTangents = unit;
                                mRequirements.back().mShaderRequired = true;
                                if (!writableStateSet)
                                    writableStateSet = getWritableStateSet(node);
                                // normal maps are by default off since the FFP can't render them, now that we'll use
                                // shaders switch to On
                                writableStateSet->setTextureMode(unit, GL_TEXTURE_2D, osg::StateAttribute::ON);
                                normalMap = texture;
                            }
                            else if (texName == "diffuseMap")
                            {
                                int applyMode;
                                // Oblivion parallax
                                if (node.getUserValue("applyMode", applyMode) && applyMode == 4)
                                {
                                    mRequirements.back().mShaderRequired = true;
                                    mRequirements.back().mDiffuseHeight = true;
                                    mRequirements.back().mTexStageRequiringTangents = unit;
                                }
                                diffuseMap = texture;
                            }
                            else if (texName == "specularMap")
                                specularMap = texture;
                            else if (texName == "bumpMap")
                            {
                                bumpMap = texture;
                                mRequirements.back().mShaderRequired = true;
                                if (!writableStateSet)
                                    writableStateSet = getWritableStateSet(node);
                                // Bump maps are off by default as well
                                writableStateSet->setTextureMode(unit, GL_TEXTURE_2D, osg::StateAttribute::ON);
                            }
                            else if (texName == "envMap" && mApplyLightingToEnvMaps)
                            {
                                mRequirements.back().mShaderRequired = true;
                            }
                            else if (texName == "glossMap")
                            {
                                mRequirements.back().mShaderRequired = true;
                                if (!writableStateSet)
                                    writableStateSet = getWritableStateSet(node);
                                // As well as gloss maps
                                writableStateSet->setTextureMode(unit, GL_TEXTURE_2D, osg::StateAttribute::ON);
                            }
                        }
                        else
                            Log(Debug::Error) << "ShaderVisitor encountered unknown texture " << texture;
                    }
                }
            }

            if (mAutoUseNormalMaps && diffuseMap != nullptr && normalMap == nullptr && diffuseMap->getImage(0))
            {
                std::string normalMapFileName = diffuseMap->getImage(0)->getFileName();

                osg::ref_ptr<osg::Image> image;
                bool normalHeight = false;
                std::string normalHeightMap = normalMapFileName;
                Misc::StringUtils::replaceLast(normalHeightMap, ".", mNormalHeightMapPattern + ".");
                const VFS::Path::Normalized normalHeightMapPath(normalHeightMap);
                if (mImageManager.getVFS()->exists(normalHeightMapPath))
                {
                    image = mImageManager.getImage(normalHeightMapPath);
                    normalHeight = true;
                }
                else
                {
                    Misc::StringUtils::replaceLast(normalMapFileName, ".", mNormalMapPattern + ".");
                    const VFS::Path::Normalized normalMapPath(normalMapFileName);
                    if (mImageManager.getVFS()->exists(normalMapPath))
                    {
                        image = mImageManager.getImage(normalMapPath);
                    }
                }
                // Avoid using the auto-detected normal map if it's already being used as a bump map.
                // It's probably not an actual normal map.
                bool hasNamesakeBumpMap = image && bumpMap && bumpMap->getImage(0)
                    && image->getFileName() == bumpMap->getImage(0)->getFileName();

                if (!hasNamesakeBumpMap && image)
                {
                    osg::ref_ptr<osg::Texture2D> normalMapTex(new osg::Texture2D(image));
                    normalMapTex->setTextureSize(image->s(), image->t());
                    normalMapTex->setWrap(osg::Texture::WRAP_S, diffuseMap->getWrap(osg::Texture::WRAP_S));
                    normalMapTex->setWrap(osg::Texture::WRAP_T, diffuseMap->getWrap(osg::Texture::WRAP_T));
                    normalMapTex->setFilter(osg::Texture::MIN_FILTER, diffuseMap->getFilter(osg::Texture::MIN_FILTER));
                    normalMapTex->setFilter(osg::Texture::MAG_FILTER, diffuseMap->getFilter(osg::Texture::MAG_FILTER));
                    normalMapTex->setMaxAnisotropy(diffuseMap->getMaxAnisotropy());
                    normalMap = normalMapTex;

                    int unit = texAttributes.size();
                    if (!writableStateSet)
                        writableStateSet = getWritableStateSet(node);
                    writableStateSet->setTextureAttributeAndModes(unit, normalMapTex, osg::StateAttribute::ON);
                    writableStateSet->setTextureAttributeAndModes(unit,
                        new SceneUtil::TextureType(normalHeight ? "normalHeightMap" : "normalMap"),
                        osg::StateAttribute::ON);
                    mRequirements.back().mTextures[unit] = "normalMap";
                    mRequirements.back().mTexStageRequiringTangents = unit;
                    mRequirements.back().mShaderRequired = true;
                    mRequirements.back().mNormalHeight = normalHeight;
                }
            }

            if (normalMap != nullptr && normalMap->getImage(0))
            {
                // Special handling for red-green normal maps (e.g. BC5 or R8G8)
                switch (SceneUtil::computeUnsizedPixelFormat(normalMap->getImage(0)->getPixelFormat()))
                {
                    case GL_RG:
                    case GL_RG_INTEGER:
                    {
                        mRequirements.back().mReconstructNormalZ = true;
                        mRequirements.back().mNormalHeight = false;
                    }
                }
            }

            if (mAutoUseSpecularMaps && diffuseMap != nullptr && specularMap == nullptr && diffuseMap->getImage(0))
            {
                std::string specularMapFileName = diffuseMap->getImage(0)->getFileName();
                Misc::StringUtils::replaceLast(specularMapFileName, ".", mSpecularMapPattern + ".");
                const VFS::Path::Normalized specularMapPath(specularMapFileName);
                if (mImageManager.getVFS()->exists(specularMapPath))
                {
                    osg::ref_ptr<osg::Image> image(mImageManager.getImage(specularMapPath));
                    osg::ref_ptr<osg::Texture2D> specularMapTex(new osg::Texture2D(image));
                    specularMapTex->setTextureSize(image->s(), image->t());
                    specularMapTex->setWrap(osg::Texture::WRAP_S, diffuseMap->getWrap(osg::Texture::WRAP_S));
                    specularMapTex->setWrap(osg::Texture::WRAP_T, diffuseMap->getWrap(osg::Texture::WRAP_T));
                    specularMapTex->setFilter(
                        osg::Texture::MIN_FILTER, diffuseMap->getFilter(osg::Texture::MIN_FILTER));
                    specularMapTex->setFilter(
                        osg::Texture::MAG_FILTER, diffuseMap->getFilter(osg::Texture::MAG_FILTER));
                    specularMapTex->setMaxAnisotropy(diffuseMap->getMaxAnisotropy());

                    int unit = texAttributes.size();
                    if (!writableStateSet)
                        writableStateSet = getWritableStateSet(node);
                    writableStateSet->setTextureAttributeAndModes(unit, specularMapTex, osg::StateAttribute::ON);
                    writableStateSet->setTextureAttributeAndModes(
                        unit, new SceneUtil::TextureType("specularMap"), osg::StateAttribute::ON);
                    mRequirements.back().mTextures[unit] = "specularMap";
                    mRequirements.back().mShaderRequired = true;
                }
            }
        }

        const osg::StateSet::AttributeList& attributes = stateset->getAttributeList();
        osg::StateSet::AttributeList removedAttributes;
        if (osg::ref_ptr<osg::StateSet> removedState = getRemovedState(*stateset))
            removedAttributes = removedState->getAttributeList();

        for (const auto* attributeMap :
            std::initializer_list<const osg::StateSet::AttributeList*>{ &attributes, &removedAttributes })
        {
            for (osg::StateSet::AttributeList::const_iterator it = attributeMap->begin(); it != attributeMap->end();
                 ++it)
            {
                if (addedState && attributeMap != &removedAttributes && addedState->hasAttribute(it->first))
                    continue;
                if (it->first.first == osg::StateAttribute::MATERIAL)
                {
                    // This should probably be moved out of ShaderRequirements and be applied directly now it's a
                    // uniform instead of a define
                    if (!mRequirements.back().mMaterialOverridden || it->second.second & osg::StateAttribute::PROTECTED)
                    {
                        if (it->second.second & osg::StateAttribute::OVERRIDE)
                            mRequirements.back().mMaterialOverridden = true;

                        const osg::Material* mat = static_cast<const osg::Material*>(it->second.first.get());

                        int colorMode;
                        switch (mat->getColorMode())
                        {
                            case osg::Material::OFF:
                                colorMode = 0;
                                break;
                            case osg::Material::EMISSION:
                                colorMode = 1;
                                break;
                            default:
                            case osg::Material::AMBIENT_AND_DIFFUSE:
                                colorMode = 2;
                                break;
                            case osg::Material::AMBIENT:
                                colorMode = 3;
                                break;
                            case osg::Material::DIFFUSE:
                                colorMode = 4;
                                break;
                            case osg::Material::SPECULAR:
                                colorMode = 5;
                                break;
                        }

                        mRequirements.back().mColorMode = colorMode;
                    }
                }
                else if (it->first.first == osg::StateAttribute::ALPHAFUNC)
                {
                    if (!mRequirements.back().mAlphaTestOverridden
                        || it->second.second & osg::StateAttribute::PROTECTED)
                    {
                        if (it->second.second & osg::StateAttribute::OVERRIDE)
                            mRequirements.back().mAlphaTestOverridden = true;

                        const osg::AlphaFunc* alpha = static_cast<const osg::AlphaFunc*>(it->second.first.get());
                        mRequirements.back().mAlphaFunc = alpha->getFunction();
                        mRequirements.back().mAlphaRef = alpha->getReferenceValue();
                    }
                }
                else if (it->first.first == osg::StateAttribute::BLENDFUNC)
                {
                    if (!mRequirements.back().mBlendFuncOverridden
                        || it->second.second & osg::StateAttribute::PROTECTED)
                    {
                        if (it->second.second & osg::StateAttribute::OVERRIDE)
                            mRequirements.back().mBlendFuncOverridden = true;

                        const osg::BlendFunc* blend = static_cast<const osg::BlendFunc*>(it->second.first.get());
                        mRequirements.back().mAdditiveBlending = blend->getSource() == osg::BlendFunc::SRC_ALPHA
                            && blend->getDestination() == osg::BlendFunc::ONE;
                    }
                }
            }
        }

        unsigned int alphaBlend = stateset->getMode(GL_BLEND);
        if (alphaBlend != osg::StateAttribute::INHERIT
            && (!mRequirements.back().mAlphaBlendOverridden || alphaBlend & osg::StateAttribute::PROTECTED))
        {
            if (alphaBlend & osg::StateAttribute::OVERRIDE)
                mRequirements.back().mAlphaBlendOverridden = true;

            mRequirements.back().mAlphaBlend = alphaBlend & osg::StateAttribute::ON;
        }
    }

    void ShaderVisitor::pushRequirements(osg::Node& node)
    {
        if (mRequirements.empty())
            mRequirements.emplace_back();
        else
            mRequirements.push_back(mRequirements.back());
        mRequirements.back().mNode = &node;
    }

    void ShaderVisitor::popRequirements()
    {
        mRequirements.pop_back();
    }

    void ShaderVisitor::createProgram(const ShaderRequirements& reqs)
    {
        if (!reqs.mShaderRequired && !mForceShaders)
        {
            ensureFFP(*reqs.mNode);
            return;
        }

        /**
         * The shader visitor is supposed to be idempotent and undoable.
         * That means we need to back up state we've removed (so it can be restored and/or considered by further
         * applications of the visitor) and track which state we added (so it can be removed and/or ignored by further
         * applications of the visitor).
         * Before editing writableStateSet in a way that explicitly removes state or might overwrite existing state, it
         * should be copied to removedState, another StateSet, unless it's there already or was added by a previous
         * application of the visitor (is in previousAddedState).
         * If it's a new class of state that's not already handled by ReinstateRemovedStateVisitor::apply, make sure to
         * add handling there.
         * Similarly, any time new state is added to writableStateSet, the equivalent method should be called on
         * addedState.
         * If that method doesn't exist yet, implement it - we don't use a full StateSet as we only need to check
         * existence, not equality, and don't need to actually get the value as we can get it from writableStateSet
         * instead.
         */
        osg::Node& node = *reqs.mNode;
        osg::StateSet* writableStateSet = nullptr;
        if (mAllowedToModifyStateSets)
            writableStateSet = node.getOrCreateStateSet();
        else
            writableStateSet = getWritableStateSet(node);
        osg::ref_ptr<AddedState> addedState = new AddedState;
        osg::ref_ptr<AddedState> previousAddedState = getAddedState(*writableStateSet);
        if (!previousAddedState)
            previousAddedState = new AddedState;

        ShaderManager::DefineMap defineMap;
        for (unsigned int i = 0; i < sizeof(defaultTextures) / sizeof(defaultTextures[0]); ++i)
        {
            defineMap[defaultTextures[i]] = "0";
            defineMap[std::string(defaultTextures[i]) + std::string("UV")] = "0";
        }
        for (std::map<int, std::string>::const_iterator texIt = reqs.mTextures.begin(); texIt != reqs.mTextures.end();
             ++texIt)
        {
            defineMap[texIt->second] = "1";
            defineMap[texIt->second + std::string("UV")] = std::to_string(texIt->first);
        }

        if (defineMap["diffuseMap"] == "0")
        {
            writableStateSet->addUniform(new osg::Uniform("useDiffuseMapForShadowAlpha", false));
            addedState->addUniform("useDiffuseMapForShadowAlpha");
        }

        defineMap["diffuseParallax"] = reqs.mDiffuseHeight ? "1" : "0";
        defineMap["parallax"] = reqs.mNormalHeight ? "1" : "0";
        defineMap["reconstructNormalZ"] = reqs.mReconstructNormalZ ? "1" : "0";

        writableStateSet->addUniform(new osg::Uniform("colorMode", reqs.mColorMode));
        addedState->addUniform("colorMode");

        defineMap["alphaFunc"] = std::to_string(reqs.mAlphaFunc);

        defineMap["additiveBlending"] = reqs.mAdditiveBlending ? "1" : "0";

        osg::ref_ptr<osg::StateSet> removedState;
        if ((removedState = getRemovedState(*writableStateSet)) && !mAllowedToModifyStateSets)
            removedState = new osg::StateSet(*removedState, osg::CopyOp::SHALLOW_COPY);
        if (!removedState)
            removedState = new osg::StateSet();

        defineMap["alphaToCoverage"] = "0";
        defineMap["adjustCoverage"] = "0";
        if (reqs.mAlphaFunc != osg::AlphaFunc::ALWAYS)
        {
            writableStateSet->addUniform(new osg::Uniform("alphaRef", reqs.mAlphaRef));
            addedState->addUniform("alphaRef");

            if (!removedState->getAttributePair(osg::StateAttribute::ALPHAFUNC))
            {
                const auto* alphaFunc = writableStateSet->getAttributePair(osg::StateAttribute::ALPHAFUNC);
                if (alphaFunc && !previousAddedState->hasAttribute(osg::StateAttribute::ALPHAFUNC, 0))
                    removedState->setAttribute(alphaFunc->first, alphaFunc->second);
            }
            // This prevents redundant glAlphaFunc calls while letting the shadows bin still see the test
            writableStateSet->setAttribute(RemovedAlphaFunc::getInstance(reqs.mAlphaFunc),
                osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            addedState->setAttribute(RemovedAlphaFunc::getInstance(reqs.mAlphaFunc));

            // Blending won't work with A2C as we use the alpha channel for coverage. gl_SampleCoverage from
            // ARB_sample_shading would save the day, but requires GLSL 130
            if (mConvertAlphaTestToAlphaToCoverage && !reqs.mAlphaBlend)
            {
                writableStateSet->setMode(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB, osg::StateAttribute::ON);
                addedState->setMode(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB);
                defineMap["alphaToCoverage"] = "1";
            }

            // Adjusting coverage isn't safe with blending on as blending requires the alpha to be intact.
            // Maybe we could also somehow (e.g. userdata) detect when the diffuse map has coverage-preserving mip maps
            // in the future
            if (mAdjustCoverageForAlphaTest && !reqs.mAlphaBlend)
                defineMap["adjustCoverage"] = "1";

            // Preventing alpha tested stuff shrinking as lower mip levels are used requires knowing the texture size
            if (SceneUtil::getGLExtensions().isGpuShader4Supported)
                defineMap["useGPUShader4"] = "1";
            // We could fall back to a texture size uniform if EXT_gpu_shader4 is missing
        }

        bool simpleLighting = false;
        node.getUserValue("simpleLighting", simpleLighting);
        if (simpleLighting)
            defineMap["endLight"] = "0";

        if (simpleLighting || dynamic_cast<osgParticle::ParticleSystem*>(&node))
            defineMap["forcePPL"] = "0";

        bool particleOcclusion = false;
        node.getUserValue("particleOcclusion", particleOcclusion);
        defineMap["particleOcclusion"] = particleOcclusion && mWeatherParticleOcclusion ? "1" : "0";

        if (reqs.mAlphaBlend && mSupportsNormalsRT)
        {
            if (reqs.mSoftParticles)
                defineMap["disableNormals"] = "1";
            auto colorMask = new osg::ColorMaski(1, false, false, false, false);
            writableStateSet->setAttribute(colorMask);
            addedState->setAttribute(colorMask);
        }

        if (reqs.mSoftParticles)
        {
            const int unitSoftEffect
                = mShaderManager.reserveGlobalTextureUnits(Shader::ShaderManager::Slot::OpaqueDepthTexture);
            writableStateSet->addUniform(new osg::Uniform("opaqueDepthTex", unitSoftEffect));
            addedState->addUniform("opaqueDepthTex");
        }

        if (writableStateSet->getMode(GL_ALPHA_TEST) != osg::StateAttribute::INHERIT
            && !previousAddedState->hasMode(GL_ALPHA_TEST))
            removedState->setMode(GL_ALPHA_TEST, writableStateSet->getMode(GL_ALPHA_TEST));
        // This disables the deprecated fixed-function alpha test
        writableStateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED);
        addedState->setMode(GL_ALPHA_TEST);

        if (!removedState->getModeList().empty() || !removedState->getAttributeList().empty())
        {
            // user data is normally shallow copied so shared with the original stateset
            osg::ref_ptr<osg::UserDataContainer> writableUserData;
            if (mAllowedToModifyStateSets)
                writableUserData = writableStateSet->getOrCreateUserDataContainer();
            else
                writableUserData = getWritableUserDataContainer(*writableStateSet);

            updateRemovedState(*writableUserData, removedState);
        }

        defineMap["softParticles"] = reqs.mSoftParticles ? "1" : "0";

        Stereo::shaderStereoDefines(defineMap);

        std::string shaderPrefix;
        if (!node.getUserValue("shaderPrefix", shaderPrefix))
            shaderPrefix = mDefaultShaderPrefix;

        auto program = mShaderManager.getProgram(shaderPrefix, defineMap, mProgramTemplate);
        writableStateSet->setAttributeAndModes(program, osg::StateAttribute::ON);
        addedState->setAttributeAndModes(std::move(program));

        for (const auto& [unit, name] : reqs.mTextures)
        {
            writableStateSet->addUniform(new osg::Uniform(name.c_str(), unit), osg::StateAttribute::ON);
            addedState->addUniform(name);
        }

        if (!addedState->empty())
        {
            // user data is normally shallow copied so shared with the original stateset
            osg::ref_ptr<osg::UserDataContainer> writableUserData;
            if (mAllowedToModifyStateSets)
                writableUserData = writableStateSet->getOrCreateUserDataContainer();
            else
                writableUserData = getWritableUserDataContainer(*writableStateSet);

            updateAddedState(*writableUserData, addedState);
        }
    }

    void ShaderVisitor::ensureFFP(osg::Node& node)
    {
        if (!node.getStateSet() || !node.getStateSet()->getAttribute(osg::StateAttribute::PROGRAM))
            return;
        osg::StateSet* writableStateSet = nullptr;
        if (mAllowedToModifyStateSets)
            writableStateSet = node.getStateSet();
        else
            writableStateSet = getWritableStateSet(node);

        /**
         * We might have been using shaders temporarily with the node (e.g. if a GlowUpdater applied a temporary
         * environment map for a temporary enchantment).
         * We therefore need to remove any state doing so added, and restore any that it removed.
         * This is kept track of in createProgram in the StateSet's userdata.
         * If new classes of state get added, handling it here is required - not all StateSet features are implemented
         * in AddedState yet as so far they've not been necessary.
         * Removed state requires no particular special handling as it's dealt with by merging StateSets.
         * We don't need to worry about state in writableStateSet having the OVERRIDE flag as if it's in both, it's also
         * in addedState, and gets removed first.
         */

        // user data is normally shallow copied so shared with the original stateset - we'll need to copy before edits
        osg::ref_ptr<osg::UserDataContainer> writableUserData;

        if (osg::ref_ptr<AddedState> addedState = getAddedState(*writableStateSet))
        {
            if (mAllowedToModifyStateSets)
                writableUserData = writableStateSet->getUserDataContainer();
            else
                writableUserData = getWritableUserDataContainer(*writableStateSet);

            unsigned int index = writableUserData->getUserObjectIndex("addedState");
            writableUserData->removeUserObject(index);

            // O(n log n) to use StateSet::removeX, but this is O(n)
            for (auto itr = writableStateSet->getUniformList().begin();
                 itr != writableStateSet->getUniformList().end();)
            {
                if (addedState->hasUniform(itr->first))
                    writableStateSet->getUniformList().erase(itr++);
                else
                    ++itr;
            }

            for (auto itr = writableStateSet->getModeList().begin(); itr != writableStateSet->getModeList().end();)
            {
                if (addedState->hasMode(itr->first))
                    writableStateSet->getModeList().erase(itr++);
                else
                    ++itr;
            }

            // StateAttributes track the StateSets they're attached to
            // We don't have access to the function to do that, and can't call removeAttribute with an iterator
            for (const auto& [type, member] : addedState->getAttributes())
                writableStateSet->removeAttribute(type, member);

            for (unsigned int unit = 0; unit < writableStateSet->getTextureModeList().size(); ++unit)
            {
                for (auto itr = writableStateSet->getTextureModeList()[unit].begin();
                     itr != writableStateSet->getTextureModeList()[unit].end();)
                {
                    if (addedState->hasTextureMode(unit, itr->first))
                        writableStateSet->getTextureModeList()[unit].erase(itr++);
                    else
                        ++itr;
                }
            }

            for (const auto& [unit, attributeList] : addedState->getTextureAttributes())
            {
                for (const auto& [type, member] : attributeList)
                    writableStateSet->removeTextureAttribute(unit, type);
            }
        }

        if (osg::ref_ptr<osg::StateSet> removedState = getRemovedState(*writableStateSet))
        {
            if (!writableUserData)
            {
                if (mAllowedToModifyStateSets)
                    writableUserData = writableStateSet->getUserDataContainer();
                else
                    writableUserData = getWritableUserDataContainer(*writableStateSet);
            }

            unsigned int index = writableUserData->getUserObjectIndex("removedState");
            writableUserData->removeUserObject(index);

            writableStateSet->merge(*removedState);
        }
    }

    bool ShaderVisitor::adjustGeometry(osg::Geometry& sourceGeometry, const ShaderRequirements& reqs)
    {
        bool useShader = reqs.mShaderRequired || mForceShaders;
        bool generateTangents = reqs.mTexStageRequiringTangents != -1;
        bool changed = false;

        if (mAllowedToModifyStateSets && (useShader || generateTangents))
        {
            // make sure that all UV sets are there
            // it's not safe to assume there's one for slot zero, so try and use one from another slot if possible
            // if there are none at all, bail.
            // the TangentSpaceGenerator would bail, but getTangentArray would give an empty array, which is enough to
            // bypass null checks, but feeds the driver a bad pointer
            if (sourceGeometry.getTexCoordArray(0) == nullptr)
            {
                for (const auto& array : sourceGeometry.getTexCoordArrayList())
                {
                    if (array)
                    {
                        sourceGeometry.setTexCoordArray(0, array);
                        break;
                    }
                }
                if (sourceGeometry.getTexCoordArray(0) == nullptr)
                    return changed;
            }

            for (const auto& [unit, name] : reqs.mTextures)
            {
                if (sourceGeometry.getTexCoordArray(unit) == nullptr)
                {
                    sourceGeometry.setTexCoordArray(unit, sourceGeometry.getTexCoordArray(0));
                    changed = true;
                }
            }

            if (generateTangents)
            {
                osg::ref_ptr<osgUtil::TangentSpaceGenerator> generator(new osgUtil::TangentSpaceGenerator);
                generator->generate(&sourceGeometry, reqs.mTexStageRequiringTangents);

                sourceGeometry.setTexCoordArray(7, generator->getTangentArray(), osg::Array::BIND_PER_VERTEX);
                changed = true;
            }
        }
        return changed;
    }

    void ShaderVisitor::apply(osg::Geometry& geometry)
    {
        bool needPop = geometry.getStateSet() || mRequirements.empty();
        if (needPop)
            pushRequirements(geometry);

        if (geometry.getStateSet()) // TODO: check if stateset affects shader permutation before pushing it
            applyStateSet(geometry.getStateSet(), geometry);

        if (!mRequirements.empty())
        {
            const ShaderRequirements& reqs = mRequirements.back();

            adjustGeometry(geometry, reqs);

            createProgram(reqs);
        }
        else
            ensureFFP(geometry);

        if (needPop)
            popRequirements();
    }

    void ShaderVisitor::apply(osg::Drawable& drawable)
    {
        bool needPop = drawable.getStateSet() || mRequirements.empty();

        // We need to push and pop a requirements object because particle systems can have
        // different shader requirements to other drawables, so might need a different shader variant.
        if (!needPop && dynamic_cast<osgParticle::ParticleSystem*>(&drawable))
            needPop = true;

        if (needPop)
        {
            pushRequirements(drawable);

            if (drawable.getStateSet())
                applyStateSet(drawable.getStateSet(), drawable);
        }

        const ShaderRequirements& reqs = mRequirements.back();
        createProgram(reqs);

        if (auto rig = dynamic_cast<SceneUtil::RigGeometry*>(&drawable))
        {
            osg::ref_ptr<osg::Geometry> sourceGeometry = rig->getSourceGeometry();
            if (sourceGeometry && adjustGeometry(*sourceGeometry, reqs))
                rig->setSourceGeometry(std::move(sourceGeometry));
        }
        else if (auto morph = dynamic_cast<SceneUtil::MorphGeometry*>(&drawable))
        {
            osg::ref_ptr<osg::Geometry> sourceGeometry = morph->getSourceGeometry();
            if (sourceGeometry && adjustGeometry(*sourceGeometry, reqs))
                morph->setSourceGeometry(std::move(sourceGeometry));
        }
        else if (auto osgaRig = dynamic_cast<SceneUtil::RigGeometryHolder*>(&drawable))
        {
            osg::ref_ptr<SceneUtil::OsgaRigGeometry> sourceOsgaRigGeometry = osgaRig->getSourceRigGeometry();
            osg::ref_ptr<osg::Geometry> sourceGeometry = sourceOsgaRigGeometry->getSourceGeometry();
            if (sourceGeometry && adjustGeometry(*sourceGeometry, reqs))
            {
                sourceOsgaRigGeometry->setSourceGeometry(std::move(sourceGeometry));
                osgaRig->setSourceRigGeometry(std::move(sourceOsgaRigGeometry));
            }
        }

        if (needPop)
            popRequirements();
    }

    void ShaderVisitor::setAllowedToModifyStateSets(bool allowed)
    {
        mAllowedToModifyStateSets = allowed;
    }

    void ShaderVisitor::setAutoUseNormalMaps(bool use)
    {
        mAutoUseNormalMaps = use;
    }

    void ShaderVisitor::setNormalMapPattern(const std::string& pattern)
    {
        mNormalMapPattern = pattern;
    }

    void ShaderVisitor::setNormalHeightMapPattern(const std::string& pattern)
    {
        mNormalHeightMapPattern = pattern;
    }

    void ShaderVisitor::setAutoUseSpecularMaps(bool use)
    {
        mAutoUseSpecularMaps = use;
    }

    void ShaderVisitor::setSpecularMapPattern(const std::string& pattern)
    {
        mSpecularMapPattern = pattern;
    }

    void ShaderVisitor::setApplyLightingToEnvMaps(bool apply)
    {
        mApplyLightingToEnvMaps = apply;
    }

    void ShaderVisitor::setConvertAlphaTestToAlphaToCoverage(bool convert)
    {
        mConvertAlphaTestToAlphaToCoverage = convert;
    }

    void ShaderVisitor::setAdjustCoverageForAlphaTest(bool adjustCoverage)
    {
        mAdjustCoverageForAlphaTest = adjustCoverage;
    }

    ReinstateRemovedStateVisitor::ReinstateRemovedStateVisitor(bool allowedToModifyStateSets)
        : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        , mAllowedToModifyStateSets(allowedToModifyStateSets)
    {
    }

    void ReinstateRemovedStateVisitor::apply(osg::Node& node)
    {
        // TODO: this may eventually need to remove added state.
        // If so, we can migrate from explicitly copying removed state to just calling osg::StateSet::merge.
        // Not everything is transferred from removedState yet - implement more when createProgram starts marking more
        // as removed.
        if (node.getStateSet())
        {
            osg::ref_ptr<osg::StateSet> removedState = getRemovedState(*node.getStateSet());
            if (removedState)
            {
                osg::ref_ptr<osg::StateSet> writableStateSet;
                if (mAllowedToModifyStateSets)
                    writableStateSet = node.getStateSet();
                else
                    writableStateSet = getWritableStateSet(node);

                // user data is normally shallow copied so shared with the original stateset
                osg::ref_ptr<osg::UserDataContainer> writableUserData;
                if (mAllowedToModifyStateSets)
                    writableUserData = writableStateSet->getUserDataContainer();
                else
                    writableUserData = getWritableUserDataContainer(*writableStateSet);
                unsigned int index = writableUserData->getUserObjectIndex("removedState");
                writableUserData->removeUserObject(index);

                for (const auto& [mode, value] : removedState->getModeList())
                    writableStateSet->setMode(mode, value);

                for (const auto& attribute : removedState->getAttributeList())
                    writableStateSet->setAttribute(attribute.second.first, attribute.second.second);

                for (unsigned int unit = 0; unit < removedState->getTextureModeList().size(); ++unit)
                {
                    for (const auto& [mode, value] : removedState->getTextureModeList()[unit])
                        writableStateSet->setTextureMode(unit, mode, value);
                }
            }
        }

        traverse(node);
    }

}
