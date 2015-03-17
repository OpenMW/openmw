#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <components/bsa/bsa_file.hpp>
#include <components/nif/niffile.hpp>

#include <components/nifosg/nifloader.hpp>

#include <components/vfs/manager.hpp>
#include <components/vfs/bsaarchive.hpp>

#include <osgGA/TrackballManipulator>

#include <osgDB/Registry>
#include <osgDB/WriteFile>

#include <osg/PolygonMode>

// EventHandler to toggle wireframe when 'w' key is pressed
class WireframeKeyHandler : public osgGA::GUIEventHandler
{
public:
    WireframeKeyHandler()
        : mWireframe(false)
    {

    }

    virtual bool handle(const osgGA::GUIEventAdapter& adapter,osgGA::GUIActionAdapter& action)
    {
        switch (adapter.getEventType())
        {
        case osgGA::GUIEventAdapter::KEYDOWN:
            if (adapter.getKey() == osgGA::GUIEventAdapter::KEY_W)
            {
                mWireframe = !mWireframe;
                // applying state from an event handler doesn't appear to be safe, so do it in the frame update
                return true;
            }
        default:
            break;
        }
        return false;
    }

    bool getWireframe() const
    {
        return mWireframe;
    }

private:
    bool mWireframe;
};

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " <BSA file> <NIF file>" << std::endl;
        return 1;
    }

    VFS::Manager resourceMgr (false);
    resourceMgr.addArchive(new VFS::BsaArchive(argv[1]));
    resourceMgr.buildIndex();

    Nif::NIFFilePtr nif(new Nif::NIFFile(resourceMgr.get(argv[2]), std::string(argv[2])));

    osgViewer::Viewer viewer;

    osg::ref_ptr<osg::Group> root(new osg::Group());
    root->getOrCreateStateSet()->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
    // To prevent lighting issues with scaled meshes
    root->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);


    //osgDB::writeNodeFile(*newNode, "out.osg");

    std::vector<NifOsg::Controller >  controllers;
    osg::Group* newNode = new osg::Group;
    NifOsg::Loader loader;
    loader.resourceManager = &resourceMgr;
    loader.loadAsSkeleton(nif, newNode);

    for (unsigned int i=0; i<loader.mControllers.size(); ++i)
        controllers.push_back(loader.mControllers[i]);

    for (int x=0; x<1;++x)
    {
        root->addChild(newNode);
    }

    viewer.setSceneData(root);

    viewer.setUpViewInWindow(0, 0, 800, 600);
    viewer.realize();
    viewer.setCameraManipulator(new osgGA::TrackballManipulator());

    WireframeKeyHandler* keyHandler = new WireframeKeyHandler;

    viewer.addEventHandler(keyHandler);
    viewer.addEventHandler(new osgViewer::StatsHandler);

    bool wireframe = false;

    while (!viewer.done())
    {

        if (wireframe != keyHandler->getWireframe())
        {
            wireframe = keyHandler->getWireframe();
            osg::PolygonMode* mode = new osg::PolygonMode;
            mode->setMode(osg::PolygonMode::FRONT_AND_BACK,
                          wireframe ? osg::PolygonMode::LINE : osg::PolygonMode::FILL);
            root->getOrCreateStateSet()->setAttributeAndModes(mode, osg::StateAttribute::ON);
            root->getOrCreateStateSet()->setMode(GL_CULL_FACE, wireframe ? osg::StateAttribute::OFF
                                                                           : osg::StateAttribute::ON);
        }

        viewer.frame();

        for (unsigned int i=0; i<controllers.size(); ++i)
            controllers[i].update();
    }

    return 0;
}
