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

int main(int argc, char **argv) {
    namespace bpo = boost::program_options;
    bpo::options_description desc("Syntax: nifconv <options>\nAllowed options");

    desc.add_options()
        ("help", "print help message")
        ("version", "print version information and quit")
        ("fs-strict", bpo::value<bool>()->implicit_value(true)
            ->default_value(false), "strict file system handling (no case folding)")
        ("binary", bpo::value<bool>()->implicit_value(true)
            ->default_value(true), "write OSG file in binary format (OSGB)")
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

    osgDB::Registry* reg = osgDB::Registry::instance();
    osgDB::ReaderWriter* writer = reg->getReaderWriterForExtension("osgt");
    if (!writer) {
        std::cout << "Failed to create OSGB/OSGT writer." << std::endl;
    }

    bool binary_format = variables["binary"].as<bool>();
    std::string new_extension = ".osgb";

    std::string ostr = "";
    // Exported objects should reference textures by name, and not be embedded into the
    // OSG stream.
    ostr += "WriteImageHint=UseExternal";

    // "osgconv -O WriteImageHint=WriteOut x.osgb x.osgt" produces these errors:
    // Empty Image::FileName resetting to image.dds
    // OutputStream::writeImage(): Write image data to external file image.dds
    if (!binary_format) {
        new_extension = ".osgt";
        // Filetype is automatically when passing the filename instead of a stream.
        //ostr += " fileType=Ascii";
    }
    // ostr += " Compressor=zlib";
    osgDB::Options* options = new osgDB::Options(ostr);

    Resource::TextureManager* texmgr = sceneManager->getTextureManager();
    texmgr->setStoreImageFilenames(true);

    BOOST_FOREACH(const std::string nfile, variables["nif"].as<StringsVector>()) {
        if (nfile.substr(nfile.size() - 4, 4) != ".nif") {
            std::cout << "File to convert is not a NIF: " << nfile << std::endl;
            continue;
        }
        osg::ref_ptr<const osg::Node> onode = sceneManager->getTemplate(nfile);
        std::string osgfile = nfile.substr(0, nfile.size() - 4) + new_extension;
        
        std::cout << "Writing: " << osgfile << std::endl;
        osgDB::ReaderWriter::WriteResult result =
            writer->writeNode(*onode, osgfile, options);
        if (!result.success()) {
            std::cout << "Error writing " << osgfile << ": "
                      << result.message() << " code " << result.status() << std::endl;
        }
    }
}
