#include "scenemanager.hpp"

#include <cstdlib>
#include <filesystem>

#include <osg/AlphaFunc>
#include <osg/Capability>
#include <osg/ColorMaski>
#include <osg/Group>
#include <osg/Node>
#include <osg/UserDataContainer>

#include <osgAnimation/BasicAnimationManager>
#include <osgAnimation/Bone>
#include <osgAnimation/RigGeometry>
#include <osgAnimation/UpdateBone>

#include <osgParticle/ParticleSystem>

#include <osgUtil/IncrementalCompileOperation>

#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgDB/SharedStateManager>

#include <components/debug/debuglog.hpp>

#include <components/nifosg/controller.hpp>
#include <components/nifosg/nifloader.hpp>

#include <components/nif/niffile.hpp>

#include <components/misc/algorithm.hpp>
#include <components/misc/osguservalues.hpp>
#include <components/misc/pathhelpers.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/misc/strings/conversion.hpp>

#include <components/vfs/manager.hpp>
#include <components/vfs/pathutil.hpp>

#include <components/sceneutil/clone.hpp>
#include <components/sceneutil/controller.hpp>
#include <components/sceneutil/depth.hpp>
#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/optimizer.hpp>
#include <components/sceneutil/riggeometryosgaextension.hpp>
#include <components/sceneutil/util.hpp>
#include <components/sceneutil/visitor.hpp>

#include <components/shader/shadermanager.hpp>
#include <components/shader/shadervisitor.hpp>

#include <components/files/conversion.hpp>
#include <components/files/hash.hpp>
#include <components/files/memorystream.hpp>

#include "bgsmfilemanager.hpp"
#include "errormarker.hpp"
#include "imagemanager.hpp"
#include "niffilemanager.hpp"
#include "objectcache.hpp"

namespace
{
    class InitWorldSpaceParticlesCallback
        : public SceneUtil::NodeCallback<InitWorldSpaceParticlesCallback, osgParticle::ParticleSystem*>
    {
    public:
        void operator()(osgParticle::ParticleSystem* node, osg::NodeVisitor* nv)
        {
            // HACK: Ignore the InverseWorldMatrix transform the particle system is attached to
            if (node->getNumParents() && node->getParent(0)->getNumParents())
                transformInitialParticles(node, node->getParent(0)->getParent(0));

            node->removeUpdateCallback(this);
        }

        void transformInitialParticles(osgParticle::ParticleSystem* partsys, osg::Node* node)
        {
            osg::NodePathList nodepaths = node->getParentalNodePaths();
            if (nodepaths.empty())
                return;
            osg::Matrixf worldMat = osg::computeLocalToWorld(nodepaths[0]);
            worldMat.orthoNormalize(worldMat); // scale is already applied on the particle node
            for (int i = 0; i < partsys->numParticles(); ++i)
            {
                partsys->getParticle(i)->transformPositionVelocity(worldMat);
            }

            // transform initial bounds to worldspace
            osg::BoundingSphere sphere(partsys->getInitialBound());
            SceneUtil::transformBoundingSphere(worldMat, sphere);
            osg::BoundingBox box;
            box.expandBy(sphere);
            partsys->setInitialBound(box);
        }
    };

    class InitParticlesVisitor : public osg::NodeVisitor
    {
    public:
        /// @param mask The node mask to set on ParticleSystem nodes.
        InitParticlesVisitor(unsigned int mask)
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
            , mMask(mask)
        {
        }

        bool isWorldSpaceParticleSystem(osgParticle::ParticleSystem* partsys)
        {
            // HACK: ParticleSystem has no getReferenceFrame()
            return (partsys->getUserDataContainer() && partsys->getUserDataContainer()->getNumDescriptions() > 0
                && partsys->getUserDataContainer()->getDescriptions()[0] == "worldspace");
        }

        void apply(osg::Drawable& drw) override
        {
            if (osgParticle::ParticleSystem* partsys = dynamic_cast<osgParticle::ParticleSystem*>(&drw))
            {
                if (isWorldSpaceParticleSystem(partsys))
                {
                    partsys->addUpdateCallback(new InitWorldSpaceParticlesCallback);
                }
                partsys->setNodeMask(mMask);
            }
        }

    private:
        unsigned int mMask;
    };
}

namespace Resource
{
    void TemplateMultiRef::addRef(const osg::Node* node)
    {
        mObjects.emplace_back(node);
    }

    class SharedStateManager : public osgDB::SharedStateManager
    {
    public:
        size_t getNumSharedTextures() const { return _sharedTextureList.size(); }

        size_t getNumSharedStateSets() const { return _sharedStateSetList.size(); }

        void clearCache()
        {
            std::lock_guard<OpenThreads::Mutex> lock(_listMutex);
            _sharedTextureList.clear();
            _sharedStateSetList.clear();
        }
    };

    /// Set texture filtering settings on textures contained in a FlipController.
    class SetFilterSettingsControllerVisitor : public SceneUtil::ControllerVisitor
    {
    public:
        SetFilterSettingsControllerVisitor(
            osg::Texture::FilterMode minFilter, osg::Texture::FilterMode magFilter, float maxAnisotropy)
            : mMinFilter(minFilter)
            , mMagFilter(magFilter)
            , mMaxAnisotropy(maxAnisotropy)
        {
        }

        void visit(osg::Node& node, SceneUtil::Controller& ctrl) override
        {
            if (NifOsg::FlipController* flipctrl = dynamic_cast<NifOsg::FlipController*>(&ctrl))
            {
                for (const osg::ref_ptr<osg::Texture2D>& tex : flipctrl->getTextures())
                {
                    tex->setFilter(osg::Texture::MIN_FILTER, mMinFilter);
                    tex->setFilter(osg::Texture::MAG_FILTER, mMagFilter);
                    tex->setMaxAnisotropy(mMaxAnisotropy);
                }
            }
        }

