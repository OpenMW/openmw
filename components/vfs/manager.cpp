#include "manager.hpp"

#include <cassert>
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

    Files::IStreamPtr Manager::find(Path::NormalizedView name) const
    {
        return findNormalized(name.value());
    }

    Files::IStreamPtr Manager::get(const Path::Normalized& name) const
    {
        return getNormalized(name);
    }

    Files::IStreamPtr Manager::get(Path::NormalizedView name) const
    {
        return getNormalized(name.value());
    }

    Files::IStreamPtr Manager::getNormalized(std::string_view normalizedName) const
    {
        assert(Path::isNormalized(normalizedName));
        auto ptr = findNormalized(normalizedName);
        if (ptr == nullptr)
            throw std::runtime_error("Resource '" + std::string(normalizedName) + "' not found");
        return ptr;
    }

    bool Manager::exists(const Path::Normalized& name) const
    {
        return mIndex.find(name) != mIndex.end();
    }

    bool Manager::exists(Path::NormalizedView name) const
    {
        return mIndex.find(name) != mIndex.end();
    }

    std::string Manager::getArchive(const Path::Normalized& name) const
    {
        for (auto it = mArchives.rbegin(); it != mArchives.rend(); ++it)
        {
            if ((*it)->contains(name))
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
            throw std::runtime_error("Resource '" + normalized + "' is not found");
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

    RecursiveDirectoryRange Manager::getRecursiveDirectoryIterator(VFS::Path::NormalizedView path) const
    {
        if (path.value().empty())
            return { mIndex.begin(), mIndex.end() };
        const auto it = mIndex.lower_bound(path);
        if (it == mIndex.end() || !it->first.view().starts_with(path.value()))
            return { it, it };
        std::string copy(path.value());
        ++copy.back();
        return { it, mIndex.lower_bound(copy) };
    }

    RecursiveDirectoryRange Manager::getRecursiveDirectoryIterator() const
    {
        return { mIndex.begin(), mIndex.end() };
    }

    Files::IStreamPtr Manager::findNormalized(std::string_view normalizedPath) const
    {
        assert(Path::isNormalized(normalizedPath));
        const auto it = mIndex.find(normalizedPath);
        if (it == mIndex.end())
            return nullptr;
        return it->second->open();
    }
}
