#include "scenemanager.hpp"

#include <iostream>
#include <osg/Node>
#include <osg/UserDataContainer>

#include <osgParticle/ParticleSystem>

#include <osgUtil/IncrementalCompileOperation>

#include <osgDB/SharedStateManager>
#include <osgDB/Registry>

#include <components/nifosg/nifloader.hpp>
#include <components/nif/niffile.hpp>

#include <components/vfs/manager.hpp>

#include <components/sceneutil/clone.hpp>
#include <components/sceneutil/util.hpp>
#include <components/sceneutil/controller.hpp>

#include <components/shader/shadervisitor.hpp>
#include <components/shader/shadermanager.hpp>

#include "imagemanager.hpp"
#include "niffilemanager.hpp"
#include "objectcache.hpp"
#include "multiobjectcache.hpp"

namespace
{

    /// @todo Do this in updateCallback so that animations are accounted for.
    class InitWorldSpaceParticlesVisitor : public osg::NodeVisitor
    {
    public:
        /// @param mask The node mask to set on ParticleSystem nodes.
        InitWorldSpaceParticlesVisitor(unsigned int mask)
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

        void apply(osg::Drawable& drw)
        {
            if (osgParticle::ParticleSystem* partsys = dynamic_cast<osgParticle::ParticleSystem*>(&drw))
            {
                if (isWorldSpaceParticleSystem(partsys))
                {
                    // HACK: Ignore the InverseWorldMatrix transform the particle system is attached to
                    if (partsys->getNumParents() && partsys->getParent(0)->getNumParents())
                        transformInitialParticles(partsys, partsys->getParent(0)->getParent(0));
                }
                partsys->setNodeMask(mMask);
            }
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
    private:
        unsigned int mMask;
    };
}

namespace Resource
{

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

        virtual void visit(osg::Node& node, SceneUtil::Controller& ctrl)
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

        virtual void apply(osg::Node& node)
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



    SceneManager::SceneManager(const VFS::Manager *vfs, Resource::ImageManager* imageManager, Resource::NifFileManager* nifFileManager)
        : ResourceManager(vfs)
        , mShaderManager(new Shader::ShaderManager)
        , mForceShaders(false)
        , mClampLighting(true)
        , mForcePerPixelLighting(false)
        , mAutoUseNormalMaps(false)
        , mAutoUseSpecularMaps(false)
        , mInstanceCache(new MultiObjectCache)
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

    void SceneManager::recreateShaders(osg::ref_ptr<osg::Node> node)
    {
        Shader::ShaderVisitor shaderVisitor(*mShaderManager.get(), *mImageManager, "objects_vertex.glsl", "objects_fragment.glsl");
        shaderVisitor.setForceShaders(mForceShaders);
        shaderVisitor.setClampLighting(mClampLighting);
        shaderVisitor.setForcePerPixelLighting(mForcePerPixelLighting);
        shaderVisitor.setAllowedToModifyStateSets(false);
        node->accept(shaderVisitor);
    }

    void SceneManager::setClampLighting(bool clamp)
    {
        mClampLighting = clamp;
    }

    bool SceneManager::getClampLighting() const
    {
        return mClampLighting;
    }

    void SceneManager::setForcePerPixelLighting(bool force)
    {
        mForcePerPixelLighting = force;
    }

