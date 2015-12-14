#include <iostream>

#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <osgDB/Registry>
#include <osgDB/ObjectWrapper>

#include <components/files/configurationmanager.hpp>
#include <components/files/collections.hpp>
#include <components/vfs/manager.hpp>
#include <components/vfs/registerarchives.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/texturemanager.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/resource/keyframemanager.hpp>
#include <components/misc/stringops.hpp>
#include <components/nifosg/nifloader.hpp>

typedef std::vector<std::string> StringsVector;
typedef std::map<std::string, VFS::File*> FileIndexMap;

// The standard boost::filesystem::replace_extension() replaces multiple extensions, which
// is not what we wanted.  We also wanted case insensitive comparisons, so here it is.
bool extensionIs(std::string path, std::string ext) {
    size_t ps = path.size();
    size_t es = ext.size();
    if (ps <= es) return false;
    std::string pext = path.substr(ps - es, es);
    if (Misc::StringUtils::ciEqual(pext, ext)) return true;
    return false;
}

int main(int argc, char **argv) {
    namespace bpo = boost::program_options;
    bpo::options_description desc("Syntax: nifconv <options>\nAllowed options");

    desc.add_options()
        ("help", "print help message")
        ("version", "print version information and quit")
        ("format", bpo::value<std::string>()->default_value("osgb"),
         "format to export (osgt, osgb, or osgx)")
        ("all", bpo::value<bool>()->implicit_value(true)
            ->default_value(false), "export all known NIF assets")
        ("quiet", bpo::value<bool>()->implicit_value(true)
            ->default_value(false), "don't report each conversion")
        ("output", bpo::value<std::string>()->default_value(""),
         "directory prefix to write output files")
        ("fs-strict", bpo::value<bool>()->implicit_value(true)
            ->default_value(false), "strict file system handling (no case folding)")
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
        exit(0);
    }

    // Validate format choice before building the environment.
    std::string format = variables["format"].as<std::string>();
    if (format != "osgb" && format != "osgt" && format != "osgx") {
        std::cerr << "Unrecognized format '" << format << "', valid formats are "
                  << "'osgb', 'osgt', and 'osgx'." << std::endl;
        exit(1);
    }

    // Report each file?
    bool quiet = variables["quiet"].as<bool>();

    boost::filesystem::path outdir("");
    if (variables.count("output") > 0) {
        std::string outdirstr = variables["output"].as<std::string>();
        outdir = boost::filesystem::path(outdirstr);
    }

    Files::ConfigurationManager cfgMgr;
    cfgMgr.readConfiguration(variables, desc);

    bool mFSStrict = variables["fs-strict"].as<bool>();
    if (mFSStrict)
        std::cout << "Strict filenames are being used!" << std::endl;
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
    Resource::KeyframeManager* keyframeManager = mResourceSystem->getKeyframeManager();

    osgDB::Registry* reg = osgDB::Registry::instance();
    osgDB::ReaderWriter* writer = reg->getReaderWriterForExtension(format);
    if (!writer) {
        std::cerr << "Failed to create '" << format << "' writer." << std::endl;
        exit(2);
    }

    std::string ostr = "";
    // Exported objects should reference textures by name, and not be embedded into the
    // OSG stream.
    ostr += "WriteImageHint=UseExternal";

    // "osgconv -O WriteImageHint=WriteOut x.osgb x.osgt" produces these errors:
    // Empty Image::FileName resetting to image.dds
    // OutputStream::writeImage(): Write image data to external file image.dds
    // ostr += " Compressor=zlib";
    osgDB::Options* options = new osgDB::Options(ostr);

    // Notify the texture manager that we want filenames in the osg::Image objects.
    Resource::TextureManager* texmgr = sceneManager->getTextureManager();
    texmgr->setStoreImageFilenames(true);

    // Ask the virtual file system what files are available, make a list of those that end
    // with ".nif".  Use a case insensitive compare regardless of the setting of
    // fs-strict, since we're really just trying to filter by file type.
    std::set<std::string> nif_files;
    if (variables["all"].as<bool>()) {
        BOOST_FOREACH(const FileIndexMap::value_type& pair, mVFS->getIndex()) {
            if (extensionIs(pair.first, ".kf") || extensionIs(pair.first, ".nif")) {
                nif_files.insert(pair.first);
            }
        }
    }

    // The user probably shouldn't specify both --all and explicitly list files, but if
    // they do just add the explicit files to list of files to convert.  Don't filter
    // based on extension with the presumption that the user knows best.  Maybe the file
    // was named incorrectly, and that's what they're trying to fix?
    if (variables.count("nif") > 0) {
        BOOST_FOREACH(const std::string fstr, variables["nif"].as<StringsVector>()) {
            nif_files.insert(fstr);
        }
    }

    // The number of bytes written to std::cerr (or -1 if it's a console stream).  The
    // idea was to check how many bytes we've written as a way to check whether there were
    // problems loading the NIF file or any of it's dependencies.  This approch is a
    // pretty hackish, but it allows us to provide context for which files had problems
    // loading without changing the VFS component.  It turns out that this trick only
    // works if stderr has been redirected to a file, but that's still better then
    // nothing.  I'd like a better way to detect problems loading and converting assets.
    std::streampos error_position;
    BOOST_FOREACH(const std::string fpath, nif_files) {

        // KF files need a little special renaming attention to avoid conflicts with
        // OSG conversions of NIF files with the same basename.
        bool kf_file = false;
        std::string fstr;
        if (extensionIs(fpath, ".kf")) {
            fstr = fpath.substr(0, fpath.size() - 3) + "_kf";
            kf_file = true;
        }
        else {
            if (extensionIs(fpath, ".nif")) {
                fstr = fpath.substr(0, fpath.size() - 4);
            }
            else {
                fstr = fpath;
            }
            kf_file = false;
        }
        fstr += ".";
        fstr += format;

        // Build the output path.
        boost::filesystem::path outpath = outdir / boost::filesystem::path(fstr);

        // Report all conversions unless the user requested that we be quiet.  This also
        // has to go to std::cerr, or it will not be interleaved with the otehr failures
        // in any intelligble way.
        if (!quiet)
            std::cerr << "Converting: '" << fpath << "' -> '" << outpath.string() << "'." << std::endl;

        // Create any directories that are required.
        boost::filesystem::create_directories(outpath.parent_path());

        error_position = std::cerr.tellp();

        // Load kf files differently.  Assume that all other files (regardless of
        // extension) are NIF files.
        osg::ref_ptr<const osg::Object> object;
        if (kf_file) {
            osg::ref_ptr<const NifOsg::KeyframeHolder> kfh = keyframeManager->get(fpath);
            object = kfh;
        }
        else {
            osg::ref_ptr<const osg::Node> node = sceneManager->getTemplate(fpath);
            object = node;
        }

        if (!object) {
            std::cerr << "Complete failure loading: '" << fpath << "'." << std::endl;
            // If there's no osg::Object, there's nothing to export.
            continue;
        }
        if (error_position != std::cerr.tellp()) {
            std::cerr << "Partial failure loading: '" << fpath << "'." << std::endl;
            // We should still try exporting the object because we'll be able to export
            // whatever was loaded successfully.
        }

        // The osgDB::ReaderWriter will also occasionally report errors to std::cerr
        // without actually failing.  Detect that as well.
        error_position = std::cerr.tellp();
        osgDB::ReaderWriter::WriteResult result =
            writer->writeObject(*object, outpath.string(), options);
        if (!result.success()) {
            std::cerr << "Complete failure converting: '" << fpath << "': "
                      << result.message() << " code " << result.status() << std::endl;
            // Don't warn twice (if we also wrote to std::cerr).
            continue;
        }
        if (error_position != std::cerr.tellp()) {
            std::cerr << "Partial failure converting: '" << fpath << "'." << std::endl;
        }
    }
}