    private:
        osg::Texture::FilterMode mMinFilter;
        osg::Texture::FilterMode mMagFilter;
        float mMaxAnisotropy;
    };

    /// Set texture filtering settings on textures contained in StateSets.
    class SetFilterSettingsVisitor : public osg::NodeVisitor
    {
    public:
        SetFilterSettingsVisitor(
            osg::Texture::FilterMode minFilter, osg::Texture::FilterMode magFilter, float maxAnisotropy)
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
            , mMinFilter(minFilter)
            , mMagFilter(magFilter)
            , mMaxAnisotropy(maxAnisotropy)
        {
        }

        void apply(osg::Node& node) override
        {
            osg::StateSet* stateset = node.getStateSet();
            if (stateset)
                applyStateSet(stateset);

            traverse(node);
        }

        void applyStateSet(osg::StateSet* stateset)
        {
            const osg::StateSet::TextureAttributeList& texAttributes = stateset->getTextureAttributeList();
            for (unsigned int unit = 0; unit < texAttributes.size(); ++unit)
            {
                osg::StateAttribute* texture = stateset->getTextureAttribute(unit, osg::StateAttribute::TEXTURE);
                if (texture)
                    applyStateAttribute(texture);
            }
        }

        void applyStateAttribute(osg::StateAttribute* attr)
        {
            osg::Texture* tex = attr->asTexture();
            if (tex)
            {
                tex->setFilter(osg::Texture::MIN_FILTER, mMinFilter);
                tex->setFilter(osg::Texture::MAG_FILTER, mMagFilter);
                tex->setMaxAnisotropy(mMaxAnisotropy);
            }
        }

    private:
        osg::Texture::FilterMode mMinFilter;
        osg::Texture::FilterMode mMagFilter;
        float mMaxAnisotropy;
    };

    // Check Collada extra descriptions
    class ColladaDescriptionVisitor : public osg::NodeVisitor
    {
    public:
        ColladaDescriptionVisitor()
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
            , mSkeleton(nullptr)
        {
        }

        osg::AlphaFunc::ComparisonFunction getTestMode(std::string mode)
        {
            if (mode == "ALWAYS")
                return osg::AlphaFunc::ALWAYS;
            if (mode == "LESS")
                return osg::AlphaFunc::LESS;
            if (mode == "EQUAL")
                return osg::AlphaFunc::EQUAL;
            if (mode == "LEQUAL")
                return osg::AlphaFunc::LEQUAL;
            if (mode == "GREATER")
                return osg::AlphaFunc::GREATER;
            if (mode == "NOTEQUAL")
                return osg::AlphaFunc::NOTEQUAL;
            if (mode == "GEQUAL")
                return osg::AlphaFunc::GEQUAL;
            if (mode == "NEVER")
                return osg::AlphaFunc::NEVER;

            Log(Debug::Warning) << "Unexpected alpha testing mode: " << mode;
            return osg::AlphaFunc::LEQUAL;
        }

        void apply(osg::Node& node) override
        {
            if (osg::StateSet* stateset = node.getStateSet())
            {
                if (stateset->getRenderingHint() == osg::StateSet::TRANSPARENT_BIN)
                {
                    osg::ref_ptr<osg::Depth> depth = new SceneUtil::AutoDepth;
                    depth->setWriteMask(false);

                    stateset->setAttributeAndModes(depth, osg::StateAttribute::ON);
                }
                else if (stateset->getRenderingHint() == osg::StateSet::OPAQUE_BIN)
                {
                    osg::ref_ptr<osg::Depth> depth = new SceneUtil::AutoDepth;
                    depth->setWriteMask(true);

                    stateset->setAttributeAndModes(depth, osg::StateAttribute::ON);
                }
            }
            /* Check if the <node> has <extra type="Node"> <technique profile="OpenSceneGraph"> <Descriptions>
               <Description> correct format for OpenMW: <Description>alphatest mode value MaterialName</Description> e.g
               <Description>alphatest GEQUAL 0.8 MyAlphaTestedMaterial</Description> */
            std::vector<std::string> descriptions = node.getDescriptions();
            for (const auto& description : descriptions)
            {
                mDescriptions.emplace_back(description);
            }

            // Iterate each description, and see if the current node uses the specified material for alpha testing
            if (node.getStateSet())
            {
                for (const auto& description : mDescriptions)
                {
                    std::vector<std::string> descriptionParts;
                    std::istringstream descriptionStringStream(description);
                    for (std::string part; std::getline(descriptionStringStream, part, ' ');)
                    {
                        descriptionParts.emplace_back(part);
                    }

                    if (descriptionParts.size() > (3) && descriptionParts.at(3) == node.getStateSet()->getName())
                    {
                        if (descriptionParts.at(0) == "alphatest")
                        {
                            osg::AlphaFunc::ComparisonFunction mode = getTestMode(descriptionParts.at(1));
                            osg::ref_ptr<osg::AlphaFunc> alphaFunc(new osg::AlphaFunc(
                                mode, Misc::StringUtils::toNumeric<float>(descriptionParts.at(2), 0.0f)));
                            node.getStateSet()->setAttributeAndModes(alphaFunc, osg::StateAttribute::ON);
                        }
                    }

                    if (descriptionParts.size() > (0) && descriptionParts.at(0) == "bodypart")
                    {
                        SceneUtil::FindByClassVisitor osgaRigFinder("RigGeometryHolder");
                        node.accept(osgaRigFinder);
                        for (osg::Node* foundRigNode : osgaRigFinder.mFoundNodes)
                        {
                            if (SceneUtil::RigGeometryHolder* rigGeometryHolder
                                = dynamic_cast<SceneUtil::RigGeometryHolder*>(foundRigNode))
                                mRigGeometryHolders.emplace_back(
                                    osg::ref_ptr<SceneUtil::RigGeometryHolder>(rigGeometryHolder));
                            else
                                Log(Debug::Error) << "Converted RigGeometryHolder is of a wrong type.";
                        }

                        if (!mRigGeometryHolders.empty())
                        {
                            osgAnimation::RigGeometry::FindNearestParentSkeleton skeletonFinder;
                            mRigGeometryHolders[0]->accept(skeletonFinder);
                            if (skeletonFinder._root.valid())
                                mSkeleton = skeletonFinder._root;
                        }
                    }
                }
            }

            traverse(node);
        }

