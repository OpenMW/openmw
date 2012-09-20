#ifndef _GAME_RENDER_GLOBALMAP_H
#define _GAME_RENDER_GLOBALMAP_H

#include <string>

namespace MWRender
{

    class GlobalMap
    {
    public:
        GlobalMap(const std::string& cacheDir);

        void render();

    private:
        std::string mCacheDir;
    };

}

#endif

