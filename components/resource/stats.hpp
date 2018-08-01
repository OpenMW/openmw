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

namespace Resource
{
    class Profiler : public osgViewer::StatsHandler
    {
    public:
        Profiler();
    };

    class StatsHandler : public osgGA::GUIEventHandler
    {
    public:
        StatsHandler();

        void setKey(int key) { _key = key; }
        int getKey() const { return _key; }

        bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa);

        void setWindowSize(int w, int h);

        void toggle(osgViewer::ViewerBase* viewer);

        void setUpHUDCamera(osgViewer::ViewerBase* viewer);
        void setUpScene(osgViewer::ViewerBase* viewer);

        /** Get the keyboard and mouse usage of this manipulator.*/
        virtual void getUsage(osg::ApplicationUsage& usage) const;

    private:
        osg::ref_ptr<osg::Switch> _switch;
        int _key;
        osg::ref_ptr<osg::Camera>  _camera;
        bool _initialized;
        bool _statsType;

        float                               _statsWidth;
        float                               _statsHeight;

        std::string                         _font;
        float                               _characterSize;

        int _resourceStatsChildNum;

    };

}

#endif
