#ifndef OPENMW_APPS_OPENMW_MAPEXTRACTOR_HPP
#define OPENMW_APPS_OPENMW_MAPEXTRACTOR_HPP

#include <filesystem>
#include <memory>
#include <string>

namespace osg
{
    class Group;
}

namespace SceneUtil
{
    class WorkQueue;
}

namespace MWRender
{
    class GlobalMap;
    class LocalMap;
}

namespace MWWorld
{
    class World;
}

namespace OMW
{
    class MapExtractor
    {
    public:
        MapExtractor(MWWorld::World& world, const std::string& worldMapOutput, const std::string& localMapOutput);
        ~MapExtractor();

        void extractWorldMap();
        void extractLocalMaps();

    private:
        MWWorld::World& mWorld;
        std::filesystem::path mWorldMapOutputDir;
        std::filesystem::path mLocalMapOutputDir;

        std::unique_ptr<MWRender::GlobalMap> mGlobalMap;
        std::unique_ptr<MWRender::LocalMap> mLocalMap;

        void saveWorldMapTexture();
        void saveWorldMapInfo();
        void saveLocalMapTextures();
    };
}

#endif
