#include <osgViewer/Viewer>

#include <components/bsa/bsa_file.hpp>
#include <components/nif/niffile.hpp>

#include <components/nifosg/nifloader.hpp>

#include <osgGA/TrackballManipulator>

#include <osgDB/Registry>
#include <osgDB/WriteFile>

#include <osg/PolygonMode>

// EventHandler to toggle wireframe when 'w' key is pressed
class EventHandler : public osgGA::GUIEventHandler
{
public:
    EventHandler(osg::Node* node)
        : mWireframe(false)
        , mNode(node)
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
                osg::PolygonMode* mode = new osg::PolygonMode;
                mode->setMode(osg::PolygonMode::FRONT_AND_BACK,
                              mWireframe ? osg::PolygonMode::LINE : osg::PolygonMode::FILL);
                mNode->getOrCreateStateSet()->setAttributeAndModes(mode, osg::StateAttribute::ON);
                mNode->getOrCreateStateSet()->setMode(GL_CULL_FACE, mWireframe ? osg::StateAttribute::OFF
                                                                               : osg::StateAttribute::ON);
                return true;
            }
        default:
            break;
        }
        return false;
    }

private:
    bool mWireframe;
    osg::Node* mNode;
};

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " <BSA file> <NIF file>" << std::endl;
        return 1;
    }

    Bsa::BSAFile bsa;
    bsa.open(argv[1]);

    Nif::NIFFilePtr nif(new Nif::NIFFile(bsa.getFile(argv[2]), std::string(argv[2])));

    osgViewer::Viewer viewer;

    osg::ref_ptr<osg::Group> root(new osg::Group());
    root->getOrCreateStateSet()->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
    // To prevent lighting issues with scaled meshes
    root->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);

    osg::Group* newNode = new osg::Group;
    NifOsg::Loader loader;
    loader.resourceManager = &bsa;
    loader.loadAsSkeleton(nif, newNode);

    //osgDB::writeNodeFile(*newNode, "out.osg");

    for (int x=0; x<1;++x)
    {
        root->addChild(newNode);
    }

    viewer.setSceneData(root);

    viewer.setUpViewInWindow(0, 0, 800, 600);
    viewer.realize();
    viewer.setCameraManipulator(new osgGA::TrackballManipulator());
    viewer.addEventHandler(new EventHandler(root));

    while (!viewer.done())
    {
        viewer.frame();

        for (unsigned int i=0; i<loader.mControllers.size(); ++i)
            loader.mControllers[i].update();
    }
    return 0;
}
