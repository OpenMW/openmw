#ifndef OPENMW_LUAUI_LAYERS
#define OPENMW_LUAUI_LAYERS

#include <string>
#include <string_view>

#include <MyGUI_LayerManager.h>
#include <MyGUI_OverlappedLayer.h>

namespace LuaUi
{
    namespace Layers
    {
        struct Options {
            bool mInteractive;
        };

        size_t size()
        {
            return MyGUI::LayerManager::getInstance().getLayerCount();
        }

        std::string at(size_t index)
        {
            if (index >= size())
                throw std::logic_error("Invalid layer index");
            return MyGUI::LayerManager::getInstance().getLayer(index)->getName();
        }

        size_t indexOf(std::string_view name)
        {
            for (size_t i = 0; i < size(); i++)
                if (at(i) == name)
                    return i;
            return size();
        }

        void insert(size_t index, std::string_view name, Options options)
        {
            if (index > size())
                throw std::logic_error("Invalid layer index");
            if (indexOf(name) < size())
                Log(Debug::Error) << "Layer \"" << name << "\" already exists";
            else
            {
                auto layer = MyGUI::LayerManager::getInstance()
                                 .createLayerAt(std::string(name), "OverlappedLayer", index);
                auto overlappedLayer = dynamic_cast<MyGUI::OverlappedLayer*>(layer);
                overlappedLayer->setPick(options.mInteractive);
            }
        }
    }
}

#endif // OPENMW_LUAUI_LAYERS
