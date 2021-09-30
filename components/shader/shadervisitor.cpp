#include "shadervisitor.hpp"

#include <unordered_set>
#include <set>

#include <osg/AlphaFunc>
#include <osg/Geometry>
#include <osg/GLExtensions>
#include <osg/Material>
#include <osg/Multisample>
#include <osg/Texture>
#include <osg/ValueObject>

#include <osgUtil/TangentSpaceGenerator>

#include <components/debug/debuglog.hpp>
#include <components/misc/stringops.hpp>
#include <components/resource/imagemanager.hpp>
#include <components/vfs/manager.hpp>
#include <components/sceneutil/riggeometry.hpp>
#include <components/sceneutil/morphgeometry.hpp>

#include "removedalphafunc.hpp"
#include "shadermanager.hpp"

namespace Shader
{
    class AddedState : public osg::Object
    {
    public:
        AddedState() = default;
        AddedState(const AddedState& rhs, const osg::CopyOp& copyOp)
            : osg::Object(rhs, copyOp)
            , mUniforms(rhs.mUniforms)
            , mModes(rhs.mModes)
            , mAttributes(rhs.mAttributes)
        {
        }

        void addUniform(const std::string& name) { mUniforms.emplace(name); }
        void setMode(osg::StateAttribute::GLMode mode) { mModes.emplace(mode); }
        void setAttribute(osg::StateAttribute::TypeMemberPair typeMemberPair) { mAttributes.emplace(typeMemberPair); }

        void setAttribute(const osg::StateAttribute* attribute)
        {
            mAttributes.emplace(attribute->getTypeMemberPair());
        }
        template<typename T>
        void setAttribute(osg::ref_ptr<T> attribute) { setAttribute(attribute.get()); }

        void setAttributeAndModes(const osg::StateAttribute* attribute)
        {
            setAttribute(attribute);
            InterrogateModesHelper helper(this);
            attribute->getModeUsage(helper);
        }
        template<typename T>
        void setAttributeAndModes(osg::ref_ptr<T> attribute) { setAttributeAndModes(attribute.get()); }

        bool hasUniform(const std::string& name) { return mUniforms.count(name); }
        bool hasMode(osg::StateAttribute::GLMode mode) { return mModes.count(mode); }
        bool hasAttribute(const osg::StateAttribute::TypeMemberPair &typeMemberPair) { return mAttributes.count(typeMemberPair); }
        bool hasAttribute(osg::StateAttribute::Type type, unsigned int member) { return hasAttribute(osg::StateAttribute::TypeMemberPair(type, member)); }

        const std::set<osg::StateAttribute::TypeMemberPair>& getAttributes() { return mAttributes; }

        bool empty()
        {
            return mUniforms.empty() && mModes.empty() && mAttributes.empty();
        }

        META_Object(Shader, AddedState)

    private:
        class InterrogateModesHelper : public osg::StateAttribute::ModeUsage
        {
        public:
            InterrogateModesHelper(AddedState* tracker) : mTracker(tracker) {}
            void usesMode(osg::StateAttribute::GLMode mode) override { mTracker->setMode(mode); }
            void usesTextureMode(osg::StateAttribute::GLMode mode) override {}

        private:
            AddedState* mTracker;
        };

        std::unordered_set<std::string> mUniforms;
        std::unordered_set<osg::StateAttribute::GLMode> mModes;
        std::set<osg::StateAttribute::TypeMemberPair> mAttributes;
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
        , mNormalHeight(false)
        , mTexStageRequiringTangents(-1)
        , mNode(nullptr)
    {
    }

    ShaderVisitor::ShaderVisitor(ShaderManager& shaderManager, Resource::ImageManager& imageManager, const std::string &defaultShaderPrefix)
        : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        , mForceShaders(false)
        , mAllowedToModifyStateSets(true)
        , mAutoUseNormalMaps(false)
        , mAutoUseSpecularMaps(false)
        , mApplyLightingToEnvMaps(false)
        , mConvertAlphaTestToAlphaToCoverage(false)
        , mTranslucentFramebuffer(false)
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
        if (node.getStateSet())
        {
            pushRequirements(node);
            applyStateSet(node.getStateSet(), node);
            traverse(node);
            popRequirements();
        }
        else
            traverse(node);
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

