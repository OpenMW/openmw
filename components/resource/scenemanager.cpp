#include "scenemanager.hpp"

#include <cstdlib>

#include <osg/AlphaFunc>
#include <osg/Node>
#include <osg/UserDataContainer>

#include <osgParticle/ParticleSystem>

#include <osgUtil/IncrementalCompileOperation>

#include <osgDB/SharedStateManager>
#include <osgDB/Registry>

#include <components/debug/debuglog.hpp>

#include <components/nifosg/nifloader.hpp>
#include <components/nif/niffile.hpp>

#include <components/misc/pathhelpers.hpp>
#include <components/misc/stringops.hpp>
#include <components/misc/algorithm.hpp>

#include <components/vfs/manager.hpp>

#include <components/sceneutil/clone.hpp>
#include <components/sceneutil/util.hpp>
#include <components/sceneutil/controller.hpp>
#include <components/sceneutil/optimizer.hpp>
#include <components/sceneutil/visitor.hpp>
#include <components/sceneutil/lightmanager.hpp>

#include <components/shader/shadervisitor.hpp>
#include <components/shader/shadermanager.hpp>

#include "imagemanager.hpp"
#include "niffilemanager.hpp"
#include "objectcache.hpp"

namespace
{

    class InitWorldSpaceParticlesCallback : public SceneUtil::NodeCallback<InitWorldSpaceParticlesCallback, osgParticle::ParticleSystem*>
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
            for (int i=0; i<partsys->numParticles(); ++i)
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
            return (partsys->getUserDataContainer()
                    && partsys->getUserDataContainer()->getNumDescriptions() > 0
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
        unsigned int getNumSharedTextures() const
        {
            return _sharedTextureList.size();
        }

        unsigned int getNumSharedStateSets() const
        {
            return _sharedStateSetList.size();
        }

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
        SetFilterSettingsControllerVisitor(osg::Texture::FilterMode minFilter, osg::Texture::FilterMode magFilter, int maxAnisotropy)
            : mMinFilter(minFilter)
            , mMagFilter(magFilter)
            , mMaxAnisotropy(maxAnisotropy)
        {
        }

        void visit(osg::Node& node, SceneUtil::Controller& ctrl) override
        {
            if (NifOsg::FlipController* flipctrl = dynamic_cast<NifOsg::FlipController*>(&ctrl))
            {
                for (std::vector<osg::ref_ptr<osg::Texture2D> >::iterator it = flipctrl->getTextures().begin(); it != flipctrl->getTextures().end(); ++it)
                {
                    osg::Texture* tex = *it;
                    tex->setFilter(osg::Texture::MIN_FILTER, mMinFilter);
                    tex->setFilter(osg::Texture::MAG_FILTER, mMagFilter);
                    tex->setMaxAnisotropy(mMaxAnisotropy);
                }
            }
        }

    private:
        osg::Texture::FilterMode mMinFilter;
        osg::Texture::FilterMode mMagFilter;
        int mMaxAnisotropy;
    };

    /// Set texture filtering settings on textures contained in StateSets.
    class SetFilterSettingsVisitor : public osg::NodeVisitor
    {
    public:
        SetFilterSettingsVisitor(osg::Texture::FilterMode minFilter, osg::Texture::FilterMode magFilter, int maxAnisotropy)
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
            for(unsigned int unit=0;unit<texAttributes.size();++unit)
            {
                osg::StateAttribute *texture = stateset->getTextureAttribute(unit, osg::StateAttribute::TEXTURE);
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
        int mMaxAnisotropy;
    };

    // Check Collada extra descriptions
    class ColladaAlphaTrickVisitor : public osg::NodeVisitor
    {
    public:
        ColladaAlphaTrickVisitor()
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        {
        }

        osg::AlphaFunc::ComparisonFunction getTestMode(std::string mode)
        {
            if (mode == "ALWAYS") return osg::AlphaFunc::ALWAYS;
            if (mode == "LESS") return osg::AlphaFunc::LESS;
            if (mode == "EQUAL") return osg::AlphaFunc::EQUAL;
            if (mode == "LEQUAL") return osg::AlphaFunc::LEQUAL;
            if (mode == "GREATER") return osg::AlphaFunc::GREATER;
            if (mode == "NOTEQUAL") return osg::AlphaFunc::NOTEQUAL;
            if (mode == "GEQUAL") return osg::AlphaFunc::GEQUAL;
            if (mode == "NEVER") return osg::AlphaFunc::NEVER;

            Log(Debug::Warning) << "Unexpected alpha testing mode: " << mode;
            return osg::AlphaFunc::LEQUAL;
        }

