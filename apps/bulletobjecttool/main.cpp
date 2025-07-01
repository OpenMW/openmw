#include <components/debug/debugging.hpp>
#include <components/debug/debuglog.hpp>
#include <components/esm/defs.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/esm3/readerscache.hpp>
#include <components/esmloader/esmdata.hpp>
#include <components/esmloader/load.hpp>
#include <components/fallback/fallback.hpp>
#include <components/fallback/validate.hpp>
#include <components/files/collections.hpp>
#include <components/files/configurationmanager.hpp>
#include <components/files/multidircollection.hpp>
#include <components/misc/strings/conversion.hpp>
#include <components/platform/platform.hpp>
#include <components/resource/bgsmfilemanager.hpp>
#include <components/resource/bulletshape.hpp>
#include <components/resource/bulletshapemanager.hpp>
#include <components/resource/foreachbulletobject.hpp>
#include <components/resource/imagemanager.hpp>
#include <components/resource/niffilemanager.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/settings/settings.hpp>
#include <components/toutf8/toutf8.hpp>
#include <components/version/version.hpp>
#include <components/vfs/manager.hpp>
#include <components/vfs/registerarchives.hpp>

#include <boost/program_options.hpp>

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{
    namespace bpo = boost::program_options;

    using StringsVector = std::vector<std::string>;

    constexpr std::string_view applicationName = "BulletObjectTool";

    bpo::options_description makeOptionsDescription()
    {
        bpo::options_description result;
        auto addOption = result.add_options();
        addOption("help", "print help message");

        addOption("version", "print version information and quit");

        addOption("data",
            bpo::value<Files::MaybeQuotedPathContainer>()
                ->default_value(Files::MaybeQuotedPathContainer(), "data")
                ->multitoken()
                ->composing(),
            "set data directories (later directories have higher priority)");

        addOption("data-local",
            bpo::value<Files::MaybeQuotedPathContainer::value_type>()->default_value(
                Files::MaybeQuotedPathContainer::value_type(), ""),
            "set local data directory (highest priority)");

        addOption("fallback-archive",
            bpo::value<StringsVector>()->default_value(StringsVector(), "fallback-archive")->multitoken()->composing(),
            "set fallback BSA archives (later archives have higher priority)");

        addOption("content", bpo::value<StringsVector>()->default_value(StringsVector(), "")->multitoken()->composing(),
            "content file(s): esm/esp, or omwgame/omwaddon/omwscripts");

        addOption("encoding", bpo::value<std::string>()->default_value("win1252"),
            "Character encoding used in OpenMW game messages:\n"
            "\n\twin1250 - Central and Eastern European such as Polish, Czech, Slovak, Hungarian, Slovene, Bosnian, "
            "Croatian, Serbian (Latin script), Romanian and Albanian languages\n"
            "\n\twin1251 - Cyrillic alphabet such as Russian, Bulgarian, Serbian Cyrillic and other languages\n"
            "\n\twin1252 - Western European (Latin) alphabet, used by default");

        addOption("fallback",
            bpo::value<Fallback::FallbackMap>()->default_value(Fallback::FallbackMap(), "")->multitoken()->composing(),
            "fallback values");

        Files::ConfigurationManager::addCommonOptions(result);

        return result;
    }

    struct WriteArray
    {
        const float (&mValue)[3];

        friend std::ostream& operator<<(std::ostream& stream, const WriteArray& value)
        {
            for (std::size_t i = 0; i < 2; ++i)
                stream << std::setprecision(std::numeric_limits<float>::max_exponent10) << value.mValue[i] << ", ";
            return stream << std::setprecision(std::numeric_limits<float>::max_exponent10) << value.mValue[2];
        }
    };

    int runBulletObjectTool(int argc, char* argv[])
    {
        Platform::init();

        bpo::options_description desc = makeOptionsDescription();

        bpo::parsed_options options = bpo::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
        bpo::variables_map variables;

        bpo::store(options, variables);
        bpo::notify(variables);

        if (variables.find("help") != variables.end())
        {
            Debug::getRawStdout() << desc << std::endl;
            return 0;
        }

        Files::ConfigurationManager config;
        config.processPaths(variables, std::filesystem::current_path());
        config.readConfiguration(variables, desc);

        Debug::setupLogging(config.getLogPath(), applicationName);

        const std::string encoding(variables["encoding"].as<std::string>());
        Log(Debug::Info) << ToUTF8::encodingUsingMessage(encoding);
        ToUTF8::Utf8Encoder encoder(ToUTF8::calculateEncoding(encoding));

        Files::PathContainer dataDirs(asPathContainer(variables["data"].as<Files::MaybeQuotedPathContainer>()));

        auto local = variables["data-local"].as<Files::MaybeQuotedPathContainer::value_type>();
        if (!local.empty())
            dataDirs.push_back(std::move(local));

        config.filterOutNonExistingPaths(dataDirs);

        const auto& resDir = variables["resources"].as<Files::MaybeQuotedPath>();
        Log(Debug::Info) << Version::getOpenmwVersionDescription();
        dataDirs.insert(dataDirs.begin(), resDir / "vfs");
        const Files::Collections fileCollections(dataDirs);
        const auto& archives = variables["fallback-archive"].as<StringsVector>();
        StringsVector contentFiles{ "builtin.omwscripts" };
        const auto& configContentFiles = variables["content"].as<StringsVector>();
        contentFiles.insert(contentFiles.end(), configContentFiles.begin(), configContentFiles.end());

        Fallback::Map::init(variables["fallback"].as<Fallback::FallbackMap>().mMap);

        VFS::Manager vfs;

        VFS::registerArchives(&vfs, fileCollections, archives, true);

        Settings::Manager::load(config);

        ESM::ReadersCache readers;
        EsmLoader::Query query;
        query.mLoadActivators = true;
        query.mLoadCells = true;
        query.mLoadContainers = true;
        query.mLoadDoors = true;
        query.mLoadGameSettings = true;
        query.mLoadLands = true;
        query.mLoadStatics = true;
        const EsmLoader::EsmData esmData
            = EsmLoader::loadEsmData(query, contentFiles, fileCollections, readers, &encoder);

        constexpr double expiryDelay = 0;
        Resource::ImageManager imageManager(&vfs, expiryDelay);
        Resource::NifFileManager nifFileManager(&vfs, &encoder.getStatelessEncoder());
        Resource::BgsmFileManager bgsmFileManager(&vfs, expiryDelay);
        Resource::SceneManager sceneManager(&vfs, &imageManager, &nifFileManager, &bgsmFileManager, expiryDelay);
        Resource::BulletShapeManager bulletShapeManager(&vfs, &sceneManager, &nifFileManager, expiryDelay);

        Resource::forEachBulletObject(
            readers, vfs, bulletShapeManager, esmData, [](const ESM::Cell& cell, const Resource::BulletObject& object) {
                Log(Debug::Verbose) << "Found bullet object in " << (cell.isExterior() ? "exterior" : "interior")
                                    << " cell \"" << cell.getDescription() << "\":"
                                    << " fileName=\"" << object.mShape->mFileName << '"'
                                    << " fileHash=" << Misc::StringUtils::toHex(object.mShape->mFileHash)
                                    << " collisionShape=" << std::boolalpha
                                    << (object.mShape->mCollisionShape == nullptr)
                                    << " avoidCollisionShape=" << std::boolalpha
                                    << (object.mShape->mAvoidCollisionShape == nullptr) << " position=("
                                    << WriteArray{ object.mPosition.pos } << ')' << " rotation=("
                                    << WriteArray{ object.mPosition.rot } << ')'
                                    << " scale=" << std::setprecision(std::numeric_limits<float>::max_exponent10)
                                    << object.mScale;
            });

        Log(Debug::Info) << "Done";

        return 0;
    }
}

int main(int argc, char* argv[])
{
    return Debug::wrapApplication(runBulletObjectTool, argc, argv, applicationName);
}
