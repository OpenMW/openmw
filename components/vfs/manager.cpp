#include "manager.hpp"

#include <algorithm>
#include <stdexcept>

#include <components/files/conversion.hpp>
#include <components/misc/strings/lower.hpp>
#include <components/vfs/recursivedirectoryiterator.hpp>

#include "archive.hpp"
#include "file.hpp"
#include "pathutil.hpp"
#include "recursivedirectoryiterator.hpp"

namespace VFS
{
    Manager::Manager() = default;

    Manager::~Manager() = default;

    void Manager::reset()
    {
        mIndex.clear();
        mArchives.clear();
    }

    void Manager::addArchive(std::unique_ptr<Archive>&& archive)
    {
        mArchives.push_back(std::move(archive));
    }

    void Manager::buildIndex()
    {
        mIndex.clear();

        for (const auto& archive : mArchives)
            archive->listResources(mIndex);
    }

    Files::IStreamPtr Manager::get(std::string_view name) const
    {
        return getNormalized(Path::normalizeFilename(name));
    }

    Files::IStreamPtr Manager::getNormalized(std::string_view normalizedName) const
    {
        const auto found = mIndex.find(normalizedName);
        if (found == mIndex.end())
            throw std::runtime_error("Resource '" + std::string(normalizedName) + "' not found");
        return found->second->open();
    }

    bool Manager::exists(std::string_view name) const
    {
        return mIndex.find(Path::normalizeFilename(name)) != mIndex.end();
    }

    std::string Manager::getArchive(std::string_view name) const
    {
        std::string normalized = Path::normalizeFilename(name);
        for (auto it = mArchives.rbegin(); it != mArchives.rend(); ++it)
        {
            if ((*it)->contains(normalized))
                return (*it)->getDescription();
        }
        return {};
    }

    std::filesystem::path Manager::getAbsoluteFileName(const std::filesystem::path& name) const
    {
        std::string normalized = Files::pathToUnicodeString(name);
        Path::normalizeFilenameInPlace(normalized);

        const auto found = mIndex.find(normalized);
        if (found == mIndex.end())
            throw std::runtime_error("Resource '" + normalized + "' not found");
        return found->second->getPath();
    }

    RecursiveDirectoryRange Manager::getRecursiveDirectoryIterator(std::string_view path) const
    {
        if (path.empty())
            return { mIndex.begin(), mIndex.end() };
        std::string normalized = Path::normalizeFilename(path);
        const auto it = mIndex.lower_bound(normalized);
        if (it == mIndex.end() || !it->first.view().starts_with(normalized))
            return { it, it };
        ++normalized.back();
        return { it, mIndex.lower_bound(normalized) };
    }
}
