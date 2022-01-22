#ifndef GAME_MWWORLD_GROUNDCOVER_STORE_H
#define GAME_MWWORLD_GROUNDCOVER_STORE_H

#include <vector>
#include <string>
#include <map>

#include <components/esm3/esmreader.hpp>
#include <components/esmloader/esmdata.hpp>
#include <components/files/collections.hpp>

#include "esmstore.hpp"

namespace MWWorld
{
    class GroundcoverStore
    {
        private:
            std::map<std::string, std::string> mMeshCache;
            std::map<std::pair<int, int>, std::vector<ESM::ESM_Context>> mCellContexts;

        public:
            void init(const Store<ESM::Static>& statics, const Files::Collections& fileCollections, const std::vector<std::string>& groundcoverFiles, ToUTF8::Utf8Encoder* encoder);
            std::string getGroundcoverModel(const std::string& id) const;
            void initCell(ESM::Cell& cell, int cellX, int cellY) const;
    };
}

#endif
