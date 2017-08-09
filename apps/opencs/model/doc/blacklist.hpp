#ifndef CSM_DOC_BLACKLIST_H
#define CSM_DOC_BLACKLIST_H

#include <map>
#include <vector>
#include <string>

#include "../world/universalid.hpp"

namespace CSMDoc
{
    /// \brief ID blacklist sorted by UniversalId type
    class Blacklist
    {
            std::map<CSMWorld::UniversalId::Type, std::vector<std::string> > mIds;

        public:

            bool isBlacklisted (const CSMWorld::UniversalId& id) const;

            void add (CSMWorld::UniversalId::Type type, const std::vector<std::string>& ids);
    };
}

#endif
