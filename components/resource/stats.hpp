#ifndef OPENMW_COMPONENTS_RESOURCE_STATS_H
#define OPENMW_COMPONENTS_RESOURCE_STATS_H

#include <osgViewer/ViewerEventHandlers>

namespace osgViewer
{
    class ViewerBase;
}

namespace osg
{
    class Switch;
}

namespace osgText
{
    class Font;
}

namespace VFS
{
    class Manager;
}

namespace Resource
{
    class Profiler : public osgViewer::StatsHandler
    {
    public:
        explicit Profiler(bool offlineCollect, const VFS::Manager& vfs);

        bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa) override;

    private:
        void setUpFonts();

        bool mInitFonts = false;
        bool mOfflineCollect;
        osg::ref_ptr<osgText::Font> mTextFont;
    };

    class StatsHandler : public osgGA::GUIEventHandler
    {
    public:
        explicit StatsHandler(bool offlineCollect, const VFS::Manager& vfs);

        bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa) override;

        /** Get the keyboard and mouse usage of this manipulator.*/
        void getUsage(osg::ApplicationUsage& usage) const override;

    private:
        unsigned mPage = 0;
        bool mInitialized = false;
        bool mOfflineCollect;
        osg::ref_ptr<osg::Switch> mSwitch;
        osg::ref_ptr<osg::Camera> mCamera;
        osg::ref_ptr<osgText::Font> mTextFont;
        std::vector<std::string> mStatNames;

        void setWindowSize(int w, int h);

        void toggle(osgViewer::ViewerBase& viewer);

        void setUpHUDCamera(osgViewer::ViewerBase& viewer);

        void setUpScene(osgViewer::ViewerBase& viewer);
    };

    void collectStatistics(osgViewer::ViewerBase& viewer);
}

#endif
