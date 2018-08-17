#include "registerarchives.hpp"

#include <set>
#include <sstream>

#include <components/debug/debuglog.hpp>

#include <components/vfs/manager.hpp>
#include <components/vfs/bsaarchive.hpp>
#include <components/vfs/filesystemarchive.hpp>

namespace VFS
{

    void registerArchives(VFS::Manager *vfs, const Files::Collections &collections, const std::vector<std::string> &archives, bool useLooseFiles)
    {
        const Files::PathContainer& dataDirs = collections.getPaths();

        for (std::vector<std::string>::const_iterator archive = archives.begin(); archive != archives.end(); ++archive)
        {
            if (collections.doesExist(*archive))
            {
                // Last BSA has the highest priority
                const std::string archivePath = collections.getPath(*archive).string();
                Log(Debug::Info) << "Adding BSA archive " << archivePath;

                vfs->addArchive(new BsaArchive(archivePath));
            }
            else
            {
                std::stringstream message;
                message << "Archive '" << *archive << "' not found";
                throw std::runtime_error(message.str());
            }
        }

        if (useLooseFiles)
        {
            std::set<boost::filesystem::path> seen;
            for (Files::PathContainer::const_iterator iter = dataDirs.begin(); iter != dataDirs.end(); ++iter)
            {
                if (seen.insert(*iter).second)
                {
                    Log(Debug::Info) << "Adding data directory " << iter->string();
                    // Last data dir has the highest priority
                    vfs->addArchive(new FileSystemArchive(iter->string()));
                }
                else
                    Log(Debug::Info) << "Ignoring duplicate data directory " << iter->string();
            }
        }

        vfs->buildIndex();
    }

}