        void apply(osg::Node& node) override
        {
            if (osg::StateSet* stateset = node.getStateSet())
            {
                if (stateset->getRenderingHint() == osg::StateSet::TRANSPARENT_BIN)
                {
                    osg::ref_ptr<osg::Depth> depth = SceneUtil::createDepth();
                    depth->setWriteMask(false);

                    stateset->setAttributeAndModes(depth, osg::StateAttribute::ON);
                }
                else if (stateset->getRenderingHint() == osg::StateSet::OPAQUE_BIN)
                {
                    osg::ref_ptr<osg::Depth> depth = SceneUtil::createDepth();
                    depth->setWriteMask(true);

                    stateset->setAttributeAndModes(depth, osg::StateAttribute::ON);
                }
            }

            /* Check if the <node> has <extra type="Node"> <technique profile="OpenSceneGraph"> <Descriptions> <Description>
               correct format for OpenMW: <Description>alphatest mode value MaterialName</Description>
                                      e.g <Description>alphatest GEQUAL 0.8 MyAlphaTestedMaterial</Description> */
            std::vector<std::string> descriptions = node.getDescriptions();
            for (auto description : descriptions)
            {
                mDescriptions.emplace_back(description);
            }

            // Iterate each description, and see if the current node uses the specified material for alpha testing
            if (node.getStateSet())
            {
                for (auto description : mDescriptions)
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
                            osg::ref_ptr<osg::AlphaFunc> alphaFunc (new osg::AlphaFunc(mode, std::stod(descriptionParts.at(2))));
                            node.getStateSet()->setAttributeAndModes(alphaFunc, osg::StateAttribute::ON);
                        }
                    }
                }
            }

