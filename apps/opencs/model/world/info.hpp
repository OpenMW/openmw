#ifndef CSM_WOLRD_INFO_H
#define CSM_WOLRD_INFO_H

#include <components/esm/loadinfo.hpp>

namespace CSMWorld
{
    struct Info : public ESM::DialInfo
    {
        std::string mTopicId;
    };
}

#endif
