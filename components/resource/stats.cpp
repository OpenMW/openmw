#include "stats.hpp"

#include <sstream>
#include <iomanip>
#include <algorithm>

#include <osg/PolygonMode>

#include <osgText/Text>

#include <osgDB/Registry>

#include <osgViewer/Viewer>
#include <osgViewer/Renderer>

#include <components/myguiplatform/myguidatamanager.hpp>

namespace Resource
{

static bool collectStatRendering = false;
static bool collectStatCameraObjects = false;
static bool collectStatViewerObjects = false;
static bool collectStatResource = false;
static bool collectStatGPU = false;
static bool collectStatEvent = false;
static bool collectStatFrameRate = false;
static bool collectStatUpdate = false;
static bool collectStatEngine = false;

static void setupStatCollection()
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

        if (kw.compare("gpu") == 0)
            collectStatGPU = true;
        else if (kw.compare("event") == 0)
            collectStatEvent = true;
        else if (kw.compare("frame_rate") == 0)
            collectStatFrameRate = true;
        else if (kw.compare("update") == 0)
            collectStatUpdate = true;
        else if (kw.compare("engine") == 0)
            collectStatEngine = true;
        else if (kw.compare("rendering") == 0)
            collectStatRendering = true;
        else if (kw.compare("cameraobjects") == 0)
            collectStatCameraObjects = true;
        else if (kw.compare("viewerobjects") == 0)
            collectStatViewerObjects = true;
        else if (kw.compare("resource") == 0)
            collectStatResource = true;
        else if (kw.compare("times") == 0)
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

StatsHandler::StatsHandler(bool offlineCollect):
    _key(osgGA::GUIEventAdapter::KEY_F4),
    _initialized(false),
    _statsType(false),
    _offlineCollect(offlineCollect),
    _statsWidth(1280.0f),
    _statsHeight(1024.0f),
    _font(""),
    _characterSize(18.0f)
{
    _camera = new osg::Camera;
    _camera->getOrCreateStateSet()->setGlobalDefaults();
    _camera->setRenderer(new osgViewer::Renderer(_camera.get()));
    _camera->setProjectionResizePolicy(osg::Camera::FIXED);

    _resourceStatsChildNum = 0;

    if (osgDB::Registry::instance()->getReaderWriterForExtension("ttf"))
        _font = osgMyGUI::DataManager::getInstance().getDataPath("DejaVuLGCSansMono.ttf");
}

Profiler::Profiler(bool offlineCollect):
    _offlineCollect(offlineCollect)
{
    if (osgDB::Registry::instance()->getReaderWriterForExtension("ttf"))
        _font = osgMyGUI::DataManager::getInstance().getDataPath("DejaVuLGCSansMono.ttf");
    else
        _font = "";

    _characterSize = 18;

    setKeyEventTogglesOnScreenStats(osgGA::GUIEventAdapter::KEY_F3);
    setupStatCollection();
}

bool Profiler::handle(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa)
{
    osgViewer::ViewerBase* viewer = nullptr;

    bool handled = StatsHandler::handle(ea, aa);

    auto* view = dynamic_cast<osgViewer::View*>(&aa);
    if (view)
        viewer = view->getViewerBase();

    if (viewer)
    {
        // Add/remove openmw stats to the osd as necessary
        viewer->getViewerStats()->collectStats("engine", _statsType >= StatsHandler::StatsType::VIEWER_STATS);

        if (_offlineCollect)
            CollectStatistics(viewer);
    }
    return handled;
}

bool StatsHandler::handle(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa)
{
    if (ea.getHandled()) return false;

    switch(ea.getEventType())
    {
        case(osgGA::GUIEventAdapter::KEYDOWN):
        {
            if (ea.getKey()== _key)
            {
                osgViewer::View* myview = dynamic_cast<osgViewer::View*>(&aa);
                if (!myview) return false;

                osgViewer::ViewerBase* viewer = myview->getViewerBase();

                toggle(viewer);

                if (_offlineCollect)
                    CollectStatistics(viewer);

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

    _camera->setViewport(0, 0, width, height);
    if (fabs(height*_statsWidth) <= fabs(width*_statsHeight))
    {
        _camera->setProjectionMatrix(osg::Matrix::ortho2D(_statsWidth - width*_statsHeight/height, _statsWidth,0.0,_statsHeight));
    }
    else
    {
        _camera->setProjectionMatrix(osg::Matrix::ortho2D(0.0,_statsWidth,_statsHeight-height*_statsWidth/width,_statsHeight));
    }
}

void StatsHandler::toggle(osgViewer::ViewerBase *viewer)
{
    if (!_initialized)
    {
        setUpHUDCamera(viewer);
        setUpScene(viewer);
    }

    _statsType = !_statsType;

    if (!_statsType)
    {
        _camera->setNodeMask(0);
        _switch->setAllChildrenOff();

        viewer->getViewerStats()->collectStats("resource", false);
    }
    else
    {
        _camera->setNodeMask(0xffffffff);
        _switch->setSingleChildOn(_resourceStatsChildNum);

        viewer->getViewerStats()->collectStats("resource", true);
    }
}

void StatsHandler::setUpHUDCamera(osgViewer::ViewerBase* viewer)
{
    // Try GraphicsWindow first so we're likely to get the main viewer window
    osg::GraphicsContext* context = dynamic_cast<osgViewer::GraphicsWindow*>(_camera->getGraphicsContext());

    if (!context)
    {
        osgViewer::Viewer::Windows windows;
        viewer->getWindows(windows);

        if (!windows.empty()) context = windows.front();
        else
        {
            // No GraphicsWindows were found, so let's try to find a GraphicsContext
            context = _camera->getGraphicsContext();

            if (!context)
            {
                osgViewer::Viewer::Contexts contexts;
                viewer->getContexts(contexts);

                if (contexts.empty()) return;

                context = contexts.front();
            }
        }
    }

    _camera->setGraphicsContext(context);

    _camera->setRenderOrder(osg::Camera::POST_RENDER, 11);

    _camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    _camera->setViewMatrix(osg::Matrix::identity());
    setWindowSize(context->getTraits()->width, context->getTraits()->height);

    // only clear the depth buffer
    _camera->setClearMask(0);
    _camera->setAllowEventFocus(false);

    _camera->setRenderer(new osgViewer::Renderer(_camera.get()));

    _initialized = true;
}

osg::Geometry* createBackgroundRectangle(const osg::Vec3& pos, const float width, const float height, osg::Vec4& color)
{
    osg::StateSet *ss = new osg::StateSet;

    osg::Geometry* geometry = new osg::Geometry;

    geometry->setUseDisplayList(false);
    geometry->setStateSet(ss);

    osg::Vec3Array* vertices = new osg::Vec3Array;
    geometry->setVertexArray(vertices);

    vertices->push_back(osg::Vec3(pos.x(), pos.y(), 0));
    vertices->push_back(osg::Vec3(pos.x(), pos.y()-height,0));
    vertices->push_back(osg::Vec3(pos.x()+width, pos.y()-height,0));
    vertices->push_back(osg::Vec3(pos.x()+width, pos.y(),0));

    osg::Vec4Array* colors = new osg::Vec4Array;
    colors->push_back(color);
    geometry->setColorArray(colors, osg::Array::BIND_OVERALL);

    osg::DrawElementsUShort *base =  new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLE_FAN,0);
    base->push_back(0);
    base->push_back(1);
    base->push_back(2);
    base->push_back(3);

    geometry->addPrimitiveSet(base);

    return geometry;
}

class ResourceStatsTextDrawCallback : public osg::Drawable::DrawCallback
{
public:
    ResourceStatsTextDrawCallback(osg::Stats* stats, const std::vector<std::string>& statNames)
        : mStats(stats)
        , mStatNames(statNames)
    {
    }

    void drawImplementation(osg::RenderInfo& renderInfo,const osg::Drawable* drawable) const override
    {
        if (!mStats) return;

        osgText::Text* text = (osgText::Text*)(drawable);

        std::ostringstream viewStr;
        viewStr.setf(std::ios::left, std::ios::adjustfield);
        viewStr.width(14);
        // Used fixed formatting, as scientific will switch to "...e+.." notation for
        // large numbers of vertices/drawables/etc.
        viewStr.setf(std::ios::fixed);
        viewStr.precision(0);

        unsigned int frameNumber = renderInfo.getState()->getFrameStamp()->getFrameNumber()-1;

        for (const auto& statName : mStatNames.get())
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

    osg::ref_ptr<osg::Stats> mStats;
    std::reference_wrapper<const std::vector<std::string>> mStatNames;
};

void StatsHandler::setUpScene(osgViewer::ViewerBase *viewer)
{
    _switch = new osg::Switch;

    _camera->addChild(_switch);

    osg::StateSet* stateset = _switch->getOrCreateStateSet();
    stateset->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
    stateset->setMode(GL_BLEND,osg::StateAttribute::ON);
    stateset->setMode(GL_DEPTH_TEST,osg::StateAttribute::OFF);
#ifdef OSG_GL1_AVAILABLE
    stateset->setAttribute(new osg::PolygonMode(), osg::StateAttribute::PROTECTED);
#endif

    osg::Vec4 backgroundColor(0.0, 0.0, 0.0f, 0.3);
    osg::Vec4 staticTextColor(1.0, 1.0, 0.0f, 1.0);
    osg::Vec4 dynamicTextColor(1.0, 1.0, 1.0f, 1.0);
    float backgroundMargin = 5;
    float backgroundSpacing = 3;

    // resource stats
    {
        osg::Group* group = new osg::Group;
        group->setCullingActive(false);
        _resourceStatsChildNum = _switch->getNumChildren();
        _switch->addChild(group, false);

        static const std::vector<std::string> statNames({
            "FrameNumber",
            "",
            "Compiling",
            "UnrefQueue",
            "WorkQueue",
            "WorkThread",
            "",
            "Texture",
            "StateSet",
            "Node",
            "Node Instance",
            "Shape",
            "Shape Instance",
            "Image",
            "Nif",
            "Keyframe",
            "",
            "Groundcover Chunk",
            "Object Chunk",
            "Terrain Chunk",
            "Terrain Texture",
            "Land",
            "Composite",
            "",
            "NavMesh UpdateJobs",
            "NavMesh CacheSize",
            "NavMesh UsedTiles",
            "NavMesh CachedTiles",
            "NavMesh CacheHitRate",
            "",
            "Mechanics Actors",
            "Mechanics Objects",
            "",
            "Physics Actors",
            "Physics Objects",
            "Physics HeightFields",
        });

        static const auto longest = std::max_element(statNames.begin(), statNames.end(),
            [] (const std::string& lhs, const std::string& rhs) { return lhs.size() < rhs.size(); });
        const float statNamesWidth = 13 * _characterSize + 2 * backgroundMargin;
        const float statTextWidth = 7 * _characterSize + 2 * backgroundMargin;
        const float statHeight = statNames.size() * _characterSize + 2 * backgroundMargin;
        osg::Vec3 pos(_statsWidth - statNamesWidth - backgroundSpacing - statTextWidth, statHeight, 0.0f);

        group->addChild(createBackgroundRectangle(pos + osg::Vec3(-backgroundMargin, _characterSize + backgroundMargin, 0),
                                                        statNamesWidth,
                                                        statHeight,
                                                        backgroundColor));

        osg::ref_ptr<osgText::Text> staticText = new osgText::Text;
        group->addChild( staticText.get() );
        staticText->setColor(staticTextColor);
        staticText->setFont(_font);
        staticText->setCharacterSize(_characterSize);
        staticText->setPosition(pos);

        std::ostringstream viewStr;
        viewStr.clear();
        viewStr.setf(std::ios::left, std::ios::adjustfield);
        viewStr.width(longest->size());
        for (const auto& statName : statNames)
        {
            viewStr << statName << std::endl;
        }

        staticText->setText(viewStr.str());

        pos.x() += statNamesWidth + backgroundSpacing;

        group->addChild(createBackgroundRectangle(pos + osg::Vec3(-backgroundMargin, _characterSize + backgroundMargin, 0),
                                                        statTextWidth,
                                                        statHeight,
                                                        backgroundColor));

        osg::ref_ptr<osgText::Text> statsText = new osgText::Text;
        group->addChild( statsText.get() );

        statsText->setColor(dynamicTextColor);
        statsText->setFont(_font);
        statsText->setCharacterSize(_characterSize);
        statsText->setPosition(pos);
        statsText->setText("");
        statsText->setDrawCallback(new ResourceStatsTextDrawCallback(viewer->getViewerStats(), statNames));
    }
}


void StatsHandler::getUsage(osg::ApplicationUsage &usage) const
{
    usage.addKeyboardMouseBinding(_key, "On screen resource usage stats.");
}

void CollectStatistics(osgViewer::ViewerBase* viewer)
{
    osgViewer::Viewer::Cameras cameras;
    viewer->getCameras(cameras);
    for (auto* camera : cameras)
    {
        if (collectStatGPU)           camera->getStats()->collectStats("gpu", true);
        if (collectStatRendering)     camera->getStats()->collectStats("rendering", true);
        if (collectStatCameraObjects) camera->getStats()->collectStats("scene", true);
    }
    if (collectStatEvent)         viewer->getViewerStats()->collectStats("event", true);
    if (collectStatFrameRate)     viewer->getViewerStats()->collectStats("frame_rate", true);
    if (collectStatUpdate)        viewer->getViewerStats()->collectStats("update", true);
    if (collectStatResource)      viewer->getViewerStats()->collectStats("resource", true);
    if (collectStatViewerObjects) viewer->getViewerStats()->collectStats("scene", true);
    if (collectStatEngine)        viewer->getViewerStats()->collectStats("engine", true);
}

}
