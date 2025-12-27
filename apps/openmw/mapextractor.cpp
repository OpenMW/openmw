#include "mapextractor.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <thread>

#include <osg/ComputeBoundsVisitor>
#include <osg/Group>
#include <osg/Image>
#include <osg/Texture2D>
#include <osgDB/WriteFile>
#include <osgViewer/Viewer>

#include <components/debug/debuglog.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/misc/constants.hpp>
#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/workqueue.hpp>
#include <components/settings/values.hpp>

#include "mwbase/environment.hpp"
#include "mwbase/mechanicsmanager.hpp"
#include "mwbase/world.hpp"
#include "mwrender/globalmap.hpp"
#include "mwrender/localmap.hpp"
#include "mwrender/renderingmanager.hpp"
#include "mwrender/vismask.hpp"
#include "mwworld/cellstore.hpp"
#include "mwworld/esmstore.hpp"
#include "mwworld/worldimp.hpp"
#include "mwworld/worldmodel.hpp"

namespace OMW
{
    MapExtractor::MapExtractor(MWWorld::World& world, osgViewer::Viewer* viewer,
        const std::string& worldMapOutput, const std::string& localMapOutput)
        : mWorld(world)
        , mViewer(viewer)
        , mWorldMapOutputDir(worldMapOutput)
        , mLocalMapOutputDir(localMapOutput)
    {
        std::filesystem::create_directories(mWorldMapOutputDir);
        std::filesystem::create_directories(mLocalMapOutputDir);

        // Create GlobalMap and LocalMap instances
        MWRender::RenderingManager* renderingManager = mWorld.getRenderingManager();
        if (!renderingManager)
        {
            Log(Debug::Error) << "RenderingManager is null in MapExtractor constructor";
            throw std::runtime_error("RenderingManager is null");
        }

        osg::Group* lightRoot = renderingManager->getLightRoot();
        if (!lightRoot)
        {
            Log(Debug::Error) << "LightRoot is null in MapExtractor constructor";
            throw std::runtime_error("LightRoot is null");
        }

        osg::Group* root = lightRoot->getParent(0) ? lightRoot->getParent(0)->asGroup() : nullptr;
        if (!root)
        {
            Log(Debug::Error) << "Root node is null in MapExtractor constructor";
            throw std::runtime_error("Root node is null");
        }

        SceneUtil::WorkQueue* workQueue = renderingManager->getWorkQueue();
        if (!workQueue)
        {
            Log(Debug::Error) << "WorkQueue is null in MapExtractor constructor";
            throw std::runtime_error("WorkQueue is null");
        }

        try
        {
            mGlobalMap = std::make_unique<MWRender::GlobalMap>(root, workQueue);
            mLocalMap = std::make_unique<MWRender::LocalMap>(root);
        }
        catch (const std::exception& e)
        {
            Log(Debug::Error) << "Failed to create map objects: " << e.what();
            throw;
        }
    }

    MapExtractor::~MapExtractor() = default;

