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
        Profiler(bool offlineCollect, VFS::Manager* vfs);
        bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa) override;

    private:
        void setUpFonts();

        bool _offlineCollect;
        bool _initFonts;
        osg::ref_ptr<osgText::Font> _textFont;
    };

    class StatsHandler : public osgGA::GUIEventHandler
    {
    public:
        StatsHandler(bool offlineCollect, VFS::Manager* vfs);

        void setKey(int key) { _key = key; }
        int getKey() const { return _key; }

        bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa) override;

        void setWindowSize(int w, int h);

        void toggle(osgViewer::ViewerBase* viewer);

        void setUpHUDCamera(osgViewer::ViewerBase* viewer);
        void setUpScene(osgViewer::ViewerBase* viewer);

        /** Get the keyboard and mouse usage of this manipulator.*/
        void getUsage(osg::ApplicationUsage& usage) const override;

    private:
        osg::ref_ptr<osg::Switch> _switch;
        int _key;
        osg::ref_ptr<osg::Camera>  _camera;
        bool _initialized;
        bool _statsType;
        bool _offlineCollect;

        float                               _statsWidth;
        float                               _statsHeight;

        float                               _characterSize;

        int _resourceStatsChildNum;

        osg::ref_ptr<osgText::Font> _textFont;
    };

    void CollectStatistics(osgViewer::ViewerBase* viewer);

}

#endif
