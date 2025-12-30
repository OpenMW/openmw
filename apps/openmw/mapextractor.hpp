#ifndef OPENMW_APPS_OPENMW_MAPEXTRACTOR_HPP
#define OPENMW_APPS_OPENMW_MAPEXTRACTOR_HPP

#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <functional>

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
    class RenderingManager;
}

namespace MWWorld
{
    class World;
    class CellStore;
    class ESMStore;
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
        MapExtractor(const std::string& worldMapOutput, const std::string& localMapOutput, bool forceOverwrite,
                     MWRender::RenderingManager* renderingManager, const MWWorld::ESMStore* store);
        ~MapExtractor();

        void setLocalMap(MWRender::LocalMap* localMap) { mLocalMap = localMap; }

        void extractWorldMap();
        void extractLocalMaps(const std::vector<const MWWorld::CellStore*>& activeCells);
        
        // Called every frame to process pending extractions
        void update();
        
        // Check if extraction is complete
        bool isExtractionComplete() const;

    private:
        struct PendingExtraction
        {
            const MWWorld::CellStore* cellStore;
            bool isExterior;
            std::filesystem::path outputPath;
            int framesWaited;
            std::function<void()> completionCallback;
        };

        std::filesystem::path mWorldMapOutputDir;
        std::filesystem::path mLocalMapOutputDir;
        MWRender::RenderingManager* mRenderingManager;
        const MWWorld::ESMStore* mStore;

        std::unique_ptr<MWRender::GlobalMap> mGlobalMap;
        MWRender::LocalMap* mLocalMap;
        
        std::vector<PendingExtraction> mPendingExtractions;
        int mFramesToWait;
        bool mForceOverwrite;

        void saveWorldMapTexture();
        void saveWorldMapInfo();
        
        void startExtraction(const std::vector<const MWWorld::CellStore*>& activeCells);
        void processNextCell();
        bool savePendingExtraction(const PendingExtraction& extraction);
        
        bool extractExteriorCell(const MWWorld::CellStore* cellStore, bool forceOverwrite);
        bool extractInteriorCell(const MWWorld::CellStore* cellStore, bool forceOverwrite);
        void saveInteriorCellTextures(const ESM::RefId& cellId, const std::string& cellName);
        void saveInteriorMapInfo(const ESM::RefId& cellId, const std::string& lowerCaseId, 
                                 int segmentsX, int segmentsY);
    };
}

#endif