    private:
        std::vector<std::string> mDescriptions;

    public:
        osgAnimation::Skeleton* mSkeleton; // pointer is valid only if the model is a bodypart, osg::ref_ptr<Skeleton>
        std::vector<osg::ref_ptr<SceneUtil::RigGeometryHolder>> mRigGeometryHolders;
    };

    class ReplaceAnimationUnderscoresVisitor : public osg::NodeVisitor
    {
    public:
        ReplaceAnimationUnderscoresVisitor()
            : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
        {
        }

        void apply(osg::Node& node) override
        {
            // NOTE: MUST update the animation manager names first!
            if (auto* animationManager = dynamic_cast<osgAnimation::BasicAnimationManager*>(node.getUpdateCallback()))
                renameAnimationChannelTargets(*animationManager);

            // Then, any applicable node names
            if (auto* rigGeometry = dynamic_cast<osgAnimation::RigGeometry*>(&node))
            {
                renameNode(*rigGeometry);
                updateVertexInfluenceMap(*rigGeometry);
            }
            else if (auto* matrixTransform = dynamic_cast<osg::MatrixTransform*>(&node))
            {
                renameNode(*matrixTransform);
                renameUpdateCallbacks(*matrixTransform);
            }

            traverse(node);
        }

    private:
        inline void renameNode(osg::Node& node)
        {
            node.setName(Misc::StringUtils::underscoresToSpaces(node.getName()));
        }

        void renameUpdateCallbacks(osg::MatrixTransform& node)
        {
            osg::Callback* cb = node.getUpdateCallback();
            while (cb)
            {
                auto* animCb = dynamic_cast<osgAnimation::AnimationUpdateCallback<osg::NodeCallback>*>(cb);
                if (animCb)
                {
                    std::string newAnimCbName = Misc::StringUtils::underscoresToSpaces(animCb->getName());
                    animCb->setName(newAnimCbName);
                }
                cb = cb->getNestedCallback();
            }
        }

        void updateVertexInfluenceMap(osgAnimation::RigGeometry& rig)
        {
            osgAnimation::VertexInfluenceMap* vertexInfluenceMap = rig.getInfluenceMap();
            if (!vertexInfluenceMap)
                return;

            std::vector<std::pair<std::string, std::string>> renameList;
            for (const auto& [oldBoneName, _] : *vertexInfluenceMap)
            {
                const std::string newBoneName = Misc::StringUtils::underscoresToSpaces(oldBoneName);
                if (newBoneName != oldBoneName)
                    renameList.emplace_back(oldBoneName, newBoneName);
            }

            for (const auto& [oldName, newName] : renameList)
            {
                if (vertexInfluenceMap->find(newName) == vertexInfluenceMap->end())
                    (*vertexInfluenceMap)[newName] = std::move((*vertexInfluenceMap)[oldName]);
                vertexInfluenceMap->erase(oldName);
            }
        }

        void renameAnimationChannelTargets(osgAnimation::BasicAnimationManager& animManager)
        {
            for (const osgAnimation::Animation* animation : animManager.getAnimationList())
            {
                if (animation)
                {
                    const osgAnimation::ChannelList& channels = animation->getChannels();
                    for (osgAnimation::Channel* channel : channels)
                        channel->setTargetName(Misc::StringUtils::underscoresToSpaces(channel->getTargetName()));
                }
            }
        }
    };

    SceneManager::SceneManager(const VFS::Manager* vfs, Resource::ImageManager* imageManager,
        Resource::NifFileManager* nifFileManager, Resource::BgsmFileManager* bgsmFileManager, double expiryDelay)
        : ResourceManager(vfs, expiryDelay)
        , mShaderManager(new Shader::ShaderManager)
        , mSharedStateManager(new SharedStateManager)
        , mImageManager(imageManager)
        , mNifFileManager(nifFileManager)
        , mBgsmFileManager(bgsmFileManager)
        , mMinFilter(osg::Texture::LINEAR_MIPMAP_LINEAR)
        , mMagFilter(osg::Texture::LINEAR)
        , mMaxAnisotropy(1.f)
        , mParticleSystemMask(~0u)
        , mLightingMethod(SceneUtil::LightingMethod::FFP)
    {
    }

    void SceneManager::setForceShaders(bool force)
    {
        mForceShaders = force;
    }

    bool SceneManager::getForceShaders() const
    {
        return mForceShaders;
    }

    void SceneManager::recreateShaders(osg::ref_ptr<osg::Node> node, const std::string& shaderPrefix,
        bool forceShadersForNode, const osg::Program* programTemplate)
    {
        osg::ref_ptr<Shader::ShaderVisitor> shaderVisitor(createShaderVisitor(shaderPrefix));
        shaderVisitor->setAllowedToModifyStateSets(false);
        shaderVisitor->setProgramTemplate(programTemplate);
        if (forceShadersForNode)
            shaderVisitor->setForceShaders(true);
        node->accept(*shaderVisitor);
    }

    void SceneManager::reinstateRemovedState(osg::ref_ptr<osg::Node> node)
    {
        osg::ref_ptr<Shader::ReinstateRemovedStateVisitor> reinstateRemovedStateVisitor
            = new Shader::ReinstateRemovedStateVisitor(false);
        node->accept(*reinstateRemovedStateVisitor);
    }

    void SceneManager::setClampLighting(bool clamp)
    {
        mClampLighting = clamp;
    }

