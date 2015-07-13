#include "scenemanager.hpp"

#include <osg/Node>
#include <osg/Geode>
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

#if OPENGLES
#include <components/shadergen/GLES2ShaderGenVisitor.h>
#endif

namespace
{

    class InitWorldSpaceParticlesVisitor : public osg::NodeVisitor
    {
    public:
        InitWorldSpaceParticlesVisitor()
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        {
        }

        void apply(osg::Node& node)
        {
            if (osg::Geode* geode = node.asGeode())
            {
                for (unsigned int i=0;i<geode->getNumDrawables();++i)
                {
                    if (osgParticle::ParticleSystem* partsys = dynamic_cast<osgParticle::ParticleSystem*>(geode->getDrawable(i)))
                    {
                        // HACK: ParticleSystem has no getReferenceFrame()
                        if (partsys->getUserDataContainer()
                                && partsys->getUserDataContainer()->getNumDescriptions() > 0
                                && partsys->getUserDataContainer()->getDescriptions()[0] == "worldspace")
                        {
                            // HACK: Ignore the InverseWorldMatrix transform the geode is attached to
                            if (geode->getNumParents() && geode->getParent(0)->getNumParents())
                                transformInitialParticles(partsys, geode->getParent(0)->getParent(0));
                        }
                        geode->setNodeMask((1<<10)); //MWRender::Mask_ParticleSystem
                    }
                }
            }

            traverse(node);
        }

        void transformInitialParticles(osgParticle::ParticleSystem* partsys, osg::Node* node)
        {
            osg::MatrixList mats = node->getWorldMatrices();
            if (mats.empty())
                return;
            osg::Matrix worldMat = mats[0];
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

}

namespace Resource
{

    SceneManager::SceneManager(const VFS::Manager *vfs, Resource::TextureManager* textureManager)
        : mVFS(vfs)
        , mTextureManager(textureManager)
    {
    }

    SceneManager::~SceneManager()
    {
        // this has to be defined in the .cpp file as we can't delete incomplete types
    }

    osg::ref_ptr<const osg::Node> SceneManager::getTemplate(const std::string &name)
    {
        std::string normalized = name;
        mVFS->normalizeFilename(normalized);

        Index::iterator it = mIndex.find(normalized);
        if (it == mIndex.end())
        {
            Files::IStreamPtr file = mVFS->get(normalized);

            // TODO: add support for non-NIF formats

            osg::ref_ptr<osg::Node> loaded = NifOsg::Loader::load(Nif::NIFFilePtr(new Nif::NIFFile(file, normalized)), mTextureManager);

            osgDB::Registry::instance()->getOrCreateSharedStateManager()->share(loaded.get());
            // TODO: run SharedStateManager::prune on unload

            if (mIncrementalCompileOperation)
                mIncrementalCompileOperation->add(loaded);

            mIndex[normalized] = loaded;
#if OPENGLES
            osgUtil::GLES2ShaderGenVisitor shaderGen;
            loaded->accept(shaderGen);
#endif
            return loaded;
        }
        else{

            return it->second;
}
    }

    osg::ref_ptr<osg::Node> SceneManager::createInstance(const std::string &name)
    {
        osg::ref_ptr<const osg::Node> scene = getTemplate(name);
        osg::ref_ptr<osg::Node> cloned = osg::clone(scene.get(), SceneUtil::CopyOp());
        return cloned;
    }

    osg::ref_ptr<osg::Node> SceneManager::createInstance(const std::string &name, osg::Group* parentNode)
    {
        osg::ref_ptr<osg::Node> cloned = createInstance(name);
        attachTo(cloned, parentNode);
        return cloned;
    }

    osg::ref_ptr<const NifOsg::KeyframeHolder> SceneManager::getKeyframes(const std::string &name)
    {
        std::string normalized = name;
        mVFS->normalizeFilename(normalized);

        KeyframeIndex::iterator it = mKeyframeIndex.find(normalized);
        if (it == mKeyframeIndex.end())
        {
            Files::IStreamPtr file = mVFS->get(normalized);

            osg::ref_ptr<NifOsg::KeyframeHolder> loaded (new NifOsg::KeyframeHolder);
            NifOsg::Loader::loadKf(Nif::NIFFilePtr(new Nif::NIFFile(file, normalized)), *loaded.get());

            mKeyframeIndex[normalized] = loaded;
            return loaded;
        }
        else
            return it->second;
    }

    void SceneManager::attachTo(osg::Node *instance, osg::Group *parentNode) const
    {
        parentNode->addChild(instance);
        notifyAttached(instance);
    }

    void SceneManager::releaseGLObjects(osg::State *state)
    {
        for (Index::iterator it = mIndex.begin(); it != mIndex.end(); ++it)
        {
            it->second->releaseGLObjects(state);
        }
    }

    void SceneManager::setIncrementalCompileOperation(osgUtil::IncrementalCompileOperation *ico)
    {
        mIncrementalCompileOperation = ico;
    }

    void SceneManager::notifyAttached(osg::Node *node) const
    {
        InitWorldSpaceParticlesVisitor visitor;
        node->accept(visitor);
    }

    const VFS::Manager* SceneManager::getVFS() const
    {
        return mVFS;
    }

    Resource::TextureManager* SceneManager::getTextureManager()
    {
        return mTextureManager;
    }

}
