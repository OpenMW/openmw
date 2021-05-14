#include "blacklist.hpp"

#include <algorithm>

#include <components/misc/stringops.hpp>

bool CSMDoc::Blacklist::isBlacklisted (const CSMWorld::UniversalId& id) const
{
    std::map<CSMWorld::UniversalId::Type, std::vector<std::string> >::const_iterator iter =
        mIds.find (id.getType());

    if (iter==mIds.end())
        return false;

    return std::binary_search (iter->second.begin(), iter->second.end(),
        Misc::StringUtils::lowerCase (id.getId()));
}

void CSMDoc::Blacklist::add (CSMWorld::UniversalId::Type type,
    const std::vector<std::string>& ids)
{
    std::vector<std::string>& list = mIds[type];

    size_t size = list.size();

    list.resize (size+ids.size());

    std::transform (ids.begin(), ids.end(), list.begin()+size, Misc::StringUtils::lowerCase);
    std::sort (list.begin(), list.end());
}
