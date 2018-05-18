#include "manager.hpp"

#include <cctype>
#include <stdexcept>

#include <components/misc/stringops.hpp>

#include "archive.hpp"

namespace
{

    char strict_normalize_char(char ch)
    {
        return ch == '\\' ? '/' : ch;
    }

    char nonstrict_normalize_char(char ch)
    {
        return ch == '\\' ? '/' : Misc::StringUtils::toLower(ch);
    }

    void normalize_path(std::string& path, bool strict)
    {
        char (*normalize_char)(char) = strict ? &strict_normalize_char : &nonstrict_normalize_char;
        std::transform(path.begin(), path.end(), path.begin(), normalize_char);
    }

}

namespace VFS
{

    Manager::Manager(bool strict)
        : mStrict(strict)
    {

    }

    Manager::~Manager()
    {
        reset();
    }

    void Manager::reset()
    {
        mIndex.clear();
        mArchiveIndexes.clear();
        for (auto it = mArchives.begin(); it != mArchives.end(); ++it)
            delete *it;
        mArchives.clear();
    }

    void Manager::addArchive(Archive *archive)
    {
        mArchives.push_back(archive);
    }

    void Manager::buildIndex()
    {
        mIndex.clear();
        mArchiveIndexes.clear();
        mArchiveIndexes.reserve(mArchives.size());

        std::back_insert_iterator<std::vector<NameFileMap>> archiveInserter(mArchiveIndexes);
        for (auto it = mArchives.begin(); it != mArchives.end(); ++it)
        {
            NameFileMap index;
            (*it)->listResources(mIndex, mStrict ? &strict_normalize_char : &nonstrict_normalize_char);
            for (NameFileMap::const_iterator nf = index.begin(); nf != index.end(); ++nf)
                mIndex[nf->first] = nf->second;
            *archiveInserter = index;
        }
    }

    Files::IStreamPtr Manager::get(const std::string &name) const
    {
        std::string normalized = name;
        normalize_path(normalized, mStrict);

        return getNormalized(normalized);
    }

    Files::IStreamPtr Manager::getNormalized(const std::string &normalizedName) const
    {
        NameFileMap::const_iterator found = mIndex.find(normalizedName);
        if (found == mIndex.end())
            throw std::runtime_error("Resource '" + normalizedName + "' not found");
        return found->second->open();
    }

    bool Manager::exists(const std::string &name) const
    {
        std::string normalized = name;
        normalize_path(normalized, mStrict);

        return mIndex.find(normalized) != mIndex.end();
    }

    const std::map<std::string, File*>& Manager::getIndex() const
    {
        return mIndex;
    }

    void Manager::normalizeFilename(std::string &name) const
    {
        normalize_path(name, mStrict);
    }

}
