#ifndef OPENMW_APPS_OPENMW_MAPEXTRACTOR_HPP
#define OPENMW_APPS_OPENMW_MAPEXTRACTOR_HPP

#include <filesystem>
#include <memory>
#include <string>

#include <components/esm/refid.hpp>

namespace osg
{
    class Group;
}

namespace osgViewer
{
    class Viewer;
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

namespace MWBase
{
    class WindowManager;
}

namespace OMW
{
    class MapExtractor
    {
    public:
        MapExtractor(MWWorld::World& world, osgViewer::Viewer* viewer, MWBase::WindowManager* windowManager,
                     const std::string& worldMapOutput, const std::string& localMapOutput);
        ~MapExtractor();

        void extractWorldMap();
        void extractLocalMaps();

    private:
        MWWorld::World& mWorld;
        osgViewer::Viewer* mViewer;
        MWBase::WindowManager* mWindowManager;
        std::filesystem::path mWorldMapOutputDir;
        std::filesystem::path mLocalMapOutputDir;

        std::unique_ptr<MWRender::GlobalMap> mGlobalMap;
        MWRender::LocalMap* mLocalMap;

        void saveWorldMapTexture();
        void saveWorldMapInfo();
        
        void extractExteriorLocalMaps();
        void extractInteriorLocalMaps();
        void saveInteriorCellTextures(const ESM::RefId& cellId, const std::string& cellName);
        void saveInteriorMapInfo(const ESM::RefId& cellId, const std::string& lowerCaseId, 
                                 int segmentsX, int segmentsY);
    };
}

#endif
