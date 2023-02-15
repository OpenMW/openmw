#ifndef CSM_WOLRD_INFOCOLLECTION_H
#define CSM_WOLRD_INFOCOLLECTION_H

#include <string>
#include <unordered_map>
#include <vector>

#include "collection.hpp"
#include "info.hpp"
#include "record.hpp"

namespace ESM
{
    struct Dialogue;
    class ESMReader;
}

namespace CSMWorld
{
    using InfosByTopic = std::unordered_map<ESM::RefId, std::vector<ESM::RefId>>;

    class InfoCollection : public Collection<Info, IdAccessor<Info>>
    {
    private:
        bool load(const Info& record, bool base);

    public:
        void load(ESM::ESMReader& reader, bool base, const ESM::Dialogue& dialogue, InfosByTopic& infosByTopic);

        std::vector<Record<Info>*> getTopicInfos(const ESM::RefId& topic) const;
    };
}

#endif
