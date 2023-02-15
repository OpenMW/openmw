#ifndef CSM_WOLRD_INFO_H
#define CSM_WOLRD_INFO_H

#include <components/esm3/loadinfo.hpp>

namespace CSMWorld
{
    struct Info : public ESM::DialInfo
    {
        ESM::RefId mTopicId;
        ESM::RefId mOriginalId;
    };
}

#endif
