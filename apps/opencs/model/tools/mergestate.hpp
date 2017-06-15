#ifndef CSM_TOOLS_MERGESTATE_H
#define CSM_TOOLS_MERGESTATE_H

#include <stdint.h>

#include <memory>
#include <map>

#include "../doc/document.hpp"

namespace CSMTools
{
    struct MergeState
    {
        std::unique_ptr<CSMDoc::Document> mTarget;
        CSMDoc::Document& mSource;
        bool mCompleted;
        std::map<std::pair<uint16_t, int>, int> mTextureIndices; // (texture, content file) -> new texture

        MergeState (CSMDoc::Document& source) : mSource (source), mCompleted (false) {}
    };
}

#endif