        osg::ref_ptr<osg::UserDataContainer> newUserData = static_cast<osg::UserDataContainer *>(object.getUserDataContainer()->clone(osg::CopyOp::SHALLOW_COPY));
        object.setUserDataContainer(newUserData);
        return newUserData.get();
    }

    osg::StateSet* getRemovedState(osg::StateSet& stateSet)
    {
        if (!stateSet.getUserDataContainer())
            return nullptr;

        return static_cast<osg::StateSet *>(stateSet.getUserDataContainer()->getUserObject("removedState"));
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

    const char* defaultTextures[] = { "diffuseMap", "normalMap", "emissiveMap", "darkMap", "detailMap", "envMap", "specularMap", "decalMap", "bumpMap" };
    bool isTextureNameRecognized(const std::string& name)
    {
        for (unsigned int i=0; i<sizeof(defaultTextures)/sizeof(defaultTextures[0]); ++i)
            if (name == defaultTextures[i])
                return true;
        return false;
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

        if (!texAttributes.empty())
        {
            const osg::Texture* diffuseMap = nullptr;
            const osg::Texture* normalMap = nullptr;
            const osg::Texture* specularMap = nullptr;
            const osg::Texture* bumpMap = nullptr;
            for(unsigned int unit=0;unit<texAttributes.size();++unit)
            {
                const osg::StateAttribute *attr = stateset->getTextureAttribute(unit, osg::StateAttribute::TEXTURE);
                if (attr)
                {
                    const osg::Texture* texture = attr->asTexture();
                    if (texture)
                    {
                        std::string texName = texture->getName();
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
                                // normal maps are by default off since the FFP can't render them, now that we'll use shaders switch to On
                                writableStateSet->setTextureMode(unit, GL_TEXTURE_2D, osg::StateAttribute::ON);
                                normalMap = texture;
                            }
                            else if (texName == "diffuseMap")
                                diffuseMap = texture;
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
                        }
                        else if (!mTranslucentFramebuffer)
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
                if (mImageManager.getVFS()->exists(normalHeightMap))
                {
                    image = mImageManager.getImage(normalHeightMap);
                    normalHeight = true;
                }
                else
                {
                    Misc::StringUtils::replaceLast(normalMapFileName, ".", mNormalMapPattern + ".");
                    if (mImageManager.getVFS()->exists(normalMapFileName))
                    {
                        image = mImageManager.getImage(normalMapFileName);
                    }
                }
                // Avoid using the auto-detected normal map if it's already being used as a bump map.
                // It's probably not an actual normal map.
                bool hasNamesakeBumpMap = image && bumpMap && bumpMap->getImage(0) && image->getFileName() == bumpMap->getImage(0)->getFileName();

                if (!hasNamesakeBumpMap && image)
                {
                    osg::ref_ptr<osg::Texture2D> normalMapTex (new osg::Texture2D(image));
                    normalMapTex->setTextureSize(image->s(), image->t());
                    normalMapTex->setWrap(osg::Texture::WRAP_S, diffuseMap->getWrap(osg::Texture::WRAP_S));
                    normalMapTex->setWrap(osg::Texture::WRAP_T, diffuseMap->getWrap(osg::Texture::WRAP_T));
                    normalMapTex->setFilter(osg::Texture::MIN_FILTER, diffuseMap->getFilter(osg::Texture::MIN_FILTER));
                    normalMapTex->setFilter(osg::Texture::MAG_FILTER, diffuseMap->getFilter(osg::Texture::MAG_FILTER));
                    normalMapTex->setMaxAnisotropy(diffuseMap->getMaxAnisotropy());
                    normalMapTex->setName("normalMap");

                    int unit = texAttributes.size();
                    if (!writableStateSet)
                        writableStateSet = getWritableStateSet(node);
                    writableStateSet->setTextureAttributeAndModes(unit, normalMapTex, osg::StateAttribute::ON);
                    mRequirements.back().mTextures[unit] = "normalMap";
                    mRequirements.back().mTexStageRequiringTangents = unit;
                    mRequirements.back().mShaderRequired = true;
                    mRequirements.back().mNormalHeight = normalHeight;
                }
            }
            if (mAutoUseSpecularMaps && diffuseMap != nullptr && specularMap == nullptr && diffuseMap->getImage(0))
            {
                std::string specularMapFileName = diffuseMap->getImage(0)->getFileName();
                Misc::StringUtils::replaceLast(specularMapFileName, ".", mSpecularMapPattern + ".");
                if (mImageManager.getVFS()->exists(specularMapFileName))
                {
                    osg::ref_ptr<osg::Image> image (mImageManager.getImage(specularMapFileName));
                    osg::ref_ptr<osg::Texture2D> specularMapTex (new osg::Texture2D(image));
                    specularMapTex->setTextureSize(image->s(), image->t());
                    specularMapTex->setWrap(osg::Texture::WRAP_S, diffuseMap->getWrap(osg::Texture::WRAP_S));
                    specularMapTex->setWrap(osg::Texture::WRAP_T, diffuseMap->getWrap(osg::Texture::WRAP_T));
                    specularMapTex->setFilter(osg::Texture::MIN_FILTER, diffuseMap->getFilter(osg::Texture::MIN_FILTER));
                    specularMapTex->setFilter(osg::Texture::MAG_FILTER, diffuseMap->getFilter(osg::Texture::MAG_FILTER));
                    specularMapTex->setMaxAnisotropy(diffuseMap->getMaxAnisotropy());
                    specularMapTex->setName("specularMap");

                    int unit = texAttributes.size();
                    if (!writableStateSet)
                        writableStateSet = getWritableStateSet(node);
                    writableStateSet->setTextureAttributeAndModes(unit, specularMapTex, osg::StateAttribute::ON);
                    mRequirements.back().mTextures[unit] = "specularMap";
                    mRequirements.back().mShaderRequired = true;
                }
            }
        }

        const osg::StateSet::AttributeList& attributes = stateset->getAttributeList();
        osg::StateSet::AttributeList removedAttributes;
        if (osg::ref_ptr<osg::StateSet> removedState = getRemovedState(*stateset))
            removedAttributes = removedState->getAttributeList();
        osg::ref_ptr<AddedState> addedState = getAddedState(*stateset);

        for (const auto* attributeMap : std::initializer_list<const osg::StateSet::AttributeList*>{ &attributes, &removedAttributes })
        {
            for (osg::StateSet::AttributeList::const_iterator it = attributeMap->begin(); it != attributeMap->end(); ++it)
            {
                if (addedState && attributeMap != &removedAttributes && addedState->hasAttribute(it->first))
                    continue;
                if (it->first.first == osg::StateAttribute::MATERIAL)
                {
                    // This should probably be moved out of ShaderRequirements and be applied directly now it's a uniform instead of a define
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
                    if (!mRequirements.back().mAlphaTestOverridden || it->second.second & osg::StateAttribute::PROTECTED)
                    {
                        if (it->second.second & osg::StateAttribute::OVERRIDE)
                            mRequirements.back().mAlphaTestOverridden = true;

                        const osg::AlphaFunc* alpha = static_cast<const osg::AlphaFunc*>(it->second.first.get());
                        mRequirements.back().mAlphaFunc = alpha->getFunction();
                        mRequirements.back().mAlphaRef = alpha->getReferenceValue();
                    }
                }
            }
        }

        unsigned int alphaBlend = stateset->getMode(GL_BLEND);
        if (alphaBlend != osg::StateAttribute::INHERIT && (!mRequirements.back().mAlphaBlendOverridden || alphaBlend & osg::StateAttribute::PROTECTED))
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

    void ShaderVisitor::createProgram(const ShaderRequirements &reqs)
    {
        if (!reqs.mShaderRequired && !mForceShaders)
        {
            ensureFFP(*reqs.mNode);
            return;
        }

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
        for (unsigned int i=0; i<sizeof(defaultTextures)/sizeof(defaultTextures[0]); ++i)
        {
            defineMap[defaultTextures[i]] = "0";
            defineMap[std::string(defaultTextures[i]) + std::string("UV")] = "0";
        }
        for (std::map<int, std::string>::const_iterator texIt = reqs.mTextures.begin(); texIt != reqs.mTextures.end(); ++texIt)
        {
            defineMap[texIt->second] = "1";
            defineMap[texIt->second + std::string("UV")] = std::to_string(texIt->first);
        }

        if (defineMap["diffuseMap"] == "0")
            writableStateSet->addUniform(new osg::Uniform("useDiffuseMapForShadowAlpha", false));

        defineMap["parallax"] = reqs.mNormalHeight ? "1" : "0";

        writableStateSet->addUniform(new osg::Uniform("colorMode", reqs.mColorMode));
        addedState->addUniform("colorMode");

        defineMap["alphaFunc"] = std::to_string(reqs.mAlphaFunc);

        // back up removed state in case recreateShaders gets rid of the shader later
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
            writableStateSet->setAttribute(RemovedAlphaFunc::getInstance(reqs.mAlphaFunc), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            addedState->setAttribute(RemovedAlphaFunc::getInstance(reqs.mAlphaFunc));

            // Blending won't work with A2C as we use the alpha channel for coverage. gl_SampleCoverage from ARB_sample_shading would save the day, but requires GLSL 130
            if (mConvertAlphaTestToAlphaToCoverage && !reqs.mAlphaBlend)
            {
                writableStateSet->setMode(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB, osg::StateAttribute::ON);
                addedState->setMode(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB);
                defineMap["alphaToCoverage"] = "1";
            }

            // Adjusting coverage isn't safe with blending on as blending requires the alpha to be intact.
            // Maybe we could also somehow (e.g. userdata) detect when the diffuse map has coverage-preserving mip maps in the future
            if (!reqs.mAlphaBlend)
                defineMap["adjustCoverage"] = "1";

            // Preventing alpha tested stuff shrinking as lower mip levels are used requires knowing the texture size
            osg::ref_ptr<osg::GLExtensions> exts = osg::GLExtensions::Get(0, false);
            if (exts && exts->isGpuShader4Supported)
                defineMap["useGPUShader4"] = "1";
            // We could fall back to a texture size uniform if EXT_gpu_shader4 is missing
        }

        if (writableStateSet->getMode(GL_ALPHA_TEST) != osg::StateAttribute::INHERIT && !previousAddedState->hasMode(GL_ALPHA_TEST))
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

        defineMap["translucentFramebuffer"] = mTranslucentFramebuffer ? "1" : "0";

        std::string shaderPrefix;
        if (!node.getUserValue("shaderPrefix", shaderPrefix))
            shaderPrefix = mDefaultShaderPrefix;

        osg::ref_ptr<osg::Shader> vertexShader (mShaderManager.getShader(shaderPrefix + "_vertex.glsl", defineMap, osg::Shader::VERTEX));
        osg::ref_ptr<osg::Shader> fragmentShader (mShaderManager.getShader(shaderPrefix + "_fragment.glsl", defineMap, osg::Shader::FRAGMENT));

        if (vertexShader && fragmentShader)
        {
            auto program = mShaderManager.getProgram(vertexShader, fragmentShader, mProgramTemplate);
            writableStateSet->setAttributeAndModes(program, osg::StateAttribute::ON);
            addedState->setAttributeAndModes(program);

            for (std::map<int, std::string>::const_iterator texIt = reqs.mTextures.begin(); texIt != reqs.mTextures.end(); ++texIt)
            {
                writableStateSet->addUniform(new osg::Uniform(texIt->second.c_str(), texIt->first), osg::StateAttribute::ON);
                addedState->addUniform(texIt->second);
            }
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
            for (auto itr = writableStateSet->getUniformList().begin(); itr != writableStateSet->getUniformList().end();)
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
            for (std::map<int, std::string>::const_iterator it = reqs.mTextures.begin(); it != reqs.mTextures.end(); ++it)
            {
                if (sourceGeometry.getTexCoordArray(it->first) == nullptr)
                {
                    sourceGeometry.setTexCoordArray(it->first, sourceGeometry.getTexCoordArray(0));
                    changed = true;
                }
            }

            if (generateTangents)
            {
                osg::ref_ptr<osgUtil::TangentSpaceGenerator> generator (new osgUtil::TangentSpaceGenerator);
                generator->generate(&sourceGeometry, reqs.mTexStageRequiringTangents);

                sourceGeometry.setTexCoordArray(7, generator->getTangentArray(), osg::Array::BIND_PER_VERTEX);
                changed = true;
            }
        }
        return changed;
    }

    void ShaderVisitor::apply(osg::Geometry& geometry)
    {
        bool needPop = (geometry.getStateSet() != nullptr);
        if (geometry.getStateSet()) // TODO: check if stateset affects shader permutation before pushing it
        {
            pushRequirements(geometry);
            applyStateSet(geometry.getStateSet(), geometry);
        }

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
        // non-Geometry drawable (e.g. particle system)
        bool needPop = (drawable.getStateSet() != nullptr);

        if (drawable.getStateSet())
        {
            pushRequirements(drawable);
            applyStateSet(drawable.getStateSet(), drawable);
        }

        if (!mRequirements.empty())
        {
            const ShaderRequirements& reqs = mRequirements.back();
            createProgram(reqs);

            if (auto rig = dynamic_cast<SceneUtil::RigGeometry*>(&drawable))
            {
                osg::ref_ptr<osg::Geometry> sourceGeometry = rig->getSourceGeometry();
                if (sourceGeometry && adjustGeometry(*sourceGeometry, reqs))
                    rig->setSourceGeometry(sourceGeometry);
            }
            else if (auto morph = dynamic_cast<SceneUtil::MorphGeometry*>(&drawable))
            {
                osg::ref_ptr<osg::Geometry> sourceGeometry = morph->getSourceGeometry();
                if (sourceGeometry && adjustGeometry(*sourceGeometry, reqs))
                    morph->setSourceGeometry(sourceGeometry);
            }
        }
        else
            ensureFFP(drawable);

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

    void ShaderVisitor::setNormalMapPattern(const std::string &pattern)
    {
        mNormalMapPattern = pattern;
    }

    void ShaderVisitor::setNormalHeightMapPattern(const std::string &pattern)
    {
        mNormalHeightMapPattern = pattern;
    }

    void ShaderVisitor::setAutoUseSpecularMaps(bool use)
    {
        mAutoUseSpecularMaps = use;
    }

    void ShaderVisitor::setSpecularMapPattern(const std::string &pattern)
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

    void ShaderVisitor::setTranslucentFramebuffer(bool translucent)
    {
        mTranslucentFramebuffer = translucent;
    }

    ReinstateRemovedStateVisitor::ReinstateRemovedStateVisitor(bool allowedToModifyStateSets)
        : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        , mAllowedToModifyStateSets(allowedToModifyStateSets)
    {
    }

    void ReinstateRemovedStateVisitor::apply(osg::Node& node)
    {
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

                for (const auto&[mode, value] : removedState->getModeList())
                    writableStateSet->setMode(mode, value);

                for (const auto& attribute : removedState->getAttributeList())
                    writableStateSet->setAttribute(attribute.second.first, attribute.second.second);
            }
        }

        traverse(node);
    }

}
