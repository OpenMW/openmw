#ifndef OPENMW_LUAUI_LAYERS
#define OPENMW_LUAUI_LAYERS

#include <string>
#include <string_view>

#include <MyGUI_LayerManager.h>
#include <MyGUI_OverlappedLayer.h>
#include <osg/Vec2f>

namespace LuaUi
{
    // this wrapper is necessary, because the MyGUI LayerManager
    // stores layers in a vector and their indices could change
    class Layer
    {
    public:
        Layer(size_t index)
            : mName(at(index)->getName())
            , mCachedIndex(index)
        {
        }

        const std::string& name() const noexcept { return mName; }
        const osg::Vec2f size()
        {
            MyGUI::ILayer* p = refresh();
            MyGUI::IntSize size = p->getSize();
            return osg::Vec2f(static_cast<float>(size.width), static_cast<float>(size.height));
        }

        struct Options
        {
            bool mInteractive;
        };

        static size_t count() { return MyGUI::LayerManager::getInstance().getLayerCount(); }

        static size_t indexOf(std::string_view name);

        static void insert(size_t index, std::string_view name, Options options);

    private:
        static MyGUI::ILayer* at(size_t index)
        {
            if (index >= count())
                throw std::logic_error("Invalid layer index");
            return MyGUI::LayerManager::getInstance().getLayer(index);
        }

        MyGUI::ILayer* refresh()
        {
            MyGUI::ILayer* p = at(mCachedIndex);
            if (p->getName() != mName)
            {
                mCachedIndex = indexOf(mName);
                p = at(mCachedIndex);
            }
            return p;
        }
        std::string mName;
        size_t mCachedIndex;
    };
}

#endif // OPENMW_LUAUI_LAYERS