    bool SceneManager::getClampLighting() const
    {
        return mClampLighting;
    }

    void SceneManager::setAutoUseNormalMaps(bool use)
    {
        mAutoUseNormalMaps = use;
    }

    void SceneManager::setNormalMapPattern(const std::string& pattern)
    {
        mNormalMapPattern = pattern;
    }

    void SceneManager::setNormalHeightMapPattern(const std::string& pattern)
    {
        mNormalHeightMapPattern = pattern;
    }

    void SceneManager::setAutoUseSpecularMaps(bool use)
    {
        mAutoUseSpecularMaps = use;
    }

    void SceneManager::setSpecularMapPattern(const std::string& pattern)
    {
        mSpecularMapPattern = pattern;
    }

    void SceneManager::setApplyLightingToEnvMaps(bool apply)
    {
        mApplyLightingToEnvMaps = apply;
    }

    void SceneManager::setSupportedLightingMethods(const SceneUtil::LightManager::SupportedMethods& supported)
    {
        mSupportedLightingMethods = supported;
    }

    bool SceneManager::isSupportedLightingMethod(SceneUtil::LightingMethod method) const
    {
        return mSupportedLightingMethods[static_cast<int>(method)];
    }

    void SceneManager::setLightingMethod(SceneUtil::LightingMethod method)
    {
        mLightingMethod = method;

        if (mLightingMethod == SceneUtil::LightingMethod::SingleUBO)
        {
            osg::ref_ptr<osg::Program> program = new osg::Program;
            program->addBindUniformBlock("LightBufferBinding", static_cast<int>(UBOBinding::LightBuffer));
            mShaderManager->setProgramTemplate(program);
        }
    }

    SceneUtil::LightingMethod SceneManager::getLightingMethod() const
    {
        return mLightingMethod;
    }

    void SceneManager::setConvertAlphaTestToAlphaToCoverage(bool convert)
    {
        mConvertAlphaTestToAlphaToCoverage = convert;
    }

    void SceneManager::setAdjustCoverageForAlphaTest(bool adjustCoverage)
    {
        mAdjustCoverageForAlphaTest = adjustCoverage;
    }

    void SceneManager::setOpaqueDepthTex(osg::ref_ptr<osg::Texture> texturePing, osg::ref_ptr<osg::Texture> texturePong)
    {
        mOpaqueDepthTex = { texturePing, texturePong };
    }

    osg::ref_ptr<osg::Texture> SceneManager::getOpaqueDepthTex(size_t frame)
    {
        return mOpaqueDepthTex[frame % 2];
    }

    SceneManager::~SceneManager()
    {
        // this has to be defined in the .cpp file as we can't delete incomplete types
    }

    Shader::ShaderManager& SceneManager::getShaderManager()
    {
        return *mShaderManager.get();
    }

    void SceneManager::setShaderPath(const std::filesystem::path& path)
    {
        mShaderManager->setShaderPath(path);
    }

    bool SceneManager::checkLoaded(VFS::Path::NormalizedView name, double timeStamp)
    {
        return mCache->checkInObjectCache(name, timeStamp);
    }

    void SceneManager::setUpNormalsRTForStateSet(osg::StateSet* stateset, bool enabled)
    {
        if (!getSupportsNormalsRT())
            return;
        stateset->setAttributeAndModes(new osg::ColorMaski(1, enabled, enabled, enabled, enabled));

        if (enabled)
            stateset->setAttributeAndModes(new osg::Disablei(GL_BLEND, 1));
    }

    /// @brief Callback to read image files from the VFS.
    class ImageReadCallback : public osgDB::ReadFileCallback
    {
    public:
        ImageReadCallback(Resource::ImageManager* imageMgr)
            : mImageManager(imageMgr)
        {
        }

        osgDB::ReaderWriter::ReadResult readImage(const std::string& filename, const osgDB::Options* options) override
        {
            auto filePath = Files::pathFromUnicodeString(filename);
            if (filePath.is_absolute())
                // It is a hack. Needed because either OSG or libcollada-dom tries to make an absolute path from
                // our relative VFS path by adding current working directory.
                filePath = std::filesystem::relative(filename, osgDB::getCurrentWorkingDirectory());
            try
            {
                return osgDB::ReaderWriter::ReadResult(
                    mImageManager->getImage(VFS::Path::toNormalized(Files::pathToUnicodeString(filePath))),
                    osgDB::ReaderWriter::ReadResult::FILE_LOADED);
            }
            catch (std::exception& e)
            {
                return osgDB::ReaderWriter::ReadResult(e.what());
            }
        }

    private:
        Resource::ImageManager* mImageManager;
    };

