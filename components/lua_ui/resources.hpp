#ifndef OPENMW_LUAUI_RESOURCES
#define OPENMW_LUAUI_RESOURCES

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <osg/Vec2f>

#include <components/vfs/pathutil.hpp>

namespace VFS
{
    class Manager;
}

namespace LuaUi
{
    struct TextureData
    {
        VFS::Path::Normalized mPath;
        osg::Vec2f mOffset;
        osg::Vec2f mSize;
    };

    // will have more/different data when automated atlasing is supported
    using TextureResource = TextureData;

    struct CursorData
    {
        VFS::Path::Normalized mPath;
        osg::Vec2f mSize;
        osg::Vec2f mHotspot;
        std::string mName;
    };

    using CursorResource = CursorData;

    class ResourceManager
    {
    public:
        std::shared_ptr<TextureResource> registerTexture(TextureData data)
        {
            TextureResources& list = mTextures[data.mPath];
            list.push_back(std::make_shared<TextureResource>(std::move(data)));
            return list.back();
        }

        std::shared_ptr<CursorResource> registerCursor(CursorData data)
        {
            data.mName = "lua_cursor_" + std::to_string(++mNextCursorId);
            mCursors.push_back(std::make_shared<CursorResource>(std::move(data)));
            return mCursors.back();
        }

        std::vector<std::string> cursorNames() const
        {
            std::vector<std::string> result;
            result.reserve(mCursors.size());
            for (const auto& cursor : mCursors)
                result.push_back(cursor->mName);
            return result;
        }

        void clear()
        {
            mTextures.clear();
            mCursors.clear();
        }

    private:
        using TextureResources = std::vector<std::shared_ptr<TextureResource>>;
        std::unordered_map<VFS::Path::Normalized, TextureResources, VFS::Path::Hash> mTextures;
        std::vector<std::shared_ptr<CursorResource>> mCursors;
        std::size_t mNextCursorId = 0;
    };
}

#endif // OPENMW_LUAUI_LAYERS
