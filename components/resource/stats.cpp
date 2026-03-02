#include "stats.hpp"

#include <algorithm>
#include <iomanip>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <osg/PolygonMode>

#include <osgText/Font>
#include <osgText/Text>

#include <osgDB/Registry>

#include <osgViewer/Renderer>
#include <osgViewer/Viewer>

#include <components/vfs/manager.hpp>

#include "cachestats.hpp"

namespace Resource
{
    namespace
    {
        constexpr float statsWidth = 1280.0f;
        constexpr float statsHeight = 1024.0f;
        constexpr float characterSize = 17.0f;
        constexpr float backgroundMargin = 5;
        constexpr float backgroundSpacing = 3;
        constexpr float maxStatsHeight = 420.0f;
        constexpr std::size_t pageSize
            = static_cast<std::size_t>((maxStatsHeight - 2 * backgroundMargin) / characterSize);
        constexpr int statsHandlerKey = osgGA::GUIEventAdapter::KEY_F4;
        const VFS::Path::Normalized fontName("Fonts/DejaVuLGCSansMono.ttf");

        bool collectStatRendering = false;
        bool collectStatCameraObjects = false;
        bool collectStatViewerObjects = false;
        bool collectStatResource = false;
        bool collectStatGPU = false;
        bool collectStatEvent = false;
        bool collectStatFrameRate = false;
        bool collectStatUpdate = false;
        bool collectStatEngine = false;

        std::vector<std::string> generateAllStatNames()
        {
            constexpr std::size_t itemsPerPage = 24;

            constexpr std::string_view firstPage[] = {
                "FrameNumber",
                "",
                "Loading",
                "Compiling",
                "WorkQueue",
                "WorkThread",
                "UnrefQueue",
                "",
                "Texture",
                "StateSet",
                "Composite",
                "",
                "Mechanics Actors",
                "Mechanics Objects",
                "",
                "Physics Actors",
                "Physics Objects",
                "Physics Projectiles",
                "Physics HeightFields",
                "",
                "Lua UsedMemory",
                "",
                "StringRefId Count",
                "",
            };

            static_assert(std::size(firstPage) == itemsPerPage);

            constexpr std::string_view caches[] = {
                "Node",
                "Shape",
                "Shape Instance",
                "Image",
                "Nif",
                "Keyframe",
                "BSShader Material",
                "Groundcover Chunk",
                "Object Chunk",
                "Terrain Chunk",
                "Terrain Texture",
                "Land",
                "Blending Rules",
            };

            constexpr std::string_view cellPreloader[] = {
                "CellPreloader Count",
                "CellPreloader Added",
                "CellPreloader Evicted",
                "CellPreloader Loaded",
                "CellPreloader Expired",
            };

            constexpr std::string_view navMesh[] = {
                "NavMesh Jobs",
                "NavMesh Removing",
                "NavMesh Updating",
                "NavMesh Delayed",
                "NavMesh Pushed",
                "NavMesh Processing",
                "NavMesh DbJobs Write",
                "NavMesh DbJobs Read",
                "NavMesh DbCache Get",
                "NavMesh DbCache Hit",
                "NavMesh CacheSize",
                "NavMesh UsedTiles",
                "NavMesh CachedTiles",
                "NavMesh Cache Get",
                "NavMesh Cache Hit",
                "NavMesh Recast Tiles",
                "NavMesh Recast Objects",
                "NavMesh Recast Heightfields",
                "NavMesh Recast Water",
            };

            std::vector<std::string> statNames;

            for (std::string_view name : firstPage)
                statNames.emplace_back(name);

            for (std::size_t i = 0; i < std::size(caches); ++i)
            {
                Resource::addCacheStatsAttibutes(caches[i], statNames);
                if ((i + 1) % 5 != 0)
                    statNames.emplace_back();
            }

            for (std::string_view name : cellPreloader)
                statNames.emplace_back(name);

            while (statNames.size() % itemsPerPage != 0)
                statNames.emplace_back();

            for (std::string_view name : navMesh)
                statNames.emplace_back(name);

            return statNames;
        }

