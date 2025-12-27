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

namespace OMW
{
    class MapExtractor
    {
    public:
        MapExtractor(MWWorld::World& world, osgViewer::Viewer* viewer, const std::string& worldMapOutput, 
                     const std::string& localMapOutput);
        ~MapExtractor();

        void extractWorldMap();
        void extractLocalMaps();

    private:
        MWWorld::World& mWorld;
        osgViewer::Viewer* mViewer;
        std::filesystem::path mWorldMapOutputDir;
        std::filesystem::path mLocalMapOutputDir;

        std::unique_ptr<MWRender::GlobalMap> mGlobalMap;
        std::unique_ptr<MWRender::LocalMap> mLocalMap;

        void saveWorldMapTexture();
        void saveWorldMapInfo();
        
        void setupExtractionMode();
        void restoreNormalMode();
        void extractExteriorLocalMaps();
        void extractInteriorLocalMaps();
        void loadCellAndWait(int x, int y);
        void loadInteriorCellAndWait(const std::string& cellName);
        void saveInteriorCellTextures(const ESM::RefId& cellId, const std::string& cellName);
        void saveInteriorMapInfo(const ESM::RefId& cellId, const std::string& lowerCaseId, 
                                 int segmentsX, int segmentsY);
    };
}

#endif
