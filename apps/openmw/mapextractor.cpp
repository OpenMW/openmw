#include "mapextractor.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <map>
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
#include "mwbase/windowmanager.hpp"
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
    MapExtractor::MapExtractor(const std::string& worldMapOutput, const std::string& localMapOutput,
        bool forceOverwrite, MWRender::RenderingManager* renderingManager, const MWWorld::ESMStore* store)
        : mWorldMapOutputDir(worldMapOutput)
        , mLocalMapOutputDir(localMapOutput)
        , mRenderingManager(renderingManager)
        , mStore(store)
        , mLocalMap(nullptr)
        , mForceOverwrite(forceOverwrite)
    {
        // Only create directories if paths are not empty
        if (!mWorldMapOutputDir.empty())
            std::filesystem::create_directories(mWorldMapOutputDir);
        if (!mLocalMapOutputDir.empty())
            std::filesystem::create_directories(mLocalMapOutputDir);

        // Create GlobalMap instance
        if (!mRenderingManager)
        {
            Log(Debug::Error) << "RenderingManager is null in MapExtractor constructor";
            throw std::runtime_error("RenderingManager is null");
        }

        osg::Group* lightRoot = mRenderingManager->getLightRoot();
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

        SceneUtil::WorkQueue* workQueue = mRenderingManager->getWorkQueue();
        if (!workQueue)
        {
            Log(Debug::Error) << "WorkQueue is null in MapExtractor constructor";
            throw std::runtime_error("WorkQueue is null");
        }

        try
        {
            mGlobalMap = std::make_unique<MWRender::GlobalMap>(root, workQueue);
            // LocalMap will be set later via setLocalMap() method
            mLocalMap = nullptr;
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

        if (mWorldMapOutputDir.empty())
        {
            Log(Debug::Warning) << "World map output directory is not set, skipping world map extraction";
            return;
        }

        // Create output directory if it doesn't exist
        std::filesystem::create_directories(mWorldMapOutputDir);

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

        int minX = std::numeric_limits<int>::max();
        int maxX = std::numeric_limits<int>::min();
        int minY = std::numeric_limits<int>::max();
        int maxY = std::numeric_limits<int>::min();

        MWWorld::Store<ESM::Cell>::iterator it = mStore->get<ESM::Cell>().extBegin();
        for (; it != mStore->get<ESM::Cell>().extEnd(); ++it)
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

    void MapExtractor::extractLocalMaps(const std::vector<const MWWorld::CellStore*>& activeCells)
    {
        Log(Debug::Info) << "Extracting active local maps...";

        if (mLocalMapOutputDir.empty())
        {
            Log(Debug::Warning) << "Local map output directory is not set, skipping local map extraction";
            return;
        }

        // Create output directory if it doesn't exist
        std::filesystem::create_directories(mLocalMapOutputDir);

        if (!mLocalMap)
        {
            Log(Debug::Error) << "Local map not initialized - cannot extract local maps";
            return;
        }

        Log(Debug::Info) << "LocalMap instance is available, starting extraction";
        
        mFramesToWait = 10; // Wait 10 frames before checking (increased from 3)
        
        startExtraction(activeCells);
    }
    
    void MapExtractor::startExtraction(const std::vector<const MWWorld::CellStore*>& activeCells)
    {
        // Enable extraction mode to prevent automatic camera cleanup
        if (mLocalMap)
        {
            mLocalMap->setExtractionMode(true);
        }
        
        Log(Debug::Info) << "Processing " << activeCells.size() << " currently active cells...";

        mPendingExtractions.clear();
        
        for (const MWWorld::CellStore* cellStore : activeCells)
        {
            if (cellStore->getCell()->isExterior())
            {
                int x = cellStore->getCell()->getGridX();
                int y = cellStore->getCell()->getGridY();
                
                std::ostringstream filename;
                filename << "(" << x << "," << y << ").png";
                std::filesystem::path outputPath = mLocalMapOutputDir / filename.str();

                if (!mForceOverwrite && std::filesystem::exists(outputPath))
                {
                    Log(Debug::Info) << "Skipping cell (" << x << "," << y << ") - file already exists";
                    continue;
                }
                
                PendingExtraction extraction;
                extraction.cellStore = cellStore;
                extraction.isExterior = true;
                extraction.outputPath = outputPath;
                extraction.framesWaited = 0;
                mPendingExtractions.push_back(extraction);
            }
            else
            {
                ESM::RefId cellId = cellStore->getCell()->getId();
                std::string cellName(cellStore->getCell()->getNameId());

                std::string lowerCaseId = cellId.toDebugString();
                std::transform(lowerCaseId.begin(), lowerCaseId.end(), lowerCaseId.begin(), ::tolower);

                const std::string invalidChars = "/\\:*?\"<>|";
                lowerCaseId.erase(std::remove_if(lowerCaseId.begin(), lowerCaseId.end(),
                    [&invalidChars](char c) { return invalidChars.find(c) != std::string::npos; }),
                    lowerCaseId.end()
                );

                if (lowerCaseId.empty())
                {
                    lowerCaseId = "_unnamed_cell_";
                }

                std::filesystem::path texturePath = mLocalMapOutputDir / (lowerCaseId + ".png");
                std::filesystem::path yamlPath = mLocalMapOutputDir / (lowerCaseId + ".yaml");

                if (!mForceOverwrite && std::filesystem::exists(yamlPath))
                {
                    Log(Debug::Info) << "Skipping interior cell: " << cellName << " - files already exist";
                    continue;
                }
                
                PendingExtraction extraction;
                extraction.cellStore = cellStore;
                extraction.isExterior = false;
                extraction.outputPath = texturePath;
                extraction.framesWaited = 0;
                mPendingExtractions.push_back(extraction);
            }
        }
        
        if (!mPendingExtractions.empty())
        {
            Log(Debug::Info) << "Queued " << mPendingExtractions.size() << " cells for extraction";
            processNextCell();
        }
        else
        {
            Log(Debug::Info) << "No cells to extract";
            // Clean up any cameras that may have been created
            if (mLocalMap)
            {
                mLocalMap->cleanupCameras();
                mLocalMap->setExtractionMode(false);
            }
        }
    }
    
    void MapExtractor::update()
    {
        if (mPendingExtractions.empty())
            return;
            
        PendingExtraction& current = mPendingExtractions.front();
        current.framesWaited++;
        
        // Wait for the required number of frames before checking
        if (current.framesWaited >= mFramesToWait)
        {
            // Check if the texture is ready before trying to save
            bool textureReady = false;
            
            if (current.isExterior)
            {
                int x = current.cellStore->getCell()->getGridX();
                int y = current.cellStore->getCell()->getGridY();
                
                // Check if the rendered image is ready
                osg::ref_ptr<osg::Image> image = mLocalMap->getMapImage(x, y);
                
                if (image && image->s() > 0 && image->t() > 0 && image->data() != nullptr)
                {
                    textureReady = true;
                }
                else if (current.framesWaited <= 120)
                {
                    return;
                }
            }
            else
            {
                // For interior cells, check if at least one segment has a valid rendered image
                MyGUI::IntRect grid = mLocalMap->getInteriorGrid();
                for (int x = grid.left; x < grid.right && !textureReady; ++x)
                {
                    for (int y = grid.top; y < grid.bottom && !textureReady; ++y)
                    {
                        osg::ref_ptr<osg::Image> image = mLocalMap->getMapImage(x, y);
                        if (image && image->s() > 0 && image->t() > 0 && image->data() != nullptr)
                        {
                            textureReady = true;
                        }
                    }
                }
                
                
                if (!textureReady && current.framesWaited <= 120)
                {
                    return;
                }
            }
            
            if (textureReady)
            {
                savePendingExtraction(current);
                
                mPendingExtractions.erase(mPendingExtractions.begin());
                
                // Clean up the camera for the current cell immediately after saving
                // This prevents memory buildup when processing many cells
                if (mLocalMap)
                    mLocalMap->cleanupCameras();
                
                if (!mPendingExtractions.empty())
                {
                    processNextCell();
                }
                else
                {
                    // Disable extraction mode
                    mLocalMap->setExtractionMode(false);
                    Log(Debug::Info) << "Extraction of active local maps complete";
                }
            }
            else if (current.framesWaited > 120)
            {
                // If we've waited too long (120 frames = ~2 seconds at 60 fps), skip this cell
                if (current.isExterior)
                {
                    int x = current.cellStore->getCell()->getGridX();
                    int y = current.cellStore->getCell()->getGridY();
                    Log(Debug::Warning) << "Timeout waiting for texture for cell (" << x << "," << y << "), skipping";
                }
                else
                {
                    std::string cellName(current.cellStore->getCell()->getNameId());
                    Log(Debug::Warning) << "Timeout waiting for texture for interior cell: " << cellName << ", skipping";
                }
                
                mPendingExtractions.erase(mPendingExtractions.begin());
                
                // Clean up cameras even after timeout
                if (mLocalMap)
                    mLocalMap->cleanupCameras();
                
                if (!mPendingExtractions.empty())
                {
                    processNextCell();
                }
                else
                {
                    // Disable extraction mode
                    mLocalMap->setExtractionMode(false);
                    Log(Debug::Info) << "Extraction of active local maps complete";
                }
            }
            // Otherwise keep waiting for the texture to be ready
        }
    }
    
    bool MapExtractor::isExtractionComplete() const
    {
        return mPendingExtractions.empty();
    }
    
    void MapExtractor::processNextCell()
    {
        if (mPendingExtractions.empty())
            return;
            
        const PendingExtraction& extraction = mPendingExtractions.front();
        
        if (extraction.isExterior)
        {
            int x = extraction.cellStore->getCell()->getGridX();
            int y = extraction.cellStore->getCell()->getGridY();
            mLocalMap->clearCellCache(x, y);
        }
        
        mLocalMap->requestMap(const_cast<MWWorld::CellStore*>(extraction.cellStore));
    }
    
    bool MapExtractor::savePendingExtraction(const PendingExtraction& extraction)
    {
        if (extraction.isExterior)
        {
            return extractExteriorCell(extraction.cellStore, mForceOverwrite);
        }
        else
        {
            return extractInteriorCell(extraction.cellStore, mForceOverwrite);
        }
    }

    bool MapExtractor::extractExteriorCell(const MWWorld::CellStore* cellStore, bool forceOverwrite)
    {
        int x = cellStore->getCell()->getGridX();
        int y = cellStore->getCell()->getGridY();

        std::ostringstream filename;
        filename << "(" << x << "," << y << ").png";
        std::filesystem::path outputPath = mLocalMapOutputDir / filename.str();

        osg::ref_ptr<osg::Image> image = mLocalMap->getMapImage(x, y);
        
        if (!image)
        {
            Log(Debug::Warning) << "Texture for cell (" << x << "," << y << ") has no image data attached";
            return false;
        }

        if (image->s() == 0 || image->t() == 0)
        {
            Log(Debug::Warning) << "Empty image for cell (" << x << "," << y << ") - size: " 
                               << image->s() << "x" << image->t();
            return false;
        }

        osg::ref_ptr<osg::Image> outputImage = new osg::Image(*image, osg::CopyOp::DEEP_COPY_ALL);
        
        if (outputImage->s() != 256 || outputImage->t() != 256)
        {
            osg::ref_ptr<osg::Image> resized = new osg::Image;
            resized->allocateImage(256, 256, 1, outputImage->getPixelFormat(), outputImage->getDataType());
            outputImage->scaleImage(256, 256, 1);
            outputImage = resized;
        }
        if (osgDB::writeImageFile(*outputImage, outputPath.string()))
        {
            Log(Debug::Info) << "Successfully saved local map for cell (" << x << "," << y << ") to " << outputPath;
            return true;
        }
        else
        {
            Log(Debug::Warning) << "Failed to write texture for cell (" << x << "," << y << ") to " << outputPath;
            return false;
        }
    }

    bool MapExtractor::extractInteriorCell(const MWWorld::CellStore* cellStore, bool forceOverwrite)
    {
        ESM::RefId cellId = cellStore->getCell()->getId();
        std::string cellName(cellStore->getCell()->getNameId());

        Log(Debug::Info) << "Saving interior cell: " << cellName;

        saveInteriorCellTextures(cellId, cellName);
        return true;
    }

    void MapExtractor::saveInteriorCellTextures(const ESM::RefId& cellId, const std::string& cellName)
    {
        MyGUI::IntRect grid = mLocalMap->getInteriorGrid();
        
        std::string lowerCaseId = cellId.toDebugString();
        std::transform(lowerCaseId.begin(), lowerCaseId.end(), lowerCaseId.begin(), ::tolower);

        const std::string invalidChars = "/\\:*?\"<>|";
        lowerCaseId.erase(std::remove_if(lowerCaseId.begin(), lowerCaseId.end(),
            [&invalidChars](char c) { return invalidChars.find(c) != std::string::npos; }),
            lowerCaseId.end()
        );

        if (lowerCaseId.empty())
        {
            lowerCaseId = "_unnamed_cell_";
        }

        int minX = grid.right;
        int maxX = grid.left;
        int minY = grid.bottom;
        int maxY = grid.top;
        
        // Cache images and find bounds
        std::map<std::pair<int, int>, osg::ref_ptr<osg::Image>> imageCache;
        
        for (int x = grid.left; x < grid.right; ++x)
        {
            for (int y = grid.top; y < grid.bottom; ++y)
            {
                // Get the rendered image directly from camera attachment
                osg::ref_ptr<osg::Image> image = mLocalMap->getMapImage(x, y);
                if (image && image->s() > 0 && image->t() > 0 && image->data() != nullptr)
                {
                    imageCache[{x, y}] = image;
                    minX = std::min(minX, x);
                    maxX = std::max(maxX, x);
                    minY = std::min(minY, y);
                    maxY = std::max(maxY, y);
                }
            }
        }
        
        if (minX > maxX || minY > maxY)
        {
            Log(Debug::Warning) << "No valid image segments found for interior cell: " << cellName;
            return;
        }
        
        int segmentsX = maxX - minX + 1;
        int segmentsY = maxY - minY + 1;
        
        if (segmentsX <= 0 || segmentsY <= 0)
        {
            Log(Debug::Warning) << "Invalid segment dimensions for interior cell: " << cellName;
            return;
        }

        int totalWidth = segmentsX * 256;
        int totalHeight = segmentsY * 256;
        
        osg::ref_ptr<osg::Image> combinedImage = new osg::Image;
        combinedImage->allocateImage(totalWidth, totalHeight, 1, GL_RGB, GL_UNSIGNED_BYTE);
        
        unsigned char* data = combinedImage->data();
        memset(data, 0, totalWidth * totalHeight * 3);

        for (int x = minX; x <= maxX; ++x)
        {
            for (int y = minY; y <= maxY; ++y)
            {
                auto it = imageCache.find({x, y});
                if (it == imageCache.end())
                    continue;

                osg::ref_ptr<osg::Image> segmentImage = it->second;
                int segWidth = segmentImage->s();
                int segHeight = segmentImage->t();
                
                int destX = (x - minX) * 256;
                int destY = (y - minY) * 256;

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
        
        if (osgDB::writeImageFile(*combinedImage, texturePath.string()))
        {
            Log(Debug::Info) << "Successfully saved interior map to " << texturePath;
        }
        else
        {
            Log(Debug::Error) << "Failed to write interior map to " << texturePath;
            return;
        }

        saveInteriorMapInfo(cellId, lowerCaseId, segmentsX, segmentsY);
    }

    void MapExtractor::saveInteriorMapInfo(const ESM::RefId& cellId, const std::string& lowerCaseId,
                                           int segmentsX, int segmentsY)
    {
        // Get the bounds, center and angle that LocalMap actually used for rendering
        const osg::BoundingBox& bounds = mLocalMap->getInteriorBounds();
        const osg::Vec2f& center = mLocalMap->getInteriorCenter();
        const float nA = mLocalMap->getInteriorAngle();
        
        osg::Vec2f min(bounds.xMin(), bounds.yMin());
        
        const float mapWorldSize = Constants::CellSizeInUnits;
        
        // Calculate position of world origin (0,0) on the rotated map
        osg::Vec2f toOrigin(0.0f - center.x(), 0.0f - center.y());
        float rotatedX = toOrigin.x() * std::cos(nA) - toOrigin.y() * std::sin(nA) + center.x();
        float rotatedY = toOrigin.x() * std::sin(nA) + toOrigin.y() * std::cos(nA) + center.y();
        
        // Convert to texture coordinates (pixels from bottom-left corner)
        float oX = (rotatedX - min.x()) / mapWorldSize * 256.0f;
        float oY = (rotatedY - min.y()) / mapWorldSize * 256.0f;
        
        float totalHeight = segmentsY * 256.0f;

        std::filesystem::path yamlPath = mLocalMapOutputDir / (lowerCaseId + ".yaml");
        std::ofstream file(yamlPath);

        if (!file)
        {
            Log(Debug::Error) << "Failed to create interior map info file: " << yamlPath;
            return;
        }

        // TODO: Replace hardcoded padding with a constant from LocalMap
        // Padding value taken from the implementation in localmap.cpp.
        // Used to shrink the bounds to match the one used during cell rendering.
        const float padding = 500.0f;

        file << "v: 2\n";
        file << "nA: " << nA << "\n";
        file << "oX: " << oX << "\n";
        file << "oY: " << oY << "\n";
        file << "wT: " << segmentsX << "\n";
        file << "hT: " << segmentsY << "\n";
        file << "mBnds:\n";
        file << "  min: [" << bounds.xMin() + padding << ", " << bounds.yMin() + padding << ", " << bounds.zMin() << "]\n";
        file << "  max: [" << bounds.xMax() - padding << ", " << bounds.yMax() - padding << ", " << bounds.zMax() << "]\n";

        file.close();
    }
}
