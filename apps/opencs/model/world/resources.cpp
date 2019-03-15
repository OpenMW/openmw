#include "resources.hpp"

#include <sstream>
#include <stdexcept>
#include <algorithm>

#include <components/vfs/manager.hpp>

#include <components/misc/stringops.hpp>

CSMWorld::Resources::Resources (const VFS::Manager* vfs, const std::string& baseDirectory, UniversalId::Type type,
    const char * const *extensions)
: mBaseDirectory (baseDirectory), mType (type)
{
    recreate(vfs, extensions);
}

void CSMWorld::Resources::recreate(const VFS::Manager* vfs, const char * const *extensions)
{
    mFiles.clear();
    mIndex.clear();

    int baseSize = mBaseDirectory.size();

    const std::map<std::string, VFS::File*>& index = vfs->getIndex();
    for (std::map<std::string, VFS::File*>::const_iterator it = index.begin(); it != index.end(); ++it)
    {
        std::string filepath = it->first;
        if (static_cast<int> (filepath.size())<baseSize+1 ||
            filepath.substr (0, baseSize)!=mBaseDirectory ||
            (filepath[baseSize]!='/' && filepath[baseSize]!='\\'))
            continue;

        if (extensions)
        {
            std::string::size_type extensionIndex = filepath.find_last_of ('.');

            if (extensionIndex==std::string::npos)
                continue;

            std::string extension = filepath.substr (extensionIndex+1);

            int i = 0;

            for (; extensions[i]; ++i)
                if (extensions[i]==extension)
                    break;

            if (!extensions[i])
                continue;
        }

        std::string file = filepath.substr (baseSize+1);
        mFiles.push_back (file);
        std::replace (file.begin(), file.end(), '\\', '/');
        mIndex.insert (std::make_pair (
            Misc::StringUtils::lowerCase (file), static_cast<int> (mFiles.size())-1));
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

        throw std::runtime_error (stream.str());
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