    namespace
    {
        osg::ref_ptr<osg::Node> loadNonNif(
            VFS::Path::NormalizedView normalizedFilename, std::istream& model, Resource::ImageManager* imageManager)
        {
            const std::string_view ext = Misc::getFileExtension(normalizedFilename.value());
            const bool isColladaFile = ext == "dae";
            osgDB::ReaderWriter* reader = osgDB::Registry::instance()->getReaderWriterForExtension(std::string(ext));
            if (!reader)
            {
                std::stringstream errormsg;
                errormsg << "Error loading " << normalizedFilename << ": no readerwriter for '" << ext << "' found"
                         << std::endl;
                throw std::runtime_error(errormsg.str());
            }

            osg::ref_ptr<osgDB::Options> options(new osgDB::Options);
            // Set a ReadFileCallback so that image files referenced in the model are read from our virtual file system
            // instead of the osgDB. Note, for some formats (.obj/.mtl) that reference other (non-image) files a
            // findFileCallback would be necessary. but findFileCallback does not support virtual files, so we can't
            // implement it.
            options->setReadFileCallback(new ImageReadCallback(imageManager));
            if (isColladaFile)
                options->setOptionString("daeUseSequencedTextureUnits");

            const std::array<std::uint64_t, 2> fileHash = Files::getHash(normalizedFilename.value(), model);

            osgDB::ReaderWriter::ReadResult result = reader->readNode(model, options);
            if (!result.success())
            {
                std::stringstream errormsg;
                errormsg << "Error loading " << normalizedFilename << ": " << result.message() << " code "
                         << result.status() << std::endl;
                throw std::runtime_error(errormsg.str());
            }

            // Recognize and hide collision node
            unsigned int hiddenNodeMask = 0;
            SceneUtil::FindByNameVisitor nameFinder("Collision");

            auto node = result.getNode();
            node->accept(nameFinder);
            if (nameFinder.mFoundNode)
                nameFinder.mFoundNode->setNodeMask(hiddenNodeMask);

            // Recognize and convert osgAnimation::RigGeometry to OpenMW-optimized type
            SceneUtil::FindByClassVisitor rigFinder("RigGeometry");
            node->accept(rigFinder);

            // If a collada file with rigs, we should replace underscores with spaces
            if (isColladaFile && !rigFinder.mFoundNodes.empty())
            {
                ReplaceAnimationUnderscoresVisitor renamingVisitor;
                node->accept(renamingVisitor);
            }

            // Replace osg::Depth with reverse-Z-compatible SceneUtil::AutoDepth
            SceneUtil::ReplaceDepthVisitor replaceDepthVisitor;
            node->accept(replaceDepthVisitor);

            for (osg::Node* foundRigNode : rigFinder.mFoundNodes)
            {
                if (foundRigNode->libraryName() == std::string_view("osgAnimation"))
                {
                    osgAnimation::RigGeometry* foundRigGeometry = static_cast<osgAnimation::RigGeometry*>(foundRigNode);

                    osg::ref_ptr<SceneUtil::RigGeometryHolder> newRig
                        = new SceneUtil::RigGeometryHolder(*foundRigGeometry, osg::CopyOp::DEEP_COPY_ALL);

                    if (foundRigGeometry->getStateSet())
                        newRig->setStateSet(foundRigGeometry->getStateSet());

                    if (osg::Group* parent = dynamic_cast<osg::Group*>(foundRigGeometry->getParent(0)))
                    {
                        parent->removeChild(foundRigGeometry);
                        parent->addChild(newRig);
                    }
                }
            }

            if (isColladaFile)
            {
                Resource::ColladaDescriptionVisitor colladaDescriptionVisitor;
                node->accept(colladaDescriptionVisitor);

                if (colladaDescriptionVisitor.mSkeleton)
                {
                    if (osg::Group* group = dynamic_cast<osg::Group*>(node))
                    {
                        group->removeChildren(0, group->getNumChildren());
                        for (osg::ref_ptr<SceneUtil::RigGeometryHolder> newRiggeometryHolder :
                            colladaDescriptionVisitor.mRigGeometryHolders)
                        {
                            osg::ref_ptr<osg::MatrixTransform> backToOriginTrans = new osg::MatrixTransform();

                            newRiggeometryHolder->getOrCreateUserDataContainer()->addUserObject(
                                new TemplateRef(newRiggeometryHolder->getGeometry(0)));
                            backToOriginTrans->getOrCreateUserDataContainer()->addUserObject(
                                new TemplateRef(newRiggeometryHolder->getGeometry(0)));

                            newRiggeometryHolder->setBodyPart(true);

                            for (int i = 0; i < 2; ++i)
                            {
                                if (newRiggeometryHolder->getGeometry(i))
                                    newRiggeometryHolder->getGeometry(i)->setSkeleton(nullptr);
                            }

                            backToOriginTrans->addChild(newRiggeometryHolder);
                            group->addChild(backToOriginTrans);

                            node->getOrCreateUserDataContainer()->addUserObject(
                                new TemplateRef(newRiggeometryHolder->getGeometry(0)));
                        }
                    }
                }

                node->getOrCreateStateSet()->addUniform(new osg::Uniform("emissiveMult", 1.f));
                node->getOrCreateStateSet()->addUniform(new osg::Uniform("specStrength", 1.f));
                node->getOrCreateStateSet()->addUniform(new osg::Uniform("envMapColor", osg::Vec4f(1, 1, 1, 1)));
                node->getOrCreateStateSet()->addUniform(new osg::Uniform("useFalloff", false));
                node->getOrCreateStateSet()->addUniform(new osg::Uniform("distortionStrength", 0.f));
            }

            node->setUserValue(Misc::OsgUserValues::sFileHash,
                std::string(reinterpret_cast<const char*>(fileHash.data()), fileHash.size() * sizeof(std::uint64_t)));

            return node;
        }

        std::vector<std::string> makeSortedReservedNames()
        {
            static constexpr std::string_view names[] = {
                "Head",
                "Neck",
                "Chest",
                "Groin",
                "Right Hand",
                "Left Hand",
                "Right Wrist",
                "Left Wrist",
                "Shield Bone",
                "Right Forearm",
                "Left Forearm",
                "Right Upper Arm",
                "Left Upper Arm",
                "Right Foot",
                "Left Foot",
                "Right Ankle",
                "Left Ankle",
                "Right Knee",
                "Left Knee",
                "Right Upper Leg",
                "Left Upper Leg",
                "Right Clavicle",
                "Left Clavicle",
                "Weapon Bone",
                "Tail",
                "Bip01",
                "Root Bone",
                "BoneOffset",
                "AttachLight",
                "Arrow",
                "Camera",
                "Collision",
                "Right_Wrist",
                "Left_Wrist",
                "Shield_Bone",
                "Right_Forearm",
                "Left_Forearm",
                "Right_Upper_Arm",
                "Left_Clavicle",
                "Weapon_Bone",
                "Root_Bone",
            };

            std::vector<std::string> result;
            result.reserve(2 * std::size(names));

            for (std::string_view name : names)
            {
                result.emplace_back(name);
                std::string prefixedName("Tri ");
                prefixedName += name;
                result.push_back(std::move(prefixedName));
            }

            std::sort(result.begin(), result.end(), Misc::StringUtils::ciLess);

            return result;
        }
    }

