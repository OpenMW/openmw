#ifndef OPENMW_COMPONENTS_TERRAIN_TEXTUREMANAGER_H
#define OPENMW_COMPONENTS_TERRAIN_TEXTUREMANAGER_H

#include <components/resource/resourcemanager.hpp>
#include <components/vfs/pathutil.hpp>

namespace Resource
{
    class SceneManager;
}

namespace osg
{
    class Texture2D;
}

namespace Terrain
{

    class TextureManager : public Resource::ResourceManager
    {
    public:
        explicit TextureManager(Resource::SceneManager* sceneMgr, double expiryDelay);

        void updateTextureFiltering();

        osg::ref_ptr<osg::Texture2D> getTexture(VFS::Path::NormalizedView name);

        void reportStats(unsigned int frameNumber, osg::Stats* stats) const override;

    private:
        Resource::SceneManager* mSceneManager;
    };

}

#endif
