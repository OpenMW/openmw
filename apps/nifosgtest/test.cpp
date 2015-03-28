#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <components/bsa/bsa_file.hpp>
#include <components/nif/niffile.hpp>

#include <components/nifosg/nifloader.hpp>

#include <components/vfs/manager.hpp>
#include <components/vfs/bsaarchive.hpp>
#include <components/vfs/filesystemarchive.hpp>

#include <components/resource/texturemanager.hpp>

#include <components/files/configurationmanager.hpp>

#include <osgGA/TrackballManipulator>
#include <osg/PositionAttitudeTransform>

#include <osgDB/WriteFile>

#include <osg/PolygonMode>

// EventHandler to toggle wireframe when 'w' key is pressed
class WireframeKeyHandler : public osgGA::GUIEventHandler
{
public:
    WireframeKeyHandler(osg::Node* node)
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

                // Create a new stateset instead of changing the old one, this alleviates the need to set
                // the StateSet to DYNAMIC DataVariance, which would have a performance impact.

                osg::StateSet* stateset = new osg::StateSet;
                stateset->setAttributeAndModes(mode, osg::StateAttribute::ON);
                stateset->setMode(GL_CULL_FACE, mWireframe ? osg::StateAttribute::OFF
                                                 : osg::StateAttribute::ON);

                mNode->setStateSet(stateset);

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
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <NIF file>" << std::endl;
        return 1;
    }

    Files::ConfigurationManager cfgMgr;
    boost::program_options::options_description desc("");
    desc.add_options()
            ("data", boost::program_options::value<Files::PathContainer>()->default_value(Files::PathContainer(), "data")->multitoken()->composing())
            ("fs-strict", boost::program_options::value<bool>()->implicit_value(true)->default_value(false))
            ("fallback-archive", boost::program_options::value<std::vector<std::string> >()->
                default_value(std::vector<std::string>(), "fallback-archive")->multitoken());

    boost::program_options::variables_map variables;
    cfgMgr.readConfiguration(variables, desc);

    std::vector<std::string> archives = variables["fallback-archive"].as<std::vector<std::string> >();
    bool fsStrict = variables["fs-strict"].as<bool>();
    Files::PathContainer dataDirs;
    if (!variables["data"].empty()) {
        dataDirs = Files::PathContainer(variables["data"].as<Files::PathContainer>());
    }

    cfgMgr.processPaths(dataDirs);

    VFS::Manager resourceMgr (fsStrict);
    Files::Collections collections (dataDirs, !fsStrict);

    for (std::vector<std::string>::const_iterator it = archives.begin(); it != archives.end(); ++it)
    {
        std::string filepath = collections.getPath(*it).string();
        resourceMgr.addArchive(new VFS::BsaArchive(filepath));
    }
    for (Files::PathContainer::const_iterator it = dataDirs.begin(); it != dataDirs.end(); ++it)
    {
        resourceMgr.addArchive(new VFS::FileSystemArchive(it->string()));
    }

    resourceMgr.buildIndex();

    Nif::NIFFilePtr nif(new Nif::NIFFile(resourceMgr.get(argv[1]), std::string(argv[1])));

    // For NiStencilProperty
    osg::DisplaySettings::instance()->setMinimumNumStencilBits(8);

    osgViewer::Viewer viewer;

    osg::ref_ptr<osg::Group> root(new osg::Group());
    root->getOrCreateStateSet()->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
    // To prevent lighting issues with scaled meshes
    root->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);


    //osgDB::writeNodeFile(*newNode, "out.osg");
    osg::Group* newNode = new osg::Group;
    NifOsg::Loader loader;
    Resource::TextureManager texMgr(&resourceMgr);
    loader.mTextureManager = &texMgr;
    newNode->addChild(loader.load(nif));

    osg::PositionAttitudeTransform* trans = new osg::PositionAttitudeTransform;
    root->addChild(trans);

    for (int x=0; x<1;++x)
    {
        //root->addChild(newNode);
        trans->addChild(newNode);
    }

    viewer.setSceneData(root);

    viewer.setUpViewInWindow(0, 0, 800, 600);
    viewer.realize();
    viewer.setCameraManipulator(new osgGA::TrackballManipulator());
    viewer.addEventHandler(new WireframeKeyHandler(root));

    // Mask to separate cull visitors from update visitors
    viewer.getCamera()->setCullMask(~(0x1));

    viewer.addEventHandler(new osgViewer::StatsHandler);

    while (!viewer.done())
    {
        //trans->setAttitude(osg::Quat(viewer.getFrameStamp()->getSimulationTime()*5, osg::Vec3f(0,0,1)));

        viewer.frame();
    }

    return 0;
}