    osg::ref_ptr<osg::Node> load(VFS::Path::NormalizedView normalizedFilename, const VFS::Manager* vfs,
        Resource::ImageManager* imageManager, Resource::NifFileManager* nifFileManager,
        Resource::BgsmFileManager* materialMgr)
    {
        const std::string_view ext = Misc::getFileExtension(normalizedFilename.value());
        if (ext == "nif")
            return NifOsg::Loader::load(*nifFileManager->get(normalizedFilename), imageManager, materialMgr);
        else if (ext == "spt")
        {
            Log(Debug::Warning) << "Ignoring SpeedTree data file " << normalizedFilename;
            return new osg::Node();
        }
        else
            return loadNonNif(normalizedFilename, *vfs->get(normalizedFilename), imageManager);
    }

    class CanOptimizeCallback : public SceneUtil::Optimizer::IsOperationPermissibleForObjectCallback
    {
    public:
        bool isReservedName(const std::string& name) const
        {
            if (name.empty())
                return false;

            static const std::vector<std::string> reservedNames = makeSortedReservedNames();

            const auto it = Misc::partialBinarySearch(reservedNames.begin(), reservedNames.end(), name);
            return it != reservedNames.end();
        }

        bool isOperationPermissibleForObjectImplementation(
            const SceneUtil::Optimizer* optimizer, const osg::Drawable* node, unsigned int option) const override
        {
            if (option & SceneUtil::Optimizer::FLATTEN_STATIC_TRANSFORMS)
            {
                if (node->asGeometry() && node->className() == std::string("Geometry"))
                    return true;
                else
                    return false; // ParticleSystem would have to convert space of all the processors, RigGeometry would
                                  // have to convert bones... theoretically possible, but very complicated
            }
            return (option & optimizer->getPermissibleOptimizationsForObject(node)) != 0;
        }

        bool isOperationPermissibleForObjectImplementation(
            const SceneUtil::Optimizer* optimizer, const osg::Node* node, unsigned int option) const override
        {
            if (node->getNumDescriptions() > 0)
                return false;
            if (node->getDataVariance() == osg::Object::DYNAMIC)
                return false;
            if (isReservedName(node->getName()))
                return false;

            return (option & optimizer->getPermissibleOptimizationsForObject(node)) != 0;
        }
    };

    static bool canOptimize(std::string_view filename)
    {
        const std::string_view::size_type slashpos = filename.find_last_of('/');
        if (slashpos != std::string_view::npos && slashpos + 1 < filename.size())
        {
            const std::string_view basename = filename.substr(slashpos + 1);
            // xmesh.nif can not be optimized because there are keyframes added in post
            if (!basename.empty() && basename[0] == 'x')
                return false;

            // NPC skeleton files can not be optimized because of keyframes added in post
            // (most of them are usually named like 'xbase_anim.nif' anyway, but not all of them :( )
            if (basename.starts_with("base_anim") || basename.starts_with("skin"))
                return false;
        }

        // For spell VFX, DummyXX nodes must remain intact. Not adding those to reservedNames to avoid being overly
        // cautious - instead, decide on filename
        if (filename.find("vfx_pattern") != std::string_view::npos)
            return false;
        return true;
    }

    unsigned int getOptimizationOptions()
    {
        using namespace SceneUtil;
        const char* env = getenv("OPENMW_OPTIMIZE");
        unsigned int options
            = Optimizer::FLATTEN_STATIC_TRANSFORMS | Optimizer::REMOVE_REDUNDANT_NODES | Optimizer::MERGE_GEOMETRY;
        if (env)
        {
            std::string str(env);

            if (str.find("OFF") != std::string::npos || str.find('0') != std::string::npos)
                options = 0;

            if (str.find("~FLATTEN_STATIC_TRANSFORMS") != std::string::npos)
                options ^= Optimizer::FLATTEN_STATIC_TRANSFORMS;
            else if (str.find("FLATTEN_STATIC_TRANSFORMS") != std::string::npos)
                options |= Optimizer::FLATTEN_STATIC_TRANSFORMS;

            if (str.find("~REMOVE_REDUNDANT_NODES") != std::string::npos)
                options ^= Optimizer::REMOVE_REDUNDANT_NODES;
            else if (str.find("REMOVE_REDUNDANT_NODES") != std::string::npos)
                options |= Optimizer::REMOVE_REDUNDANT_NODES;

            if (str.find("~MERGE_GEOMETRY") != std::string::npos)
                options ^= Optimizer::MERGE_GEOMETRY;
            else if (str.find("MERGE_GEOMETRY") != std::string::npos)
                options |= Optimizer::MERGE_GEOMETRY;
        }
        return options;
    }

    void SceneManager::shareState(osg::ref_ptr<osg::Node> node)
    {
        mSharedStateMutex.lock();
        mSharedStateManager->share(node.get());
        mSharedStateMutex.unlock();
    }

    osg::ref_ptr<osg::Node> SceneManager::loadErrorMarker()
    {
        try
        {
            VFS::Path::Normalized path("meshes/marker_error.****");
            for (const auto meshType : { "nif", "osg", "osgt", "osgb", "osgx", "osg2", "dae" })
            {
                path.changeExtension(meshType);
                if (mVFS->exists(path))
                    return load(path, mVFS, mImageManager, mNifFileManager, mBgsmFileManager);
            }
        }
        catch (const std::exception& e)
        {
            Log(Debug::Warning) << "Failed to load error marker:" << e.what()
                                << ", using embedded marker_error instead";
        }
        Files::IMemStream file(ErrorMarker::sValue.data(), ErrorMarker::sValue.size());
        constexpr VFS::Path::NormalizedView errorMarker("error_marker.osgt");
        return loadNonNif(errorMarker, file, mImageManager);
    }

