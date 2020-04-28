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

StatsHandler::StatsHandler():
    _key(osgGA::GUIEventAdapter::KEY_F4),
    _initialized(false),
    _statsType(false),
    _statsWidth(1280.0f),
    _statsHeight(1024.0f),
    _font(""),
    _characterSize(20.0f)
{
    _camera = new osg::Camera;
    _camera->getOrCreateStateSet()->setGlobalDefaults();
    _camera->setRenderer(new osgViewer::Renderer(_camera.get()));
    _camera->setProjectionResizePolicy(osg::Camera::FIXED);

    _resourceStatsChildNum = 0;

    if (osgDB::Registry::instance()->getReaderWriterForExtension("ttf"))
        _font = osgMyGUI::DataManager::getInstance().getDataPath("DejaVuLGCSansMono.ttf");
}

Profiler::Profiler()
{
    if (osgDB::Registry::instance()->getReaderWriterForExtension("ttf"))
        _font = osgMyGUI::DataManager::getInstance().getDataPath("DejaVuLGCSansMono.ttf");
    else
        _font = "";

    setKeyEventTogglesOnScreenStats(osgGA::GUIEventAdapter::KEY_F3);
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

    virtual void drawImplementation(osg::RenderInfo& renderInfo,const osg::Drawable* drawable) const
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

    osg::Vec3 pos(_statsWidth-420.f, _statsHeight-500.0f,0.0f);
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
            "Compiling",
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
            "Terrain Chunk",
            "Terrain Texture",
            "Land",
            "Composite",
            "",
            "UnrefQueue",
            "",
            "NavMesh UpdateJobs",
            "NavMesh CacheSize",
            "NavMesh UsedTiles",
            "NavMesh CachedTiles",
        });

        static const auto longest = std::max_element(statNames.begin(), statNames.end(),
            [] (const std::string& lhs, const std::string& rhs) { return lhs.size() < rhs.size(); });
        const int numLines = statNames.size();
        const float statNamesWidth = 13 * _characterSize + 2 * backgroundMargin;

        group->addChild(createBackgroundRectangle(pos + osg::Vec3(-backgroundMargin, _characterSize + backgroundMargin, 0),
                                                        statNamesWidth,
                                                        numLines * _characterSize + 2 * backgroundMargin,
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
                                                        7 * _characterSize + 2 * backgroundMargin,
                                                        numLines * _characterSize + 2 * backgroundMargin,
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



}