    void MapExtractor::extractWorldMap()
    {
        Log(Debug::Info) << "Extracting world map...";

        if (!mGlobalMap)
        {
            Log(Debug::Error) << "Global map not initialized";
            throw std::runtime_error("Global map not initialized");
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

        setupExtractionMode();
        extractExteriorLocalMaps();
        extractInteriorLocalMaps();
        restoreNormalMode();

        Log(Debug::Info) << "Local map extraction complete";
    }

    void MapExtractor::setupExtractionMode()
    {
        mWorld.toggleCollisionMode();
        MWBase::Environment::get().getMechanicsManager()->toggleAI();
        mWorld.toggleScripts();
        mWorld.toggleGodMode();
    }

    void MapExtractor::restoreNormalMode()
    {
        if (!mWorld.getGodModeState())
            mWorld.toggleGodMode();
        if (!mWorld.getScriptsEnabled())
            mWorld.toggleScripts();
        if (!MWBase::Environment::get().getMechanicsManager()->isAIActive())
            MWBase::Environment::get().getMechanicsManager()->toggleAI();
    }

    void MapExtractor::extractExteriorLocalMaps()
    {
        if (!mLocalMap)
        {
            Log(Debug::Error) << "Local map not initialized";
            throw std::runtime_error("Local map not initialized");
        }

        // Get currently active cells
        MWWorld::Scene* scene = &mWorld.getWorldScene();
        if (!scene)
        {
            Log(Debug::Error) << "Scene not available";
            throw std::runtime_error("Scene not available");
        }

        const auto& activeCells = scene->getActiveCells();
        Log(Debug::Info) << "Processing " << activeCells.size() << " currently active cells...";

        int count = 0;
        int skipped = 0;

        for (const MWWorld::CellStore* cellStore : activeCells)
        {
            if (!cellStore->getCell()->isExterior())
                continue;

            int x = cellStore->getCell()->getGridX();
            int y = cellStore->getCell()->getGridY();

            Log(Debug::Info) << "Processing active cell (" << x << "," << y << ")";

            // Request map generation for this cell
            Log(Debug::Verbose) << "Requesting map for cell (" << x << "," << y << ")";
            mLocalMap->requestMap(const_cast<MWWorld::CellStore*>(cellStore));
            Log(Debug::Verbose) << "Map requested for cell (" << x << "," << y << ")";

            // CRITICAL: LocalMap::requestMap() creates RTT cameras that render asynchronously.
            // We must run the render loop to actually execute the RTT rendering before we can
            // access the resulting textures.
            
            MWRender::RenderingManager* renderingManager = mWorld.getRenderingManager();
            if (renderingManager && mViewer)
            {
                Log(Debug::Verbose) << "Starting render loop for cell (" << x << "," << y << ")";
                
                // Phase 1: Setup (let RTT cameras initialize)
                for (int i = 0; i < 5; ++i)
                {
                    mViewer->eventTraversal();
                    mViewer->updateTraversal();
                    renderingManager->update(0.016f, false);
                    mViewer->renderingTraversals();
                }
                
                // Phase 2: Main rendering (RTT cameras execute)
                for (int i = 0; i < 60; ++i)
                {
                    mViewer->eventTraversal();
                    mViewer->updateTraversal();
                    renderingManager->update(0.016f, false);
                    mViewer->renderingTraversals();
                }
                
                // Phase 3: Finalization (ensure GPU->CPU transfer completes)
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                
                for (int i = 0; i < 10; ++i)
                {
                    mViewer->eventTraversal();
                    mViewer->updateTraversal();
                    renderingManager->update(0.016f, false);
                    mViewer->renderingTraversals();
                }
                
                Log(Debug::Verbose) << "Render loop completed for cell (" << x << "," << y << ")";
            }

            // Clean up RTT cameras before trying to access textures
            mLocalMap->cleanupCameras();

            // Now try to get the texture
            Log(Debug::Verbose) << "Getting texture for cell (" << x << "," << y << ")";
            osg::ref_ptr<osg::Texture2D> texture = mLocalMap->getMapTexture(x, y);
            
            if (!texture)
            {
                Log(Debug::Warning) << "No texture for cell (" << x << "," << y << ")";
                skipped++;
                continue;
            }
            
            // Get the image from the texture (should be set by LocalMapRenderToTexture)
            osg::Image* image = texture->getImage();
            
            if (!image)
            {
                Log(Debug::Warning) << "Texture for cell (" << x << "," << y << ") has no image data attached";
                skipped++;
                continue;
            }

            if (image->s() == 0 || image->t() == 0)
            {
                Log(Debug::Warning) << "Empty image for cell (" << x << "," << y << ")";
                skipped++;
                continue;
            }

            Log(Debug::Info) << "Got image size: " << image->s() << "x" << image->t() 
                               << " for cell (" << x << "," << y << ")";

            osg::ref_ptr<osg::Image> outputImage = new osg::Image(*image, osg::CopyOp::DEEP_COPY_ALL);
            
            if (outputImage->s() != 256 || outputImage->t() != 256)
            {
                osg::ref_ptr<osg::Image> resized = new osg::Image;
                resized->allocateImage(256, 256, 1, outputImage->getPixelFormat(), outputImage->getDataType());
                outputImage->scaleImage(256, 256, 1);
                outputImage = resized;
            }

            std::ostringstream filename;
            filename << "(" << x << "," << y << ").png";
            std::filesystem::path outputPath = mLocalMapOutputDir / filename.str();

            if (osgDB::writeImageFile(*outputImage, outputPath.string()))
            {
                count++;
                Log(Debug::Info) << "Saved local map texture for cell (" << x << "," << y << ")";
            }
            else
            {
                Log(Debug::Warning) << "Failed to write texture for cell (" << x << "," << y << ")";
                skipped++;
            }
        }

        Log(Debug::Info) << "Saved " << count << " exterior local map textures";
        if (skipped > 0)
            Log(Debug::Warning) << "Skipped " << skipped << " cells without valid textures";
    }

    void MapExtractor::extractInteriorLocalMaps()
    {
        if (!mLocalMap)
        {
            Log(Debug::Error) << "Local map not initialized";
            throw std::runtime_error("Local map not initialized");
        }

        // Get currently active cells
        MWWorld::Scene* scene = &mWorld.getWorldScene();
        if (!scene)
        {
            Log(Debug::Error) << "Scene not available";
            throw std::runtime_error("Scene not available");
        }

        const auto& activeCells = scene->getActiveCells();
        Log(Debug::Info) << "Processing active interior cells...";

        int count = 0;

        for (const MWWorld::CellStore* cellStore : activeCells)
        {
            if (cellStore->getCell()->isExterior())
                continue;

            ESM::RefId cellId = cellStore->getCell()->getId();
            std::string cellName(cellStore->getCell()->getNameId());

            Log(Debug::Info) << "Processing active interior cell: " << cellName;

            // Request map generation for this cell
            mLocalMap->requestMap(const_cast<MWWorld::CellStore*>(cellStore));

            // CRITICAL: LocalMap::requestMap() creates RTT cameras that render asynchronously.
            // We must run the render loop to actually execute the RTT rendering before we can
            // access the resulting textures.
            
            MWRender::RenderingManager* renderingManager = mWorld.getRenderingManager();
            if (renderingManager && mViewer)
            {
                Log(Debug::Verbose) << "Starting render loop for interior: " << cellName;
                
                // Phase 1: Setup (let RTT cameras initialize)
                for (int i = 0; i < 5; ++i)
                {
                    mViewer->eventTraversal();
                    mViewer->updateTraversal();
                    renderingManager->update(0.016f, false);
                    mViewer->renderingTraversals();
                }
                
                // Phase 2: Main rendering (RTT cameras execute)
                for (int i = 0; i < 60; ++i)
                {
                    mViewer->eventTraversal();
                    mViewer->updateTraversal();
                    renderingManager->update(0.016f, false);
                    mViewer->renderingTraversals();
                }
                
                // Phase 3: Finalization (ensure GPU->CPU transfer completes)
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                
                for (int i = 0; i < 10; ++i)
                {
                    mViewer->eventTraversal();
                    mViewer->updateTraversal();
                    renderingManager->update(0.016f, false);
                    mViewer->renderingTraversals();
                }
                
                Log(Debug::Verbose) << "Render loop completed for interior: " << cellName;
            }

            // Clean up RTT cameras before trying to access textures
            mLocalMap->cleanupCameras();

            saveInteriorCellTextures(cellId, cellName);
            count++;
        }

        Log(Debug::Info) << "Saved " << count << " interior local map textures";
    }

    void MapExtractor::saveInteriorCellTextures(const ESM::RefId& cellId, const std::string& cellName)
    {
        MyGUI::IntRect grid = mLocalMap->getInteriorGrid();
        
        std::string lowerCaseId = cellId.toDebugString();
        std::transform(lowerCaseId.begin(), lowerCaseId.end(), lowerCaseId.begin(), ::tolower);

        int segmentsX = grid.width() + 1;
        int segmentsY = grid.height() + 1;
        
        if (segmentsX <= 0 || segmentsY <= 0)
            return;

        int totalWidth = segmentsX * 256;
        int totalHeight = segmentsY * 256;
        
        osg::ref_ptr<osg::Image> combinedImage = new osg::Image;
        combinedImage->allocateImage(totalWidth, totalHeight, 1, GL_RGB, GL_UNSIGNED_BYTE);
        
        unsigned char* data = combinedImage->data();
        memset(data, 0, totalWidth * totalHeight * 3);

        for (int x = grid.left; x < grid.right; ++x)
        {
            for (int y = grid.top; y < grid.bottom; ++y)
            {
                osg::ref_ptr<osg::Texture2D> texture = mLocalMap->getMapTexture(x, y);
                if (!texture || !texture->getImage())
                    continue;

                osg::Image* segmentImage = texture->getImage();
                int segWidth = segmentImage->s();
                int segHeight = segmentImage->t();
                
                int destX = (x - grid.left) * 256;
                int destY = (y - grid.top) * 256;

                for (int sy = 0; sy < std::min(segHeight, 256); ++sy)
                {
                    for (int sx = 0; sx < std::min(segWidth, 256); ++sx)
                    {
                        unsigned char* srcPixel = segmentImage->data(sx, sy);
                        int dx = destX + sx;
                        int dy = destY + sy;
                        
                        if (dx < totalWidth && dy < totalHeight)
                        {
                            unsigned char* destPixel = data + ((dy * totalWidth + dx) * 3);
                            destPixel[0] = srcPixel[0];
                            destPixel[1] = srcPixel[1];
                            destPixel[2] = srcPixel[2];
                        }
                    }
                }
            }
        }

        std::filesystem::path texturePath = mLocalMapOutputDir / (lowerCaseId + ".png");
        osgDB::writeImageFile(*combinedImage, texturePath.string());

        saveInteriorMapInfo(cellId, lowerCaseId, segmentsX, segmentsY);
    }

    void MapExtractor::saveInteriorMapInfo(const ESM::RefId& cellId, const std::string& lowerCaseId,
                                           int segmentsX, int segmentsY)
    {
        MWWorld::CellStore* cell = mWorld.getWorldModel().findCell(cellId);
        if (!cell)
            return;

        float nA = 0.0f;
        float mX = 0.0f;
        float mY = 0.0f;

        MWWorld::ConstPtr northmarker = cell->searchConst(ESM::RefId::stringRefId("northmarker"));
        if (!northmarker.isEmpty())
        {
            osg::Quat orient(-northmarker.getRefData().getPosition().rot[2], osg::Vec3f(0, 0, 1));
            osg::Vec3f dir = orient * osg::Vec3f(0, 1, 0);
            nA = std::atan2(dir.x(), dir.y());
        }

        osg::BoundingBox bounds;
        osg::ComputeBoundsVisitor computeBoundsVisitor;
        computeBoundsVisitor.setTraversalMask(MWRender::Mask_Scene | MWRender::Mask_Terrain | 
                                               MWRender::Mask_Object | MWRender::Mask_Static);
        
        MWRender::RenderingManager* renderingManager = mWorld.getRenderingManager();
        if (renderingManager && renderingManager->getLightRoot())
        {
            renderingManager->getLightRoot()->accept(computeBoundsVisitor);
            bounds = computeBoundsVisitor.getBoundingBox();
        }

        osg::Vec2f center(bounds.center().x(), bounds.center().y());
        osg::Vec2f min(bounds.xMin(), bounds.yMin());
        
        const float mapWorldSize = Constants::CellSizeInUnits;
        
        mX = ((0 - center.x()) * std::cos(nA) - (0 - center.y()) * std::sin(nA) + center.x() - min.x()) / mapWorldSize * 256.0f * 2.0f;
        mY = ((0 - center.x()) * std::sin(nA) + (0 - center.y()) * std::cos(nA) + center.y() - min.y()) / mapWorldSize * 256.0f * 2.0f;

        std::filesystem::path yamlPath = mLocalMapOutputDir / (lowerCaseId + ".yaml");
        std::ofstream file(yamlPath);

        if (!file)
        {
            Log(Debug::Error) << "Failed to create interior map info file: " << yamlPath;
            return;
        }

        file << "nA: " << nA << "\n";
        file << "mX: " << mX << "\n";
        file << "mY: " << mY << "\n";

        file.close();
    }
}