    void SceneManager::loadSelectionMarker(
        osg::ref_ptr<osg::Group> parentNode, const char* markerData, long long markerSize) const
    {
        Files::IMemStream file(markerData, markerSize);
        constexpr VFS::Path::NormalizedView selectionMarker("selectionmarker.osgt");
        parentNode->addChild(loadNonNif(selectionMarker, file, mImageManager));
    }

    osg::ref_ptr<osg::Node> SceneManager::cloneErrorMarker()
    {
        std::call_once(mErrorMarkerFlag, [this] { mErrorMarker = loadErrorMarker(); });

        return static_cast<osg::Node*>(mErrorMarker->clone(osg::CopyOp::DEEP_COPY_ALL));
    }

    osg::ref_ptr<const osg::Node> SceneManager::getTemplate(VFS::Path::NormalizedView path, bool compile)
    {
        osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(path);
        if (obj)
            return osg::ref_ptr<const osg::Node>(static_cast<osg::Node*>(obj.get()));
        else
        {
            osg::ref_ptr<osg::Node> loaded;
            try
            {
                loaded = load(path, mVFS, mImageManager, mNifFileManager, mBgsmFileManager);
            }
            catch (const std::exception& e)
            {
                Log(Debug::Error) << "Failed to load '" << path << "': " << e.what() << ", using marker_error instead";
                loaded = cloneErrorMarker();
            }

            // set filtering settings
            SetFilterSettingsVisitor setFilterSettingsVisitor(mMinFilter, mMagFilter, mMaxAnisotropy);
            loaded->accept(setFilterSettingsVisitor);
            SetFilterSettingsControllerVisitor setFilterSettingsControllerVisitor(
                mMinFilter, mMagFilter, mMaxAnisotropy);
            loaded->accept(setFilterSettingsControllerVisitor);

            osg::ref_ptr<Shader::ShaderVisitor> shaderVisitor(createShaderVisitor());
            loaded->accept(*shaderVisitor);

            if (canOptimize(path.value()))
            {
                SceneUtil::Optimizer optimizer;
                optimizer.setSharedStateManager(mSharedStateManager, &mSharedStateMutex);
                optimizer.setIsOperationPermissibleForObjectCallback(new CanOptimizeCallback);

                static const unsigned int options
                    = getOptimizationOptions() | SceneUtil::Optimizer::SHARE_DUPLICATE_STATE;

                optimizer.optimize(loaded, options);
            }
            else
                shareState(loaded);

            if (compile && mIncrementalCompileOperation)
                mIncrementalCompileOperation->add(loaded);
            else
                loaded->getBound();

            mCache->addEntryToObjectCache(path.value(), loaded);
            return loaded;
        }
    }

    osg::ref_ptr<osg::Node> SceneManager::getInstance(VFS::Path::NormalizedView path)
    {
        return getInstance(getTemplate(path));
    }

    osg::ref_ptr<osg::Node> SceneManager::cloneNode(const osg::Node* base)
    {
        SceneUtil::CopyOp copyop;
        if (const osg::Drawable* drawable = base->asDrawable())
        {
            if (drawable->asGeometry())
            {
                Log(Debug::Warning) << "SceneManager::cloneNode: attempting to clone osg::Geometry. For safety reasons "
                                       "this will be expensive. Consider avoiding this call.";
                copyop.setCopyFlags(
                    copyop.getCopyFlags() | osg::CopyOp::DEEP_COPY_ARRAYS | osg::CopyOp::DEEP_COPY_PRIMITIVES);
            }
        }
        osg::ref_ptr<osg::Node> cloned = static_cast<osg::Node*>(base->clone(copyop));
        // add a ref to the original template to help verify the safety of shallow cloning operations
        // in addition, if this node is managed by a cache, we hint to the cache that it's still being used and should
        // be kept in cache
        cloned->getOrCreateUserDataContainer()->addUserObject(new TemplateRef(base));
        return cloned;
    }

    osg::ref_ptr<osg::Node> SceneManager::getInstance(const osg::Node* base)
    {
        osg::ref_ptr<osg::Node> cloned = cloneNode(base);
        // we can skip any scene graphs without update callbacks since we know that particle emitters will have an
        // update callback set
        if (cloned->getNumChildrenRequiringUpdateTraversal() > 0)
        {
            InitParticlesVisitor visitor(mParticleSystemMask);
            cloned->accept(visitor);
        }

        return cloned;
    }

    osg::ref_ptr<osg::Node> SceneManager::getInstance(VFS::Path::NormalizedView path, osg::Group* parentNode)
    {
        osg::ref_ptr<osg::Node> cloned = getInstance(path);
        attachTo(cloned, parentNode);
        return cloned;
    }

    void SceneManager::attachTo(osg::Node* instance, osg::Group* parentNode) const
    {
        parentNode->addChild(instance);
    }

    void SceneManager::releaseGLObjects(osg::State* state)
    {
        mCache->releaseGLObjects(state);

        mShaderManager->releaseGLObjects(state);

        std::lock_guard<std::mutex> lock(mSharedStateMutex);
        mSharedStateManager->releaseGLObjects(state);
    }

    void SceneManager::setIncrementalCompileOperation(osgUtil::IncrementalCompileOperation* ico)
    {
        mIncrementalCompileOperation = ico;
    }

    osgUtil::IncrementalCompileOperation* SceneManager::getIncrementalCompileOperation()
    {
        return mIncrementalCompileOperation.get();
    }

    Resource::ImageManager* SceneManager::getImageManager()
    {
        return mImageManager;
    }

    void SceneManager::setParticleSystemMask(unsigned int mask)
    {
        mParticleSystemMask = mask;
    }

