#include <components/debug/debugging.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/esmloader/esmdata.hpp>
#include <components/esmloader/load.hpp>
#include <components/fallback/fallback.hpp>
#include <components/fallback/validate.hpp>
#include <components/files/configurationmanager.hpp>
#include <components/resource/bulletshapemanager.hpp>
#include <components/resource/foreachbulletobject.hpp>
#include <components/resource/imagemanager.hpp>
#include <components/resource/niffilemanager.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/settings/settings.hpp>
#include <components/version/version.hpp>
#include <components/vfs/manager.hpp>
#include <components/vfs/registerarchives.hpp>
#include <components/esm3/readerscache.hpp>
#include <components/platform/platform.hpp>

#include <boost/program_options.hpp>

#include <charconv>
#include <cstddef>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <vector>


namespace
{
    namespace bpo = boost::program_options;

    using StringsVector = std::vector<std::string>;

    bpo::options_description makeOptionsDescription()
    {
        using Fallback::FallbackMap;

        bpo::options_description result;

        result.add_options()
            ("help", "print help message")

            ("version", "print version information and quit")

            ("data", bpo::value<Files::MaybeQuotedPathContainer>()->default_value(Files::MaybeQuotedPathContainer(), "data")
                ->multitoken()->composing(), "set data directories (later directories have higher priority)")

            ("data-local", bpo::value<Files::MaybeQuotedPathContainer::value_type>()->default_value(Files::MaybeQuotedPathContainer::value_type(), ""),
                "set local data directory (highest priority)")

            ("fallback-archive", bpo::value<StringsVector>()->default_value(StringsVector(), "fallback-archive")
                ->multitoken()->composing(), "set fallback BSA archives (later archives have higher priority)")

            ("resources", bpo::value<Files::MaybeQuotedPath>()->default_value(Files::MaybeQuotedPath(), "resources"),
                "set resources directory")

            ("content", bpo::value<StringsVector>()->default_value(StringsVector(), "")
                ->multitoken()->composing(), "content file(s): esm/esp, or omwgame/omwaddon/omwscripts")

            ("fs-strict", bpo::value<bool>()->implicit_value(true)
                ->default_value(false), "strict file system handling (no case folding)")

            ("encoding", bpo::value<std::string>()->
                default_value("win1252"),
                "Character encoding used in OpenMW game messages:\n"
                "\n\twin1250 - Central and Eastern European such as Polish, Czech, Slovak, Hungarian, Slovene, Bosnian, Croatian, Serbian (Latin script), Romanian and Albanian languages\n"
                "\n\twin1251 - Cyrillic alphabet such as Russian, Bulgarian, Serbian Cyrillic and other languages\n"
                "\n\twin1252 - Western European (Latin) alphabet, used by default")

            ("fallback", bpo::value<FallbackMap>()->default_value(FallbackMap(), "")
                ->multitoken()->composing(), "fallback values")
        ;

        Files::ConfigurationManager::addCommonOptions(result);

        return result;
    }

    struct WriteArray
    {
        const float (&mValue)[3];

        friend std::ostream& operator <<(std::ostream& stream, const WriteArray& value)
        {
            for (std::size_t i = 0; i < 2; ++i)
                stream << std::setprecision(std::numeric_limits<float>::max_exponent10) << value.mValue[i] << ", ";
            return stream << std::setprecision(std::numeric_limits<float>::max_exponent10) << value.mValue[2];
        }
    };

    std::string toHex(std::string_view value)
    {
        std::string buffer(value.size() * 2, '0');
        char* out = buffer.data();
        for (const char v : value)
        {
            const std::ptrdiff_t space = static_cast<std::ptrdiff_t>(static_cast<std::uint8_t>(v) <= 0xf);
            const auto [ptr, ec] = std::to_chars(out + space, out + space + 2, static_cast<std::uint8_t>(v), 16);
            if (ec != std::errc())
                throw std::system_error(std::make_error_code(ec));
            out += 2;
        }
        return buffer;
    }

