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
        for (std::vector<Archive*>::iterator it = mArchives.begin(); it != mArchives.end(); ++it)
            delete *it;
        mArchives.clear();
    }

    void Manager::addArchive(Archive *archive)
    {
        mArchives.push_back(archive);
    }

    void Manager::buildIndex()
    {
        mMasterIndex.clear();
        mArchiveIndexes.clear();
        mArchiveIndexes.reserve(mArchives.size());

        std::back_insert_iterator<std::vector<NameFileMap> > arch_inserter(mArchiveIndexes);
        for (std::vector<Archive*>::const_iterator it = mArchives.begin(); it != mArchives.end(); ++it)
        {
            NameFileMap index;
            (*it)->listResources(index, mStrict ? &strict_normalize_char : &nonstrict_normalize_char);
            for(NameFileMap::const_iterator nf = index.begin();nf != index.end();++nf)
                mMasterIndex[nf->first] = nf->second;
            *arch_inserter = index;
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
        NameFileMap::const_iterator found = mMasterIndex.find(normalizedName);
        if (found == mMasterIndex.end())
            throw std::runtime_error("Resource '" + normalizedName + "' not found");
        return found->second->open();
    }

    bool Manager::exists(const std::string &name) const
    {
        std::string normalized = name;
        normalize_path(normalized, mStrict);

        return mMasterIndex.find(normalized) != mMasterIndex.end();
    }

    const std::map<std::string, File*>& Manager::getIndex() const
    {
        return mMasterIndex;
    }

    void Manager::normalizeFilename(std::string &name) const
    {
        normalize_path(name, mStrict);
    }

    Files::IStreamPtr Manager::getFirstOf(std::string name) const
    {
        normalize_path(name, mStrict);
        return getFirstOfEntry(name).second->open();
    }

    Files::IStreamPtr Manager::getFirstOfNormalized(const std::string &normalizedName) const
    {
        return getFirstOfEntry(normalizedName).second->open();
    }

    const std::string& Manager::findFirstOf(std::string name) const
    {
        normalize_path(name, mStrict);
        return getFirstOfEntry(name).first;
    }

    const std::string& Manager::findFirstOfNormalized(const std::string &normalizedName) const
    {
        return getFirstOfEntry(normalizedName).first;
    }

    const Manager::NameFileMap::value_type &Manager::getFirstOfEntry(const std::string &normalizedName) const
    {
        size_t extpos = normalizedName.rfind('.');
        if(extpos == std::string::npos || normalizedName.find('/', extpos+1) != std::string::npos)
            extpos = normalizedName.length();
        const std::string basename = normalizedName.substr(0, extpos);

        /* Search in reverse, so later archives take precedence. */
        std::vector<NameFileMap>::const_reverse_iterator iter = mArchiveIndexes.rbegin();
        for(;iter != mArchiveIndexes.rend();++iter)
        {
            NameFileMap::const_iterator entry = iter->lower_bound(basename);
            if(entry != iter->end() && entry->first.length() >= basename.length())
            {
                extpos = entry->first.rfind('.');
                if(extpos == std::string::npos || entry->first.find('/', extpos+1) != std::string::npos)
                    extpos = entry->first.length();
                if(entry->first.compare(0, extpos, basename) == 0)
                    return *entry;
            }
        }

        throw std::runtime_error("Resource '" + normalizedName + "' not found");
    }

}