    bool SceneManager::getForcePerPixelLighting() const
    {
        return mForcePerPixelLighting;
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

    /// @brief Callback to read image files from the VFS.
    class ImageReadCallback : public osgDB::ReadFileCallback
    {
    public:
        ImageReadCallback(Resource::ImageManager* imageMgr)
            : mImageManager(imageMgr)
        {
        }

        virtual osgDB::ReaderWriter::ReadResult readImage(const std::string& filename, const osgDB::Options* options)
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

    std::string getFileExtension(const std::string& file)
    {
        size_t extPos = file.find_last_of('.');
        if (extPos != std::string::npos && extPos+1 < file.size())
            return file.substr(extPos+1);
        return std::string();
    }

    osg::ref_ptr<osg::Node> load (Files::IStreamPtr file, const std::string& normalizedFilename, Resource::ImageManager* imageManager, Resource::NifFileManager* nifFileManager)
    {
        std::string ext = getFileExtension(normalizedFilename);
        if (ext == "nif")
            return NifOsg::Loader::load(nifFileManager->get(normalizedFilename), imageManager);
        else
        {
            osgDB::ReaderWriter* reader = osgDB::Registry::instance()->getReaderWriterForExtension(ext);
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

            osgDB::ReaderWriter::ReadResult result = reader->readNode(*file, options);
            if (!result.success())
            {
                std::stringstream errormsg;
                errormsg << "Error loading " << normalizedFilename << ": " << result.message() << " code " << result.status() << std::endl;
                throw std::runtime_error(errormsg.str());
            }
            return result.getNode();
        }
    }

    osg::ref_ptr<const osg::Node> SceneManager::getTemplate(const std::string &name)
    {
        std::string normalized = name;
        mVFS->normalizeFilename(normalized);

        osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(normalized);
        if (obj)
            return osg::ref_ptr<const osg::Node>(static_cast<osg::Node*>(obj.get()));
        else
        {
            osg::ref_ptr<osg::Node> loaded;
            try
            {
                Files::IStreamPtr file = mVFS->get(normalized);

                loaded = load(file, normalized, mImageManager, mNifFileManager);
            }
            catch (std::exception& e)
            {
                static const char * const sMeshTypes[] = { "nif", "osg", "osgt", "osgb", "osgx", "osg2" };

                for (unsigned int i=0; i<sizeof(sMeshTypes)/sizeof(sMeshTypes[0]); ++i)
                {
                    normalized = "meshes/marker_error." + std::string(sMeshTypes[i]);
                    if (mVFS->exists(normalized))
                    {
                        std::cerr << "Failed to load '" << name << "': " << e.what() << ", using marker_error." << sMeshTypes[i] << " instead" << std::endl;
                        Files::IStreamPtr file = mVFS->get(normalized);
                        loaded = load(file, normalized, mImageManager, mNifFileManager);
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

            Shader::ShaderVisitor shaderVisitor(*mShaderManager.get(), *mImageManager, "objects_vertex.glsl", "objects_fragment.glsl");
            shaderVisitor.setForceShaders(mForceShaders);
            shaderVisitor.setClampLighting(mClampLighting);
            shaderVisitor.setForcePerPixelLighting(mForcePerPixelLighting);
            shaderVisitor.setAutoUseNormalMaps(mAutoUseNormalMaps);
            shaderVisitor.setNormalMapPattern(mNormalMapPattern);
            shaderVisitor.setNormalHeightMapPattern(mNormalHeightMapPattern);
            shaderVisitor.setAutoUseSpecularMaps(mAutoUseSpecularMaps);
            shaderVisitor.setSpecularMapPattern(mSpecularMapPattern);
            loaded->accept(shaderVisitor);

            // share state
            mSharedStateMutex.lock();
            osgDB::Registry::instance()->getOrCreateSharedStateManager()->share(loaded.get());
            mSharedStateMutex.unlock();

            if (mIncrementalCompileOperation)
                mIncrementalCompileOperation->add(loaded);

            mCache->addEntryToObjectCache(normalized, loaded);
            return loaded;
        }
    }

    osg::ref_ptr<osg::Node> SceneManager::cacheInstance(const std::string &name)
    {
        std::string normalized = name;
        mVFS->normalizeFilename(normalized);

        osg::ref_ptr<osg::Node> node = createInstance(normalized);
        mInstanceCache->addEntryToObjectCache(normalized, node.get());
        return node;
    }

    osg::ref_ptr<osg::Node> SceneManager::createInstance(const std::string& name)
    {
        osg::ref_ptr<const osg::Node> scene = getTemplate(name);
        osg::ref_ptr<osg::Node> cloned = osg::clone(scene.get(), SceneUtil::CopyOp());

        // add a ref to the original template, to hint to the cache that it's still being used and should be kept in cache
        cloned->getOrCreateUserDataContainer()->addUserObject(const_cast<osg::Node*>(scene.get()));

        return cloned;
    }

    osg::ref_ptr<osg::Node> SceneManager::getInstance(const std::string &name)
    {
        std::string normalized = name;
        mVFS->normalizeFilename(normalized);

        osg::ref_ptr<osg::Object> obj = mInstanceCache->takeFromObjectCache(normalized);
        if (obj.get())
            return static_cast<osg::Node*>(obj.get());

        return createInstance(normalized);

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
        notifyAttached(instance);
    }

    void SceneManager::releaseGLObjects(osg::State *state)
    {
        mCache->releaseGLObjects(state);
        mInstanceCache->releaseGLObjects(state);
    }

    void SceneManager::setIncrementalCompileOperation(osgUtil::IncrementalCompileOperation *ico)
    {
        mIncrementalCompileOperation = ico;
    }

    void SceneManager::notifyAttached(osg::Node *node) const
    {
        InitWorldSpaceParticlesVisitor visitor (mParticleSystemMask);
        node->accept(visitor);
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
            std::cerr<< "Invalid texture mag filter: "<<magfilter <<std::endl;

        if(minfilter == "nearest")
            min = osg::Texture::NEAREST;
        else if(minfilter != "linear")
            std::cerr<< "Invalid texture min filter: "<<minfilter <<std::endl;

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
                std::cerr<< "Invalid texture mipmap: "<<mipmap <<std::endl;
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

        mInstanceCache->removeUnreferencedObjectsInCache();
    }

}