        void setupStatCollection()
        {
            const char* envList = getenv("OPENMW_OSG_STATS_LIST");
            if (envList == nullptr)
                return;

            std::string_view kwList(envList);

            auto kwBegin = kwList.begin();

            while (kwBegin != kwList.end())
            {
                auto kwEnd = std::find(kwBegin, kwList.end(), ';');

                const auto kw = kwList.substr(std::distance(kwList.begin(), kwBegin), std::distance(kwBegin, kwEnd));

                if (kw == "gpu")
                    collectStatGPU = true;
                else if (kw == "event")
                    collectStatEvent = true;
                else if (kw == "frame_rate")
                    collectStatFrameRate = true;
                else if (kw == "update")
                    collectStatUpdate = true;
                else if (kw == "engine")
                    collectStatEngine = true;
                else if (kw == "rendering")
                    collectStatRendering = true;
                else if (kw == "cameraobjects")
                    collectStatCameraObjects = true;
                else if (kw == "viewerobjects")
                    collectStatViewerObjects = true;
                else if (kw == "resource")
                    collectStatResource = true;
                else if (kw == "times")
                {
                    collectStatGPU = true;
                    collectStatEvent = true;
                    collectStatFrameRate = true;
                    collectStatUpdate = true;
                    collectStatEngine = true;
                    collectStatRendering = true;
                }

                if (kwEnd == kwList.end())
                    break;

                kwBegin = std::next(kwEnd);
            }
        }

        osg::ref_ptr<osg::Geometry> createBackgroundRectangle(
            const osg::Vec3& pos, const float width, const float height, const osg::Vec4& color)
        {
            osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;

            geometry->setUseDisplayList(false);

            osg::ref_ptr<osg::StateSet> stateSet = new osg::StateSet;
            geometry->setStateSet(stateSet);

            osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
            vertices->push_back(osg::Vec3(pos.x(), pos.y(), 0));
            vertices->push_back(osg::Vec3(pos.x(), pos.y() - height, 0));
            vertices->push_back(osg::Vec3(pos.x() + width, pos.y() - height, 0));
            vertices->push_back(osg::Vec3(pos.x() + width, pos.y(), 0));
            geometry->setVertexArray(vertices);

            osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
            colors->push_back(color);
            geometry->setColorArray(colors, osg::Array::BIND_OVERALL);

            osg::ref_ptr<osg::DrawElementsUShort> base
                = new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLE_FAN, 0);
            base->push_back(0);
            base->push_back(1);
            base->push_back(2);
            base->push_back(3);
            geometry->addPrimitiveSet(base);

            return geometry;
        }

        osg::ref_ptr<osgText::Font> getMonoFont(const VFS::Manager& vfs)
        {
            if (osgDB::Registry::instance()->getReaderWriterForExtension("ttf") && vfs.exists(fontName))
            {
                const Files::IStreamPtr streamPtr = vfs.get(fontName);
                return osgText::readRefFontStream(*streamPtr);
            }

            return nullptr;
        }

        class SetFontVisitor : public osg::NodeVisitor
        {
        public:
            SetFontVisitor(osgText::Font* font)
                : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
                , mFont(font)
            {
            }

            void apply(osg::Drawable& node) override
            {
                if (osgText::Text* text = dynamic_cast<osgText::Text*>(&node))
                {
                    text->setFont(mFont);
                }
            }