    int runBulletObjectTool(int argc, char *argv[])
    {
        Platform::init();

        bpo::options_description desc = makeOptionsDescription();

        bpo::parsed_options options = bpo::command_line_parser(argc, argv)
            .options(desc).allow_unregistered().run();
        bpo::variables_map variables;

        bpo::store(options, variables);
        bpo::notify(variables);

        if (variables.find("help") != variables.end())
        {
            getRawStdout() << desc << std::endl;
            return 0;
        }

        Files::ConfigurationManager config;

        bpo::variables_map composingVariables = Files::separateComposingVariables(variables, desc);
        config.readConfiguration(variables, desc);
        Files::mergeComposingVariables(variables, composingVariables, desc);

        const std::string encoding(variables["encoding"].as<std::string>());
        Log(Debug::Info) << ToUTF8::encodingUsingMessage(encoding);
        ToUTF8::Utf8Encoder encoder(ToUTF8::calculateEncoding(encoding));

        Files::PathContainer dataDirs(asPathContainer(variables["data"].as<Files::MaybeQuotedPathContainer>()));

        auto local = variables["data-local"].as<Files::MaybeQuotedPathContainer::value_type>();
        if (!local.empty())
            dataDirs.push_back(std::move(local));

        config.filterOutNonExistingPaths(dataDirs);

        const auto fsStrict = variables["fs-strict"].as<bool>();
        const auto resDir = variables["resources"].as<Files::MaybeQuotedPath>();
        Version::Version v = Version::getOpenmwVersion(resDir.string());
        Log(Debug::Info) << v.describe();
        dataDirs.insert(dataDirs.begin(), resDir / "vfs");
        const auto fileCollections = Files::Collections(dataDirs, !fsStrict);
        const auto archives = variables["fallback-archive"].as<StringsVector>();
        const auto contentFiles = variables["content"].as<StringsVector>();

        Fallback::Map::init(variables["fallback"].as<Fallback::FallbackMap>().mMap);

        VFS::Manager vfs(fsStrict);

        VFS::registerArchives(&vfs, fileCollections, archives, true);

        Settings::Manager settings;
        settings.load(config);

        ESM::ReadersCache readers;
        EsmLoader::Query query;
        query.mLoadActivators = true;
        query.mLoadCells = true;
        query.mLoadContainers = true;
        query.mLoadDoors = true;
        query.mLoadGameSettings = true;
        query.mLoadLands = true;
        query.mLoadStatics = true;
        const EsmLoader::EsmData esmData = EsmLoader::loadEsmData(query, contentFiles, fileCollections, readers, &encoder);

        Resource::ImageManager imageManager(&vfs);
        Resource::NifFileManager nifFileManager(&vfs);
        Resource::SceneManager sceneManager(&vfs, &imageManager, &nifFileManager);
        Resource::BulletShapeManager bulletShapeManager(&vfs, &sceneManager, &nifFileManager);

        Resource::forEachBulletObject(readers, vfs, bulletShapeManager, esmData,
            [] (const ESM::Cell& cell, const Resource::BulletObject& object)
            {
                Log(Debug::Verbose) << "Found bullet object in " << (cell.isExterior() ? "exterior" : "interior")
                    << " cell \"" << cell.getDescription() << "\":"
                    << " fileName=\"" << object.mShape->mFileName << '"'
                    << " fileHash=" << toHex(object.mShape->mFileHash)
                    << " collisionShape=" << std::boolalpha << (object.mShape->mCollisionShape == nullptr)
                    << " avoidCollisionShape=" << std::boolalpha << (object.mShape->mAvoidCollisionShape == nullptr)
                    << " position=(" << WriteArray {object.mPosition.pos} << ')'
                    << " rotation=(" << WriteArray {object.mPosition.rot} << ')'
                    << " scale=" << std::setprecision(std::numeric_limits<float>::max_exponent10) << object.mScale;
            });

        Log(Debug::Info) << "Done";

        return 0;
    }
}

int main(int argc, char *argv[])
{
    return wrapApplication(runBulletObjectTool, argc, argv, "BulletObjectTool");
}