    void SceneManager::setFilterSettings(
        const std::string& magfilter, const std::string& minfilter, const std::string& mipmap, float maxAnisotropy)
    {
        osg::Texture::FilterMode min = osg::Texture::LINEAR;
        osg::Texture::FilterMode mag = osg::Texture::LINEAR;

        if (magfilter == "nearest")
            mag = osg::Texture::NEAREST;
        else if (magfilter != "linear")
            Log(Debug::Warning) << "Warning: Invalid texture mag filter: " << magfilter;

        if (minfilter == "nearest")
            min = osg::Texture::NEAREST;
        else if (minfilter != "linear")
            Log(Debug::Warning) << "Warning: Invalid texture min filter: " << minfilter;

        if (mipmap == "nearest")
        {
            if (min == osg::Texture::NEAREST)
                min = osg::Texture::NEAREST_MIPMAP_NEAREST;
            else if (min == osg::Texture::LINEAR)
                min = osg::Texture::LINEAR_MIPMAP_NEAREST;
        }
        else if (mipmap != "none")
        {
            if (mipmap != "linear")
                Log(Debug::Warning) << "Warning: Invalid texture mipmap: " << mipmap;
            if (min == osg::Texture::NEAREST)
                min = osg::Texture::NEAREST_MIPMAP_LINEAR;
            else if (min == osg::Texture::LINEAR)
                min = osg::Texture::LINEAR_MIPMAP_LINEAR;
        }

        mMinFilter = min;
        mMagFilter = mag;
        mMaxAnisotropy = std::max(1.f, maxAnisotropy);

        SetFilterSettingsControllerVisitor setFilterSettingsControllerVisitor(mMinFilter, mMagFilter, mMaxAnisotropy);
        SetFilterSettingsVisitor setFilterSettingsVisitor(mMinFilter, mMagFilter, mMaxAnisotropy);

        mCache->accept(setFilterSettingsVisitor);
        mCache->accept(setFilterSettingsControllerVisitor);
    }

    void SceneManager::applyFilterSettings(osg::Texture* tex)
    {
        tex->setFilter(osg::Texture::MIN_FILTER, mMinFilter);
        tex->setFilter(osg::Texture::MAG_FILTER, mMagFilter);
        tex->setMaxAnisotropy(mMaxAnisotropy);
    }

    void SceneManager::setUnRefImageDataAfterApply(bool unref)
    {
        mUnRefImageDataAfterApply = unref;
    }

    void SceneManager::updateCache(double referenceTime)
    {
        ResourceManager::updateCache(referenceTime);

        mSharedStateMutex.lock();
        mSharedStateManager->prune();
        mSharedStateMutex.unlock();

        if (mIncrementalCompileOperation)
        {
            std::lock_guard<OpenThreads::Mutex> lock(*mIncrementalCompileOperation->getToCompiledMutex());
            osgUtil::IncrementalCompileOperation::CompileSets& sets = mIncrementalCompileOperation->getToCompile();
            for (osgUtil::IncrementalCompileOperation::CompileSets::iterator it = sets.begin(); it != sets.end();)
            {
                int refcount = (*it)->_subgraphToCompile->referenceCount();
                if ((*it)->_subgraphToCompile->asDrawable())
                    refcount -= 1; // ref by CompileList.
                if (refcount <= 2) // ref by ObjectCache + ref by _subgraphToCompile.
                {
                    // no other ref = not needed anymore.
                    it = sets.erase(it);
                }
                else
                    ++it;
            }
        }
    }

    void SceneManager::clearCache()
    {
        ResourceManager::clearCache();

        std::lock_guard<std::mutex> lock(mSharedStateMutex);
        mSharedStateManager->clearCache();
    }

    void SceneManager::reportStats(unsigned int frameNumber, osg::Stats* stats) const
    {
        if (mIncrementalCompileOperation)
        {
            std::lock_guard<OpenThreads::Mutex> lock(*mIncrementalCompileOperation->getToCompiledMutex());
            stats->setAttribute(
                frameNumber, "Compiling", static_cast<double>(mIncrementalCompileOperation->getToCompile().size()));
        }

        {
            std::lock_guard<std::mutex> lock(mSharedStateMutex);
            stats->setAttribute(
                frameNumber, "Texture", static_cast<double>(mSharedStateManager->getNumSharedTextures()));
            stats->setAttribute(
                frameNumber, "StateSet", static_cast<double>(mSharedStateManager->getNumSharedStateSets()));
        }

        Resource::reportStats("Node", frameNumber, mCache->getStats(), *stats);
    }

    osg::ref_ptr<Shader::ShaderVisitor> SceneManager::createShaderVisitor(const std::string& shaderPrefix)
    {
        osg::ref_ptr<Shader::ShaderVisitor> shaderVisitor(
            new Shader::ShaderVisitor(*mShaderManager.get(), *mImageManager, shaderPrefix));
        shaderVisitor->setForceShaders(mForceShaders);
        shaderVisitor->setAutoUseNormalMaps(mAutoUseNormalMaps);
        shaderVisitor->setNormalMapPattern(mNormalMapPattern);
        shaderVisitor->setNormalHeightMapPattern(mNormalHeightMapPattern);
        shaderVisitor->setAutoUseSpecularMaps(mAutoUseSpecularMaps);
        shaderVisitor->setSpecularMapPattern(mSpecularMapPattern);
        shaderVisitor->setApplyLightingToEnvMaps(mApplyLightingToEnvMaps);
        shaderVisitor->setConvertAlphaTestToAlphaToCoverage(mConvertAlphaTestToAlphaToCoverage);
        shaderVisitor->setAdjustCoverageForAlphaTest(mAdjustCoverageForAlphaTest);
        shaderVisitor->setSupportsNormalsRT(mSupportsNormalsRT);
        shaderVisitor->setWeatherParticleOcclusion(mWeatherParticleOcclusion);
        return shaderVisitor;
    }
}
