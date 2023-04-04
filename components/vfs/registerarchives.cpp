#include "registerarchives.hpp"

#include <filesystem>
#include <set>
#include <stdexcept>

#include <components/debug/debuglog.hpp>

#include <components/vfs/bsaarchive.hpp>
#include <components/vfs/filesystemarchive.hpp>
#include <components/vfs/manager.hpp>

namespace VFS
{

    void registerArchives(VFS::Manager* vfs, const Files::Collections& collections,
        const std::vector<std::string>& archives, bool useLooseFiles)
    {
        const Files::PathContainer& dataDirs = collections.getPaths();

        for (std::vector<std::string>::const_iterator archive = archives.begin(); archive != archives.end(); ++archive)
        {
            if (collections.doesExist(*archive))
            {
                // Last BSA has the highest priority
                const auto archivePath = collections.getPath(*archive);
                Log(Debug::Info) << "Adding BSA archive " << archivePath;
                Bsa::BsaVersion bsaVersion = Bsa::BSAFile::detectVersion(archivePath);

                if (bsaVersion == Bsa::BSAVER_COMPRESSED)
                    vfs->addArchive(std::make_unique<ArchiveSelector<Bsa::BSAVER_COMPRESSED>::type>(archivePath));
                else if (bsaVersion == Bsa::BSAVER_BA2_GNRL)
                    vfs->addArchive(std::make_unique<ArchiveSelector<Bsa::BSAVER_BA2_GNRL>::type>(archivePath));
                else if (bsaVersion == Bsa::BSAVER_BA2_DX10)
                    vfs->addArchive(std::make_unique<ArchiveSelector<Bsa::BSAVER_BA2_DX10>::type>(archivePath));
                else if (bsaVersion == Bsa::BSAVER_UNCOMPRESSED)
                    vfs->addArchive(std::make_unique<ArchiveSelector<Bsa::BSAVER_UNCOMPRESSED>::type>(archivePath));
                else
                    throw std::runtime_error("Unknown archive type '" + *archive + "'");
            }
            else
            {
                throw std::runtime_error("Archive '" + *archive + "' not found");
            }
        }

        if (useLooseFiles)
        {
            std::set<std::filesystem::path> seen;
            for (const auto& dataDir : dataDirs)
            {
                if (seen.insert(dataDir).second)
                {
                    Log(Debug::Info) << "Adding data directory " << dataDir;
                    // Last data dir has the highest priority
                    vfs->addArchive(std::make_unique<FileSystemArchive>(dataDir));
                }
                else
                    Log(Debug::Info) << "Ignoring duplicate data directory " << dataDir;
            }
        }

        vfs->buildIndex();
    }

}
