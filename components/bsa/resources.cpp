
#include "resources.hpp"

#include <iostream>

#include <OgreResourceGroupManager.h>
#include <OgreStringConverter.h>

#include "bsa_archive.hpp"

void Bsa::registerResources (const Files::Collections& collections,
    const std::vector<std::string>& archives, bool useLooseFiles, bool fsStrict)
{
    const Files::PathContainer& dataDirs = collections.getPaths();

    int i=0;

    if (useLooseFiles)
        for (Files::PathContainer::const_iterator iter = dataDirs.begin(); iter != dataDirs.end(); ++iter)
        {
            // Last data dir has the highest priority
            std::string groupName = "Data" + Ogre::StringConverter::toString(dataDirs.size()-i, 8, '0');
            Ogre::ResourceGroupManager::getSingleton ().createResourceGroup (groupName);

            std::string dataDirectory = iter->string();
            std::cout << "Data dir " << dataDirectory << std::endl;
            Bsa::addDir(dataDirectory, fsStrict, groupName);
            ++i;
        }

    i=0;
    for (std::vector<std::string>::const_iterator archive = archives.begin(); archive != archives.end(); ++archive)
    {
        if (collections.doesExist(*archive))
        {
            // Last BSA has the highest priority
            std::string groupName = "DataBSA" + Ogre::StringConverter::toString(archives.size()-i, 8, '0');

            Ogre::ResourceGroupManager::getSingleton ().createResourceGroup (groupName);

            const std::string archivePath = collections.getPath(*archive).string();
            std::cout << "Adding BSA archive " << archivePath << std::endl;
            Bsa::addBSA(archivePath, groupName);
            ++i;
        }
        else
        {
            std::stringstream message;
            message << "Archive '" << *archive << "' not found";
            throw std::runtime_error(message.str());
        }
    }
}
