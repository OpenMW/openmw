#ifndef OPENMW_LUAUI_RESOURCES
#define OPENMW_LUAUI_RESOURCES

#include <functional>
#include <memory>
#include <string>
#include <string_view>
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
        bool mPersistent = false;
    };

    using CursorResource = CursorData;

    struct CursorKey
    {
        VFS::Path::Normalized mPath;
        int mWidth;
        int mHeight;
        int mHotspotX;
        int mHotspotY;
        bool mPersistent;

        explicit CursorKey(const CursorData& data)
            : mPath(data.mPath)
            , mWidth(static_cast<int>(data.mSize.x()))
            , mHeight(static_cast<int>(data.mSize.y()))
            , mHotspotX(static_cast<int>(data.mHotspot.x()))
            , mHotspotY(static_cast<int>(data.mHotspot.y()))
            , mPersistent(data.mPersistent)
        {
        }

        bool operator==(const CursorKey&) const = default;
    };

    struct CursorKeyHash
    {
        std::size_t operator()(const CursorKey& key) const
        {
            std::size_t result = VFS::Path::Hash{}(key.mPath);
            auto combine = [&result](auto value) {
                result ^= std::hash<decltype(value)>{}(value) + 0x9e3779b9 + (result << 6) + (result >> 2);
            };
            combine(key.mWidth);
            combine(key.mHeight);
            combine(key.mHotspotX);
            combine(key.mHotspotY);
            combine(key.mPersistent);
            return result;
        }
    };

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
            CursorKey key(data);
            if (auto it = mCursors.find(key); it != mCursors.end())
                return it->second;
            data.mName = "lua_cursor_" + std::to_string(++mNextCursorId);
            auto cursor = std::make_shared<CursorResource>(std::move(data));
            mCursors.emplace(std::move(key), cursor);
            return cursor;
        }

        std::vector<std::string> cursorNames() const
        {
            std::vector<std::string> result;
            result.reserve(mCursors.size());
            for (const auto& [_, cursor] : mCursors)
                result.push_back(cursor->mName);
            return result;
        }

        std::vector<std::string> gameCursorNames() const
        {
            std::vector<std::string> result;
            for (const auto& [_, cursor] : mCursors)
            {
                if (!cursor->mPersistent)
                    result.push_back(cursor->mName);
            }
            return result;
        }

        std::shared_ptr<CursorResource> findCursor(std::string_view name) const
        {
            for (const auto& [_, cursor] : mCursors)
            {
                if (cursor->mName == name)
                    return cursor;
            }
            return nullptr;
        }

        void clear()
        {
            mTextures.clear();
            mCursors.clear();
        }

        void clearGameResources()
        {
            mTextures.clear();
            std::erase_if(mCursors, [](const auto& entry) { return !entry.second->mPersistent; });
        }

    private:
        using TextureResources = std::vector<std::shared_ptr<TextureResource>>;
        using CursorResources = std::unordered_map<CursorKey, std::shared_ptr<CursorResource>, CursorKeyHash>;
        std::unordered_map<VFS::Path::Normalized, TextureResources, VFS::Path::Hash> mTextures;
        CursorResources mCursors;
        std::size_t mNextCursorId = 0;
    };
}

#endif // OPENMW_LUAUI_LAYERS
