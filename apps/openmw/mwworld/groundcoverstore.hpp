#ifndef GAME_MWWORLD_GROUNDCOVER_STORE_H
#define GAME_MWWORLD_GROUNDCOVER_STORE_H

#include <vector>
#include <string>
#include <map>

namespace ESM
{
    struct ESM_Context;
    struct Static;
    struct Cell;
}

namespace Loading
{
    class Listener;
}

namespace Files
{
    class Collections;
}

namespace ToUTF8
{
    class Utf8Encoder;
}

namespace MWWorld
{
    template <class T>
    class Store;

    class GroundcoverStore
    {
        private:
            std::map<std::string, std::string> mMeshCache;
            std::map<std::pair<int, int>, std::vector<ESM::ESM_Context>> mCellContexts;

        public:
            void init(const Store<ESM::Static>& statics, const Files::Collections& fileCollections,
                const std::vector<std::string>& groundcoverFiles, ToUTF8::Utf8Encoder* encoder,
                Loading::Listener* listener);

            std::string getGroundcoverModel(const std::string& id) const;
            void initCell(ESM::Cell& cell, int cellX, int cellY) const;
    };
}

#endif
