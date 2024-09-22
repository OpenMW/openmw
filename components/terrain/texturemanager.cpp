#include "texturemanager.hpp"

#include <osg/Texture2D>

#include <components/resource/imagemanager.hpp>
#include <components/resource/objectcache.hpp>
#include <components/resource/scenemanager.hpp>

namespace Terrain
{

    TextureManager::TextureManager(Resource::SceneManager* sceneMgr, double expiryDelay)
        : ResourceManager(sceneMgr->getVFS(), expiryDelay)
        , mSceneManager(sceneMgr)
    {
    }

    struct UpdateTextureFilteringFunctor
    {
        UpdateTextureFilteringFunctor(Resource::SceneManager* sceneMgr)
            : mSceneManager(sceneMgr)
        {
        }
        Resource::SceneManager* mSceneManager;

        void operator()(std::string, osg::Object* obj)
        {
            mSceneManager->applyFilterSettings(static_cast<osg::Texture2D*>(obj));
        }
    };

    void TextureManager::updateTextureFiltering()
    {
        UpdateTextureFilteringFunctor f(mSceneManager);
        mCache->call(f);
    }

    osg::ref_ptr<osg::Texture2D> TextureManager::getTexture(VFS::Path::NormalizedView name)
    {
        // don't bother with case folding, since there is only one way of referring to terrain textures we can assume
        // the case is always the same
        osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(name);

        if (obj != nullptr)
            return static_cast<osg::Texture2D*>(obj.get());

        osg::ref_ptr<osg::Texture2D> texture(new osg::Texture2D(mSceneManager->getImageManager()->getImage(name)));
        texture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        texture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        mSceneManager->applyFilterSettings(texture);
        mCache->addEntryToObjectCache(name.value(), texture.get());
        return texture;
    }

    void TextureManager::reportStats(unsigned int frameNumber, osg::Stats* stats) const
    {
        Resource::reportStats("Terrain Texture", frameNumber, mCache->getStats(), *stats);
    }

}