            traverse(node);
        }
        private:
            std::vector<std::string> mDescriptions;
    };

    SceneManager::SceneManager(const VFS::Manager *vfs, Resource::ImageManager* imageManager, Resource::NifFileManager* nifFileManager)
        : ResourceManager(vfs)
        , mShaderManager(new Shader::ShaderManager)
        , mForceShaders(false)
        , mClampLighting(true)
        , mAutoUseNormalMaps(false)
        , mAutoUseSpecularMaps(false)
        , mApplyLightingToEnvMaps(false)
        , mLightingMethod(SceneUtil::LightingMethod::FFP)
        , mConvertAlphaTestToAlphaToCoverage(false)
        , mDepthFormat(0)
        , mSharedStateManager(new SharedStateManager)
        , mImageManager(imageManager)
        , mNifFileManager(nifFileManager)
        , mMinFilter(osg::Texture::LINEAR_MIPMAP_LINEAR)
        , mMagFilter(osg::Texture::LINEAR)
        , mMaxAnisotropy(1)
        , mUnRefImageDataAfterApply(false)
        , mParticleSystemMask(~0u)
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

    void SceneManager::recreateShaders(osg::ref_ptr<osg::Node> node, const std::string& shaderPrefix, bool forceShadersForNode, const osg::Program* programTemplate)
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
        osg::ref_ptr<Shader::ReinstateRemovedStateVisitor> reinstateRemovedStateVisitor = new Shader::ReinstateRemovedStateVisitor(false);
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

    void SceneManager::setDepthFormat(GLenum format)
    {
        mDepthFormat = format;
    }

    GLenum SceneManager::getDepthFormat() const
    {
        return mDepthFormat;
    }

    void SceneManager::setAutoUseNormalMaps(bool use)
    {
        mAutoUseNormalMaps = use;
    }

    void SceneManager::setNormalMapPattern(const std::string &pattern)
    {
        mNormalMapPattern = pattern;
    }

    void SceneManager::setNormalHeightMapPattern(const std::string &pattern)
    {
        mNormalHeightMapPattern = pattern;
    }

    void SceneManager::setAutoUseSpecularMaps(bool use)
    {
        mAutoUseSpecularMaps = use;
    }

    void SceneManager::setSpecularMapPattern(const std::string &pattern)
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

    SceneManager::~SceneManager()
    {
        // this has to be defined in the .cpp file as we can't delete incomplete types
    }

    Shader::ShaderManager &SceneManager::getShaderManager()
    {
        return *mShaderManager.get();
    }

    void SceneManager::setShaderPath(const std::string &path)
    {
        mShaderManager->setShaderPath(path);
    }

    bool SceneManager::checkLoaded(const std::string &name, double timeStamp)
    {
        return mCache->checkInObjectCache(mVFS->normalizeFilename(name), timeStamp);
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
            try
            {
                return osgDB::ReaderWriter::ReadResult(mImageManager->getImage(filename), osgDB::ReaderWriter::ReadResult::FILE_LOADED);
            }
            catch (std::exception& e)
            {
                return osgDB::ReaderWriter::ReadResult(e.what());
            }
        }

    private:
        Resource::ImageManager* mImageManager;
    };

    osg::ref_ptr<osg::Node> load (const std::string& normalizedFilename, const VFS::Manager* vfs, Resource::ImageManager* imageManager, Resource::NifFileManager* nifFileManager)
    {
        auto ext = Misc::getFileExtension(normalizedFilename);
        if (ext == "nif")
            return NifOsg::Loader::load(nifFileManager->get(normalizedFilename), imageManager);
        else
        {
            osgDB::ReaderWriter* reader = osgDB::Registry::instance()->getReaderWriterForExtension(std::string(ext));
            if (!reader)
            {
                std::stringstream errormsg;
                errormsg << "Error loading " << normalizedFilename << ": no readerwriter for '" << ext << "' found" << std::endl;
                throw std::runtime_error(errormsg.str());
            }

            osg::ref_ptr<osgDB::Options> options (new osgDB::Options);
            // Set a ReadFileCallback so that image files referenced in the model are read from our virtual file system instead of the osgDB.
            // Note, for some formats (.obj/.mtl) that reference other (non-image) files a findFileCallback would be necessary.
            // but findFileCallback does not support virtual files, so we can't implement it.
            options->setReadFileCallback(new ImageReadCallback(imageManager));
            if (ext == "dae") options->setOptionString("daeUseSequencedTextureUnits");

            osgDB::ReaderWriter::ReadResult result = reader->readNode(*vfs->get(normalizedFilename), options);
            if (!result.success())
            {
                std::stringstream errormsg;
                errormsg << "Error loading " << normalizedFilename << ": " << result.message() << " code " << result.status() << std::endl;
                throw std::runtime_error(errormsg.str());
            }

            // Recognize and hide collision node
            unsigned int hiddenNodeMask = 0;
            SceneUtil::FindByNameVisitor nameFinder("Collision");
            result.getNode()->accept(nameFinder);
            if (nameFinder.mFoundNode)
                nameFinder.mFoundNode->setNodeMask(hiddenNodeMask);

            if (ext == "dae")
            {
                // Collada alpha testing
                Resource::ColladaAlphaTrickVisitor colladaAlphaTrickVisitor;
                result.getNode()->accept(colladaAlphaTrickVisitor);

                result.getNode()->getOrCreateStateSet()->addUniform(new osg::Uniform("emissiveMult", 1.f));
                result.getNode()->getOrCreateStateSet()->addUniform(new osg::Uniform("envMapColor", osg::Vec4f(1,1,1,1)));
                result.getNode()->getOrCreateStateSet()->addUniform(new osg::Uniform("useFalloff", false));
            }


            return result.getNode();
        }
    }

    class CanOptimizeCallback : public SceneUtil::Optimizer::IsOperationPermissibleForObjectCallback
    {
    public:
        bool isReservedName(const std::string& name) const
        {
            if (name.empty())
                return false;

            static std::vector<std::string> reservedNames;
            if (reservedNames.empty())
            {
                const char* reserved[] = {"Head", "Neck", "Chest", "Groin", "Right Hand", "Left Hand", "Right Wrist", "Left Wrist", "Shield Bone", "Right Forearm", "Left Forearm", "Right Upper Arm",
                                          "Left Upper Arm", "Right Foot", "Left Foot", "Right Ankle", "Left Ankle", "Right Knee", "Left Knee", "Right Upper Leg", "Left Upper Leg", "Right Clavicle",
                                          "Left Clavicle", "Weapon Bone", "Tail", "Bip01", "Root Bone", "BoneOffset", "AttachLight", "Arrow", "Camera", "Collision", "Right_Wrist", "Left_Wrist",
                                          "Shield_Bone", "Right_Forearm", "Left_Forearm", "Right_Upper_Arm", "Left_Clavicle", "Weapon_Bone", "Root_Bone"};

                reservedNames = std::vector<std::string>(reserved, reserved + sizeof(reserved)/sizeof(reserved[0]));

                for (unsigned int i=0; i<sizeof(reserved)/sizeof(reserved[0]); ++i)
                    reservedNames.push_back(std::string("Tri ") + reserved[i]);

                std::sort(reservedNames.begin(), reservedNames.end(), Misc::StringUtils::ciLess);
            }

            std::vector<std::string>::iterator it = Misc::partialBinarySearch(reservedNames.begin(), reservedNames.end(), name);
            return it != reservedNames.end();
        }

        bool isOperationPermissibleForObjectImplementation(const SceneUtil::Optimizer* optimizer, const osg::Drawable* node,unsigned int option) const override
        {
            if (option & SceneUtil::Optimizer::FLATTEN_STATIC_TRANSFORMS)
            {
                if (node->asGeometry() && node->className() == std::string("Geometry"))
                    return true;
                else
                    return false; //ParticleSystem would have to convert space of all the processors, RigGeometry would have to convert bones... theoretically possible, but very complicated
            }
            return (option & optimizer->getPermissibleOptimizationsForObject(node))!=0;
        }

        bool isOperationPermissibleForObjectImplementation(const SceneUtil::Optimizer* optimizer, const osg::Node* node,unsigned int option) const override
        {
            if (node->getNumDescriptions()>0) return false;
            if (node->getDataVariance() == osg::Object::DYNAMIC) return false;
            if (isReservedName(node->getName())) return false;

            return (option & optimizer->getPermissibleOptimizationsForObject(node))!=0;
        }
    };

    bool canOptimize(const std::string& filename)
    {
        size_t slashpos = filename.find_last_of("\\/");
        if (slashpos != std::string::npos && slashpos+1 < filename.size())
        {
            std::string basename = filename.substr(slashpos+1);
            // xmesh.nif can not be optimized because there are keyframes added in post
            if (!basename.empty() && basename[0] == 'x')
                return false;

            // NPC skeleton files can not be optimized because of keyframes added in post
            // (most of them are usually named like 'xbase_anim.nif' anyway, but not all of them :( )
            if (basename.compare(0, 9, "base_anim") == 0 || basename.compare(0, 4, "skin") == 0)
                return false;
        }

        // For spell VFX, DummyXX nodes must remain intact. Not adding those to reservedNames to avoid being overly cautious - instead, decide on filename
        if (filename.find("vfx_pattern") != std::string::npos)
            return false;
        return true;
    }

    unsigned int getOptimizationOptions()
    {
        using namespace SceneUtil;
        const char* env = getenv("OPENMW_OPTIMIZE");
        unsigned int options = Optimizer::FLATTEN_STATIC_TRANSFORMS|Optimizer::REMOVE_REDUNDANT_NODES|Optimizer::MERGE_GEOMETRY;
        if (env)
        {
            std::string str(env);

            if(str.find("OFF")!=std::string::npos || str.find('0')!= std::string::npos) options = 0;

            if(str.find("~FLATTEN_STATIC_TRANSFORMS")!=std::string::npos) options ^= Optimizer::FLATTEN_STATIC_TRANSFORMS;
            else if(str.find("FLATTEN_STATIC_TRANSFORMS")!=std::string::npos) options |= Optimizer::FLATTEN_STATIC_TRANSFORMS;

            if(str.find("~REMOVE_REDUNDANT_NODES")!=std::string::npos) options ^= Optimizer::REMOVE_REDUNDANT_NODES;
            else if(str.find("REMOVE_REDUNDANT_NODES")!=std::string::npos) options |= Optimizer::REMOVE_REDUNDANT_NODES;

            if(str.find("~MERGE_GEOMETRY")!=std::string::npos) options ^= Optimizer::MERGE_GEOMETRY;
            else if(str.find("MERGE_GEOMETRY")!=std::string::npos) options |= Optimizer::MERGE_GEOMETRY;
        }
        return options;
    }

    void SceneManager::shareState(osg::ref_ptr<osg::Node> node) {
        mSharedStateMutex.lock();
        mSharedStateManager->share(node.get());
        mSharedStateMutex.unlock();
    }

    osg::ref_ptr<const osg::Node> SceneManager::getTemplate(const std::string &name, bool compile)
    {
        std::string normalized = mVFS->normalizeFilename(name);

        osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(normalized);
        if (obj)
            return osg::ref_ptr<const osg::Node>(static_cast<osg::Node*>(obj.get()));
        else
        {
            osg::ref_ptr<osg::Node> loaded;
            try
            {
                loaded = load(normalized, mVFS, mImageManager, mNifFileManager);
            }
            catch (std::exception& e)
            {
                static const char * const sMeshTypes[] = { "nif", "osg", "osgt", "osgb", "osgx", "osg2", "dae" };

                for (unsigned int i=0; i<sizeof(sMeshTypes)/sizeof(sMeshTypes[0]); ++i)
                {
                    normalized = "meshes/marker_error." + std::string(sMeshTypes[i]);
                    if (mVFS->exists(normalized))
                    {
                        Log(Debug::Error) << "Failed to load '" << name << "': " << e.what() << ", using marker_error." << sMeshTypes[i] << " instead";
                        loaded = load(normalized, mVFS, mImageManager, mNifFileManager);
                        break;
                    }
                }

                if (!loaded)
                    throw;
            }

            // set filtering settings
            SetFilterSettingsVisitor setFilterSettingsVisitor(mMinFilter, mMagFilter, mMaxAnisotropy);
            loaded->accept(setFilterSettingsVisitor);
            SetFilterSettingsControllerVisitor setFilterSettingsControllerVisitor(mMinFilter, mMagFilter, mMaxAnisotropy);
            loaded->accept(setFilterSettingsControllerVisitor);

            osg::ref_ptr<Shader::ShaderVisitor> shaderVisitor (createShaderVisitor());
            loaded->accept(*shaderVisitor);

            if (canOptimize(normalized))
            {
                SceneUtil::Optimizer optimizer;
                optimizer.setSharedStateManager(mSharedStateManager, &mSharedStateMutex);
                optimizer.setIsOperationPermissibleForObjectCallback(new CanOptimizeCallback);

                static const unsigned int options = getOptimizationOptions()|SceneUtil::Optimizer::SHARE_DUPLICATE_STATE;

                optimizer.optimize(loaded, options);
            }
            else
                shareState(loaded);

            if (compile && mIncrementalCompileOperation)
                mIncrementalCompileOperation->add(loaded);
            else
                loaded->getBound();

            mCache->addEntryToObjectCache(normalized, loaded);
            return loaded;
        }
    }

    osg::ref_ptr<osg::Node> SceneManager::getInstance(const std::string& name)
    {
        osg::ref_ptr<const osg::Node> scene = getTemplate(name);
        return getInstance(scene);
    }

    osg::ref_ptr<osg::Node> SceneManager::cloneNode(const osg::Node* base)
    {
        SceneUtil::CopyOp copyop;
        if (const osg::Drawable* drawable = base->asDrawable())
        {
            if (drawable->asGeometry())
            {
                Log(Debug::Warning) << "SceneManager::cloneNode: attempting to clone osg::Geometry. For safety reasons this will be expensive. Consider avoiding this call.";
                copyop.setCopyFlags(copyop.getCopyFlags()|osg::CopyOp::DEEP_COPY_ARRAYS|osg::CopyOp::DEEP_COPY_PRIMITIVES);
            }
        }
        osg::ref_ptr<osg::Node> cloned = static_cast<osg::Node*>(base->clone(copyop));
        // add a ref to the original template to help verify the safety of shallow cloning operations
        // in addition, if this node is managed by a cache, we hint to the cache that it's still being used and should be kept in cache
        cloned->getOrCreateUserDataContainer()->addUserObject(new TemplateRef(base));
        return cloned;
    }

    osg::ref_ptr<osg::Node> SceneManager::getInstance(const osg::Node *base)
    {
        osg::ref_ptr<osg::Node> cloned = cloneNode(base);
        // we can skip any scene graphs without update callbacks since we know that particle emitters will have an update callback set
        if (cloned->getNumChildrenRequiringUpdateTraversal() > 0)
        {
            InitParticlesVisitor visitor (mParticleSystemMask);
            cloned->accept(visitor);
        }

        return cloned;
    }

    osg::ref_ptr<osg::Node> SceneManager::getInstance(const std::string &name, osg::Group* parentNode)
    {
        osg::ref_ptr<osg::Node> cloned = getInstance(name);
        attachTo(cloned, parentNode);
        return cloned;
    }

    void SceneManager::attachTo(osg::Node *instance, osg::Group *parentNode) const
    {
        parentNode->addChild(instance);
    }

    void SceneManager::releaseGLObjects(osg::State *state)
    {
        mCache->releaseGLObjects(state);

        mShaderManager->releaseGLObjects(state);

        std::lock_guard<std::mutex> lock(mSharedStateMutex);
        mSharedStateManager->releaseGLObjects(state);
    }

    void SceneManager::setIncrementalCompileOperation(osgUtil::IncrementalCompileOperation *ico)
    {
        mIncrementalCompileOperation = ico;
    }

    osgUtil::IncrementalCompileOperation *SceneManager::getIncrementalCompileOperation()
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

    void SceneManager::setFilterSettings(const std::string &magfilter, const std::string &minfilter,
                                           const std::string &mipmap, int maxAnisotropy)
    {
        osg::Texture::FilterMode min = osg::Texture::LINEAR;
        osg::Texture::FilterMode mag = osg::Texture::LINEAR;

        if(magfilter == "nearest")
            mag = osg::Texture::NEAREST;
        else if(magfilter != "linear")
            Log(Debug::Warning) << "Warning: Invalid texture mag filter: "<< magfilter;

        if(minfilter == "nearest")
            min = osg::Texture::NEAREST;
        else if(minfilter != "linear")
            Log(Debug::Warning) << "Warning: Invalid texture min filter: "<< minfilter;

        if(mipmap == "nearest")
        {
            if(min == osg::Texture::NEAREST)
                min = osg::Texture::NEAREST_MIPMAP_NEAREST;
            else if(min == osg::Texture::LINEAR)
                min = osg::Texture::LINEAR_MIPMAP_NEAREST;
        }
        else if(mipmap != "none")
        {
            if(mipmap != "linear")
                Log(Debug::Warning) << "Warning: Invalid texture mipmap: " << mipmap;
            if(min == osg::Texture::NEAREST)
                min = osg::Texture::NEAREST_MIPMAP_LINEAR;
            else if(min == osg::Texture::LINEAR)
                min = osg::Texture::LINEAR_MIPMAP_LINEAR;
        }

        mMinFilter = min;
        mMagFilter = mag;
        mMaxAnisotropy = std::max(1, maxAnisotropy);

        SetFilterSettingsControllerVisitor setFilterSettingsControllerVisitor (mMinFilter, mMagFilter, mMaxAnisotropy);
        SetFilterSettingsVisitor setFilterSettingsVisitor (mMinFilter, mMagFilter, mMaxAnisotropy);

        mCache->accept(setFilterSettingsVisitor);
        mCache->accept(setFilterSettingsControllerVisitor);
    }

    void SceneManager::applyFilterSettings(osg::Texture *tex)
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
            for(osgUtil::IncrementalCompileOperation::CompileSets::iterator it = sets.begin(); it != sets.end();)
            {
                int refcount = (*it)->_subgraphToCompile->referenceCount();
                if ((*it)->_subgraphToCompile->asDrawable()) refcount -= 1; // ref by CompileList.
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

    void SceneManager::reportStats(unsigned int frameNumber, osg::Stats *stats) const
    {
        if (mIncrementalCompileOperation)
        {
            std::lock_guard<OpenThreads::Mutex> lock(*mIncrementalCompileOperation->getToCompiledMutex());
            stats->setAttribute(frameNumber, "Compiling", mIncrementalCompileOperation->getToCompile().size());
        }

        {
            std::lock_guard<std::mutex> lock(mSharedStateMutex);
            stats->setAttribute(frameNumber, "Texture", mSharedStateManager->getNumSharedTextures());
            stats->setAttribute(frameNumber, "StateSet", mSharedStateManager->getNumSharedStateSets());
        }

        stats->setAttribute(frameNumber, "Node", mCache->getCacheSize());
    }

    Shader::ShaderVisitor *SceneManager::createShaderVisitor(const std::string& shaderPrefix)
    {
        Shader::ShaderVisitor* shaderVisitor = new Shader::ShaderVisitor(*mShaderManager.get(), *mImageManager, shaderPrefix);
        shaderVisitor->setForceShaders(mForceShaders);
        shaderVisitor->setAutoUseNormalMaps(mAutoUseNormalMaps);
        shaderVisitor->setNormalMapPattern(mNormalMapPattern);
        shaderVisitor->setNormalHeightMapPattern(mNormalHeightMapPattern);
        shaderVisitor->setAutoUseSpecularMaps(mAutoUseSpecularMaps);
        shaderVisitor->setSpecularMapPattern(mSpecularMapPattern);
        shaderVisitor->setApplyLightingToEnvMaps(mApplyLightingToEnvMaps);
        shaderVisitor->setConvertAlphaTestToAlphaToCoverage(mConvertAlphaTestToAlphaToCoverage);
        return shaderVisitor;
    }
}
