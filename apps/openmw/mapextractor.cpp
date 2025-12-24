#include "mapextractor.hpp"

#include <fstream>
#include <iomanip>

#include <osg/Group>
#include <osg/Image>
#include <osg/Texture2D>
#include <osgDB/WriteFile>

#include <components/debug/debuglog.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/misc/constants.hpp>
#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/workqueue.hpp>
#include <components/settings/values.hpp>

#include "mwbase/environment.hpp"
#include "mwbase/world.hpp"
#include "mwrender/globalmap.hpp"
#include "mwrender/localmap.hpp"
#include "mwrender/renderingmanager.hpp"
#include "mwworld/cellstore.hpp"
#include "mwworld/esmstore.hpp"
#include "mwworld/worldimp.hpp"

namespace OMW
{
    MapExtractor::MapExtractor(
        MWWorld::World& world, const std::string& worldMapOutput, const std::string& localMapOutput)
        : mWorld(world)
        , mWorldMapOutputDir(worldMapOutput)
        , mLocalMapOutputDir(localMapOutput)
    {
        std::filesystem::create_directories(mWorldMapOutputDir);
        std::filesystem::create_directories(mLocalMapOutputDir);

        // Create GlobalMap and LocalMap instances
        MWRender::RenderingManager* renderingManager = mWorld.getRenderingManager();
        if (renderingManager)
        {
            osg::Group* root = renderingManager->getLightRoot()->getParent(0)->asGroup();
            SceneUtil::WorkQueue* workQueue = renderingManager->getWorkQueue();

            mGlobalMap = std::make_unique<MWRender::GlobalMap>(root, workQueue);
            mLocalMap = std::make_unique<MWRender::LocalMap>(root);
        }
    }

    MapExtractor::~MapExtractor() = default;

    void MapExtractor::extractWorldMap()
    {
        Log(Debug::Info) << "Extracting world map...";

        if (!mGlobalMap)
        {
            Log(Debug::Error) << "Global map not initialized";
            return;
        }

        // Temporarily set cell size to 32 pixels for extraction
        const int originalCellSize = Settings::map().mGlobalMapCellSize;
        Settings::map().mGlobalMapCellSize.set(32);

        mGlobalMap->render();
        mGlobalMap->ensureLoaded();

        saveWorldMapTexture();
        saveWorldMapInfo();

        // Restore original cell size
        Settings::map().mGlobalMapCellSize.set(originalCellSize);

        Log(Debug::Info) << "World map extraction complete";
    }

    void MapExtractor::saveWorldMapTexture()
    {
        osg::ref_ptr<osg::Texture2D> baseTexture = mGlobalMap->getBaseTexture();
        if (!baseTexture || !baseTexture->getImage())
        {
            Log(Debug::Error) << "Failed to get world map base texture";
            return;
        }

        osg::Image* image = baseTexture->getImage();
        std::filesystem::path outputPath = mWorldMapOutputDir / "map.png";

        if (!osgDB::writeImageFile(*image, outputPath.string()))
        {
            Log(Debug::Error) << "Failed to write world map texture to " << outputPath;
            return;
        }

        Log(Debug::Info) << "Saved world map texture: " << outputPath;
    }

    void MapExtractor::saveWorldMapInfo()
    {
        int width = mGlobalMap->getWidth();
        int height = mGlobalMap->getHeight();

        const MWWorld::ESMStore& store = mWorld.getStore();
        int minX = std::numeric_limits<int>::max();
        int maxX = std::numeric_limits<int>::min();
        int minY = std::numeric_limits<int>::max();
        int maxY = std::numeric_limits<int>::min();

        MWWorld::Store<ESM::Cell>::iterator it = store.get<ESM::Cell>().extBegin();
        for (; it != store.get<ESM::Cell>().extEnd(); ++it)
        {
            if (it->getGridX() < minX)
                minX = it->getGridX();
            if (it->getGridX() > maxX)
                maxX = it->getGridX();
            if (it->getGridY() < minY)
                minY = it->getGridY();
            if (it->getGridY() > maxY)
                maxY = it->getGridY();
        }

        std::filesystem::path infoPath = mWorldMapOutputDir / "mapInfo.yaml";
        std::ofstream file(infoPath);

        if (!file)
        {
            Log(Debug::Error) << "Failed to create world map info file: " << infoPath;
            return;
        }

        file << "width: " << width << "\n";
        file << "height: " << height << "\n";
        file << "pixelsPerCell: 32\n";
        file << "gridX:\n";
        file << "  min: " << minX << "\n";
        file << "  max: " << maxX << "\n";
        file << "gridY:\n";
        file << "  min: " << minY << "\n";
        file << "  max: " << maxY << "\n";
        file << "file: \"map.png\"\n";

        file.close();

        Log(Debug::Info) << "Saved world map info: " << infoPath;
    }

    void MapExtractor::extractLocalMaps()
    {
        Log(Debug::Info) << "Extracting local maps...";

        saveLocalMapTextures();

        Log(Debug::Info) << "Local map extraction complete";
    }

    void MapExtractor::saveLocalMapTextures()
    {
        if (!mLocalMap)
        {
            Log(Debug::Error) << "Local map not initialized";
            return;
        }

        const MWWorld::ESMStore& store = mWorld.getStore();
        int count = 0;

        MWWorld::Store<ESM::Cell>::iterator it = store.get<ESM::Cell>().extBegin();
        for (; it != store.get<ESM::Cell>().extEnd(); ++it)
        {
            int x = it->getGridX();
            int y = it->getGridY();

            osg::ref_ptr<osg::Texture2D> texture = mLocalMap->getMapTexture(x, y);
            if (!texture || !texture->getImage())
                continue;

            std::ostringstream filename;
            filename << "(" << x << "," << y << ").png";
            std::filesystem::path outputPath = mLocalMapOutputDir / filename.str();

            if (osgDB::writeImageFile(*texture->getImage(), outputPath.string()))
            {
                count++;
                if (count % 100 == 0)
                    Log(Debug::Info) << "Saved " << count << " local map textures...";
            }
        }

        Log(Debug::Info) << "Saved " << count << " exterior local map textures";
    }
}
