#ifndef CSM_WOLRD_INFOCOLLECTION_H
#define CSM_WOLRD_INFOCOLLECTION_H

#include <string>
#include <unordered_map>
#include <vector>

#include "collection.hpp"
#include "info.hpp"

namespace ESM
{
    struct Dialogue;
    class ESMReader;
}

namespace CSMWorld
{
    using InfosByTopic = std::unordered_map<ESM::RefId, std::vector<ESM::RefId>>;
    using InfosRecordPtrByTopic = std::unordered_map<ESM::RefId, std::vector<const Record<Info>*>>;

    class InfoCollection : public Collection<Info>
    {
    private:
        bool load(const Info& record, bool base);

    public:
        void load(ESM::ESMReader& reader, bool base, const ESM::Dialogue& dialogue, InfosByTopic& infosByTopic);

        InfosRecordPtrByTopic getInfosByTopic() const;
    };
}

#endif