        private:
            osgText::Font* mFont;
        };
    }

    Profiler::Profiler(bool offlineCollect, const VFS::Manager& vfs)
        : mOfflineCollect(offlineCollect)
        , mTextFont(getMonoFont(vfs))
    {
        _characterSize = characterSize;
        _font.clear();

        setKeyEventTogglesOnScreenStats(osgGA::GUIEventAdapter::KEY_F3);
        setupStatCollection();
    }

    void Profiler::setUpFonts()
    {
        if (mTextFont != nullptr)
        {
            SetFontVisitor visitor(mTextFont);
            _switch->accept(visitor);
        }

        mInitFonts = true;
    }

    bool Profiler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::ViewerBase* viewer = nullptr;

        bool handled = StatsHandler::handle(ea, aa);
        if (_initialized && !mInitFonts)
            setUpFonts();

        auto* view = dynamic_cast<osgViewer::View*>(&aa);
        if (view)
            viewer = view->getViewerBase();

        if (viewer != nullptr)
        {
            // Add/remove openmw stats to the osd as necessary
            viewer->getViewerStats()->collectStats("engine", _statsType >= StatsHandler::StatsType::VIEWER_STATS);

            if (mOfflineCollect)
                collectStatistics(*viewer);
        }
        return handled;
    }

    StatsHandler::StatsHandler(bool offlineCollect, const VFS::Manager& vfs)
        : mOfflineCollect(offlineCollect)
        , mSwitch(new osg::Switch)
        , mCamera(new osg::Camera)
        , mTextFont(getMonoFont(vfs))
        , mStatNames(generateAllStatNames())
    {
        osg::ref_ptr<osg::StateSet> stateset = mSwitch->getOrCreateStateSet();
        stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        stateset->setMode(GL_BLEND, osg::StateAttribute::ON);
        stateset->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
#ifdef OSG_GL1_AVAILABLE
        stateset->setAttribute(new osg::PolygonMode(), osg::StateAttribute::PROTECTED);
#endif

        mCamera->getOrCreateStateSet()->setGlobalDefaults();
        mCamera->setRenderer(new osgViewer::Renderer(mCamera.get()));
        mCamera->setProjectionResizePolicy(osg::Camera::FIXED);
        mCamera->addChild(mSwitch);
    }

    bool StatsHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        if (ea.getHandled())
            return false;

        switch (ea.getEventType())
        {
            case (osgGA::GUIEventAdapter::KEYDOWN):
            {
                if (ea.getKey() == statsHandlerKey)
                {
                    osgViewer::View* const view = dynamic_cast<osgViewer::View*>(&aa);
                    if (view == nullptr)
                        return false;

                    osgViewer::ViewerBase* const viewer = view->getViewerBase();

                    if (viewer == nullptr)
                        return false;

                    toggle(*viewer);

                    if (mOfflineCollect)
                        collectStatistics(*viewer);

                    aa.requestRedraw();
                    return true;
                }
                break;
            }
            case osgGA::GUIEventAdapter::RESIZE:
            {
                setWindowSize(ea.getWindowWidth(), ea.getWindowHeight());
                break;
            }
            default:
                break;
        }
        return false;
    }

    void StatsHandler::setWindowSize(int width, int height)
    {
        if (width <= 0 || height <= 0)
            return;

        mCamera->setViewport(0, 0, width, height);
        if (std::abs(height * statsWidth) <= std::abs(width * statsHeight))
        {
            mCamera->setProjectionMatrix(
                osg::Matrix::ortho2D(statsWidth - width * statsHeight / height, statsWidth, 0.0, statsHeight));
        }
        else
        {
            mCamera->setProjectionMatrix(
                osg::Matrix::ortho2D(0.0, statsWidth, statsHeight - height * statsWidth / width, statsHeight));
        }
    }

    void StatsHandler::toggle(osgViewer::ViewerBase& viewer)
    {
        if (!mInitialized)
        {
            setUpHUDCamera(viewer);
            setUpScene(viewer);
            mInitialized = true;
        }

        if (mPage == mSwitch->getNumChildren())
        {
            mPage = 0;

            mCamera->setNodeMask(0);
            mSwitch->setAllChildrenOff();

            viewer.getViewerStats()->collectStats("resource", false);
        }
        else
        {
            mCamera->setNodeMask(0xffffffff);
            mSwitch->setSingleChildOn(mPage);

            viewer.getViewerStats()->collectStats("resource", true);

            ++mPage;
        }
    }

    void StatsHandler::setUpHUDCamera(osgViewer::ViewerBase& viewer)
    {
        // Try GraphicsWindow first so we're likely to get the main viewer window
        osg::GraphicsContext* context = dynamic_cast<osgViewer::GraphicsWindow*>(mCamera->getGraphicsContext());

        if (!context)
        {
            osgViewer::Viewer::Windows windows;
            viewer.getWindows(windows);

            if (!windows.empty())
                context = windows.front();
            else
            {
                // No GraphicsWindows were found, so let's try to find a GraphicsContext
                context = mCamera->getGraphicsContext();

                if (!context)
                {
                    osgViewer::Viewer::Contexts contexts;
                    viewer.getContexts(contexts);

                    if (contexts.empty())
                        return;

                    context = contexts.front();
                }
            }
        }

        mCamera->setGraphicsContext(context);

        mCamera->setRenderOrder(osg::Camera::POST_RENDER, 11);

        mCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        mCamera->setViewMatrix(osg::Matrix::identity());
        setWindowSize(context->getTraits()->width, context->getTraits()->height);

        // only clear the depth buffer
        mCamera->setClearMask(0);
        mCamera->setAllowEventFocus(false);

        mCamera->setRenderer(new osgViewer::Renderer(mCamera.get()));
    }

    namespace
    {
        class ResourceStatsTextDrawCallback : public osg::Drawable::DrawCallback
        {
        public:
            explicit ResourceStatsTextDrawCallback(osg::Stats* stats, std::span<const std::string> statNames)
                : mStats(stats)
                , mStatNames(statNames)
            {
            }

            void drawImplementation(osg::RenderInfo& renderInfo, const osg::Drawable* drawable) const override
            {
                if (mStats == nullptr)
                    return;

                osgText::Text* text = (osgText::Text*)(drawable);

                std::ostringstream viewStr;
                viewStr.setf(std::ios::left, std::ios::adjustfield);
                viewStr.width(14);
                // Used fixed formatting, as scientific will switch to "...e+.." notation for
                // large numbers of vertices/drawables/etc.
                viewStr.setf(std::ios::fixed);
                viewStr.precision(0);

                const unsigned int frameNumber = renderInfo.getState()->getFrameStamp()->getFrameNumber() - 1;

                for (const std::string& statName : mStatNames)
                {
                    if (statName.empty())
                        viewStr << std::endl;
                    else
                    {
                        double value = 0.0;
                        if (mStats->getAttribute(frameNumber, statName, value))
                            viewStr << std::setw(8) << value << std::endl;
                        else
                            viewStr << std::setw(8) << "." << std::endl;
                    }
                }

                text->setText(viewStr.str());

                text->drawImplementation(renderInfo);
            }

        private:
            osg::ref_ptr<osg::Stats> mStats;
            std::span<const std::string> mStatNames;
        };
    }

    void StatsHandler::setUpScene(osgViewer::ViewerBase& viewer)
    {
        const osg::Vec4 backgroundColor(0.0f, 0.0f, 0.0f, 0.3f);
        const osg::Vec4 staticTextColor(1.0f, 1.0f, 0.0f, 1.0f);
        const osg::Vec4 dynamicTextColor(1.0f, 1.0f, 1.0f, 1.0f);

        const auto longest = std::max_element(mStatNames.begin(), mStatNames.end(),
            [](const std::string& lhs, const std::string& rhs) { return lhs.size() < rhs.size(); });
        const std::size_t longestSize = longest->size();
        const float statNamesWidth = longestSize * characterSize * 0.6f + 2 * backgroundMargin;
        const float statTextWidth = 7 * characterSize + 2 * backgroundMargin;
        const float statHeight = pageSize * characterSize + 2 * backgroundMargin;
        const float width = statNamesWidth + backgroundSpacing + statTextWidth;

        for (std::size_t offset = 0; offset < mStatNames.size(); offset += pageSize)
        {
            osg::ref_ptr<osg::Group> group = new osg::Group;

            group->setCullingActive(false);

            const std::size_t count = std::min(mStatNames.size() - offset, pageSize);
            std::span<const std::string> currentStatNames(mStatNames.data() + offset, count);
            osg::Vec3 pos(statsWidth - width, statHeight - characterSize, 0.0f);

            group->addChild(
                createBackgroundRectangle(pos + osg::Vec3(-backgroundMargin, backgroundMargin + characterSize, 0),
                    statNamesWidth, statHeight, backgroundColor));

            osg::ref_ptr<osgText::Text> staticText = new osgText::Text;
            group->addChild(staticText.get());
            staticText->setColor(staticTextColor);
            staticText->setCharacterSize(characterSize);
            staticText->setPosition(pos);

            std::ostringstream viewStr;
            viewStr.clear();
            viewStr.setf(std::ios::left, std::ios::adjustfield);
            viewStr.width(longestSize);
            for (const std::string& statName : currentStatNames)
                viewStr << statName << std::endl;

            staticText->setText(viewStr.str());

            pos.x() += statNamesWidth + backgroundSpacing;

            group->addChild(
                createBackgroundRectangle(pos + osg::Vec3(-backgroundMargin, backgroundMargin + characterSize, 0),
                    statTextWidth, statHeight, backgroundColor));

            osg::ref_ptr<osgText::Text> statsText = new osgText::Text;
            group->addChild(statsText.get());

            statsText->setColor(dynamicTextColor);
            statsText->setCharacterSize(characterSize);
            statsText->setPosition(pos);
            statsText->setText("");
            statsText->setDrawCallback(new ResourceStatsTextDrawCallback(viewer.getViewerStats(), currentStatNames));

            if (mTextFont != nullptr)
            {
                staticText->setFont(mTextFont);
                statsText->setFont(mTextFont);
            }

            mSwitch->addChild(group, false);
        }
    }

    void StatsHandler::getUsage(osg::ApplicationUsage& usage) const
    {
        usage.addKeyboardMouseBinding(statsHandlerKey, "On screen resource usage stats.");
    }

    void collectStatistics(osgViewer::ViewerBase& viewer)
    {
        osgViewer::Viewer::Cameras cameras;
        viewer.getCameras(cameras);
        for (auto* camera : cameras)
        {
            if (collectStatGPU)
                camera->getStats()->collectStats("gpu", true);
            if (collectStatRendering)
                camera->getStats()->collectStats("rendering", true);
            if (collectStatCameraObjects)
                camera->getStats()->collectStats("scene", true);
        }
        if (collectStatEvent)
            viewer.getViewerStats()->collectStats("event", true);
        if (collectStatFrameRate)
            viewer.getViewerStats()->collectStats("frame_rate", true);
        if (collectStatUpdate)
            viewer.getViewerStats()->collectStats("update", true);
        if (collectStatResource)
            viewer.getViewerStats()->collectStats("resource", true);
        if (collectStatViewerObjects)
            viewer.getViewerStats()->collectStats("scene", true);
        if (collectStatEngine)
            viewer.getViewerStats()->collectStats("engine", true);
    }
}
