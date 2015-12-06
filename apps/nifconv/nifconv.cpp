#include <iostream>

#include <boost/foreach.hpp>
#include <boost/program_options.hpp>

#include <osgDB/Registry>
#include <osgDB/ObjectWrapper>

#include <components/files/configurationmanager.hpp>
#include <components/files/collections.hpp>
#include <components/vfs/manager.hpp>
#include <components/vfs/registerarchives.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/texturemanager.hpp>
#include <components/resource/scenemanager.hpp>

typedef std::vector<std::string> StringsVector;

//#define ACTIVATE
#ifdef ACTIVATE
extern osgDB::RegisterWrapperProxy NifOsg_NodeUserData_Serializer;
extern osgDB::RegisterWrapperProxy NifOsg_ParticleShooter_Serializer;
extern osgDB::RegisterWrapperProxy NifOsg_TextKeyMapHolder_Serializer;
extern osgDB::RegisterWrapperProxy NifOsg_KeyframeController_Serializer;
//extern osgDB::RegisterWrapperProxy NifOsg_UVController_Serializer;
// bad cast...
//extern osgDB::RegisterWrapperProxy NifOsg_VisController_Serializer;
//extern osgDB::RegisterWrapperProxy NifOsg_GrowFadeAffector_Serializer;
#endif

int main(int argc, char **argv) {
    namespace bpo = boost::program_options;
    bpo::options_description desc("Syntax: nifconv <options>\nAllowed options");

    desc.add_options()
        ("help", "print help message")
        ("version", "print version information and quit")
        ("fs-strict", bpo::value<bool>()->implicit_value(true)
            ->default_value(false), "strict file system handling (no case folding)")
        ("binary", bpo::value<bool>()->implicit_value(true)
            ->default_value(false), "write OSG file in binary format (OSGB)")
        ("data",
         bpo::value<Files::PathContainer>()->default_value(Files::PathContainer(), "data")
            ->multitoken()->composing(),
         "set data directories (later directories have higher priority)")
        ("data-local", bpo::value<std::string>()->default_value(""),
         "set local data directory (highest priority)")
        ("fallback-archive",
         bpo::value<StringsVector>()->default_value(StringsVector(), "fallback-archive")
         ->multitoken(), "set fallback BSA archives (later archives have higher priority)")
        ("nif", bpo::value<StringsVector>(), "NIF file to convert")
        ;

    bpo::positional_options_description p;
    p.add("nif", -1);

    // Runtime options override settings from all configs
    bpo::parsed_options valid_opts = bpo::command_line_parser(argc, argv)
        .options(desc).positional(p).allow_unregistered().run();
    bpo::variables_map variables;
    bpo::store(valid_opts, variables);
    bpo::notify(variables);

    if (variables.count("help")) {
        std::cout << desc << std::endl;
        return false;
    }

    Files::ConfigurationManager cfgMgr;
    cfgMgr.readConfiguration(variables, desc);

    bool mFSStrict = variables["fs-strict"].as<bool>();
    Files::PathContainer dataDirs(variables["data"].as<Files::PathContainer>());

    std::string local(variables["data-local"].as<std::string>());
    if (!local.empty()) {
        dataDirs.push_back(Files::PathContainer::value_type(local));
    }

    cfgMgr.processPaths(dataDirs);

    Files::Collections mFileCollections = Files::Collections (dataDirs, !mFSStrict);

    std::vector<std::string> mArchives;
    std::auto_ptr<Resource::ResourceSystem> mResourceSystem;

    // fallback archives
    StringsVector archives = variables["fallback-archive"].as<StringsVector>();
    BOOST_FOREACH(const std::string a, archives) {
        mArchives.push_back(a);
    }

    // engine.setResourceDir(variables["resources"].as<std::string>());
        
    std::auto_ptr<VFS::Manager> mVFS;
    mVFS.reset(new VFS::Manager(mFSStrict));
    VFS::registerArchives(mVFS.get(), mFileCollections, mArchives, true);
    mResourceSystem.reset(new Resource::ResourceSystem(mVFS.get()));

    Resource::SceneManager* sceneManager = mResourceSystem->getSceneManager();

#ifdef ACTIVATE
    // Register the serializers (simply reference the global object in the library).
    NifOsg_NodeUserData_Serializer.activate();
    NifOsg_ParticleShooter_Serializer.activate();
    NifOsg_TextKeyMapHolder_Serializer.activate();
    NifOsg_KeyframeController_Serializer.activate();
    //NifOsg_UVController_Serializer.activate();
    // bad cast...
    //NifOsg_VisController_Serializer.activate();
    //NifOsg_GrowFadeAffector_Serializer.activate();
#endif
    
    osgDB::Registry* reg = osgDB::Registry::instance();
    osgDB::ReaderWriter* writer = reg->getReaderWriterForExtension("osgt");
    if (!writer) {
        std::cout << "Failed to create OSGB/OSGT writer." << std::endl;
    }

    osgDB::Options* options = NULL;
    bool binary_format = variables["binary"].as<bool>();
    std::string new_extension = ".osgb";
    if (!binary_format) {
        // Documentation was wrong.   Has to be "fileType=Ascii". :-(
        options = new osgDB::Options("fileType=Ascii");
        new_extension = ".osgt";
    }
    
    BOOST_FOREACH(const std::string nfile, variables["nif"].as<StringsVector>()) {
        if (nfile.substr(nfile.size() - 4, 4) != ".nif") {
            std::cout << "File to convert is not a NIF: " << nfile << std::endl;
            continue;
        }
        osg::ref_ptr<const osg::Node> onode = sceneManager->getTemplate(nfile);
        std::string osgfile = nfile.substr(0, nfile.size() - 4) + new_extension;
        
        std::ofstream ostream;
        ostream.open(osgfile.c_str(), std::ofstream::out | std::ofstream::trunc);
        if (ostream.is_open()) {
            osgDB::ReaderWriter::WriteResult result =
                writer->writeNode(*onode, ostream, options);
            if (!result.success()) {
                std::cout << "Error writing " << osgfile << ": "
                          << result.message() << " code " << result.status() << std::endl;
            }
            else {
                std::cout << "Succesfully wrote: " << osgfile << std::endl;
            }
        }
        else {
            std::cout << "Error opening " << osgfile << std::endl;
        }
        ostream.close();
    }
}
