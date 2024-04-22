#include "resources.hpp"

#include <algorithm>
#include <sstream>
#include <stddef.h>
#include <stdexcept>
#include <string_view>
#include <utility>

#include <apps/opencs/model/world/universalid.hpp>

#include <components/misc/strings/lower.hpp>
#include <components/vfs/manager.hpp>
#include <components/vfs/recursivedirectoryiterator.hpp>

CSMWorld::Resources::Resources(
    const VFS::Manager* vfs, const std::string& baseDirectory, UniversalId::Type type, const char* const* extensions)
    : mBaseDirectory(baseDirectory)
    , mType(type)
{
    recreate(vfs, extensions);
}

void CSMWorld::Resources::recreate(const VFS::Manager* vfs, const char* const* extensions)
{
    mFiles.clear();
    mIndex.clear();

    size_t baseSize = mBaseDirectory.size();

    for (const auto& filepath : vfs->getRecursiveDirectoryIterator())
    {
        const std::string_view view = filepath.view();
        if (view.size() < baseSize + 1 || !view.starts_with(mBaseDirectory) || view[baseSize] != '/')
            continue;

        if (extensions)
        {
            const auto extensionIndex = view.find_last_of('.');

            if (extensionIndex == std::string_view::npos)
                continue;

            std::string_view extension = view.substr(extensionIndex + 1);

            int i = 0;

            for (; extensions[i]; ++i)
                if (extensions[i] == extension)
                    break;

            if (!extensions[i])
                continue;
        }

        std::string file(view.substr(baseSize + 1));
        mFiles.push_back(file);
        mIndex.emplace(std::move(file), static_cast<int>(mFiles.size()) - 1);
    }
}

int CSMWorld::Resources::getSize() const
{
    return static_cast<int>(mFiles.size());
}

std::string CSMWorld::Resources::getId(int index) const
{
    return mFiles.at(index);
}

int CSMWorld::Resources::getIndex(const std::string& id) const
{
    int index = searchId(id);

    if (index == -1)
    {
        std::ostringstream stream;
        stream << "Invalid resource: " << mBaseDirectory << '/' << id;

        throw std::runtime_error(stream.str());
    }

    return index;
}

int CSMWorld::Resources::searchId(std::string_view id) const
{
    std::string id2 = Misc::StringUtils::lowerCase(id);

    std::replace(id2.begin(), id2.end(), '\\', '/');

    std::map<std::string, int>::const_iterator iter = mIndex.find(id2);

    if (iter == mIndex.end())
        return -1;

    return iter->second;
}

CSMWorld::UniversalId::Type CSMWorld::Resources::getType() const
{
    return mType;
}
