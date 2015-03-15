
#include "resources.hpp"

#include <sstream>
#include <stdexcept>
#include <algorithm>

#include <OgreResourceGroupManager.h>

#include <components/misc/stringops.hpp>

CSMWorld::Resources::Resources (const std::string& baseDirectory, UniversalId::Type type,
    const char * const *extensions)
: mBaseDirectory (baseDirectory), mType (type)
{
    int baseSize = mBaseDirectory.size();

    Ogre::StringVector resourcesGroups =
        Ogre::ResourceGroupManager::getSingleton().getResourceGroups();

    for (Ogre::StringVector::iterator iter (resourcesGroups.begin());
        iter!=resourcesGroups.end(); ++iter)
    {
        if (*iter=="General" || *iter=="Internal" || *iter=="Autodetect")
            continue;

        Ogre::StringVectorPtr resources =
            Ogre::ResourceGroupManager::getSingleton().listResourceNames (*iter);

        for (Ogre::StringVector::const_iterator iter (resources->begin());
            iter!=resources->end(); ++iter)
        {
            if (static_cast<int> (iter->size())<baseSize+1 ||
                iter->substr (0, baseSize)!=mBaseDirectory ||
                ((*iter)[baseSize]!='/' && (*iter)[baseSize]!='\\'))
                continue;

            if (extensions)
            {
                std::string::size_type index = iter->find_last_of ('.');

                if (index==std::string::npos)
                    continue;

                std::string extension = iter->substr (index+1);

                int i = 0;

                for (; extensions[i]; ++i)
                    if (extensions[i]==extension)
                        break;

                if (!extensions[i])
                    continue;
            }

            std::string file = iter->substr (baseSize+1);
            mFiles.push_back (file);
            std::replace (file.begin(), file.end(), '\\', '/');
            mIndex.insert (std::make_pair (
                Misc::StringUtils::lowerCase (file), static_cast<int> (mFiles.size())-1));
        }
    }
}

int CSMWorld::Resources::getSize() const
{
    return mFiles.size();
}

std::string CSMWorld::Resources::getId (int index) const
{
    return mFiles.at (index);
}

int CSMWorld::Resources::getIndex (const std::string& id) const
{
    int index = searchId (id);

    if (index==-1)
    {
        std::ostringstream stream;
        stream << "Invalid resource: " << mBaseDirectory << '/' << id;

        throw std::runtime_error (stream.str().c_str());
    }

    return index;
}

int CSMWorld::Resources::searchId (const std::string& id) const
{
    std::string id2 = Misc::StringUtils::lowerCase (id);

    std::replace (id2.begin(), id2.end(), '\\', '/');

    std::map<std::string, int>::const_iterator iter = mIndex.find (id2);

    if (iter==mIndex.end())
        return -1;

    return iter->second;
}

CSMWorld::UniversalId::Type CSMWorld::Resources::getType() const
{
    return mType;
}
