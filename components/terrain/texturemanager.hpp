#ifndef OPENMW_COMPONENTS_TERRAIN_TEXTUREMANAGER_H
#define OPENMW_COMPONENTS_TERRAIN_TEXTUREMANAGER_H

#include <string>

#include <components/resource/resourcemanager.hpp>

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
        TextureManager(Resource::SceneManager* sceneMgr);

        void updateTextureFiltering();

        osg::ref_ptr<osg::Texture2D> getTexture(const std::string& name);

        void reportStats(unsigned int frameNumber, osg::Stats* stats) const override;

    private:
        Resource::SceneManager* mSceneManager;

    };

}

#endif
