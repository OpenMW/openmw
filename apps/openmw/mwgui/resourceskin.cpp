#include "resourceskin.hpp"

#include <MyGUI_RenderManager.h>

#include <components/misc/stringops.hpp>

namespace MWGui
{
    void resizeSkin(MyGUI::xml::ElementPtr _node)
    {
        _node->setAttribute("type", "ResourceSkin");
        const std::string size = _node->findAttribute("size");
        if (!size.empty())
            return;

        const std::string textureName = _node->findAttribute("texture");
        if (textureName.empty())
            return;

        MyGUI::ITexture* texture = MyGUI::RenderManager::getInstance().getTexture(textureName);
        if (!texture)
            return;

        MyGUI::IntCoord coord(0, 0, texture->getWidth(), texture->getHeight());
        MyGUI::xml::ElementEnumerator basis = _node->getElementEnumerator();
        const std::string textureSize = std::to_string(coord.width) + " " +  std::to_string(coord.height);
        _node->addAttribute("size", textureSize);
        while (basis.next())
        {
            if (basis->getName() != "BasisSkin")
                continue;

            const std::string basisSkinType = basis->findAttribute("type");
            if (Misc::StringUtils::ciEqual(basisSkinType, "SimpleText"))
                continue;

            const std::string offset = basis->findAttribute("offset");
            if (!offset.empty())
                continue;

            basis->addAttribute("offset", coord);

            MyGUI::xml::ElementEnumerator state = basis->getElementEnumerator();
            while (state.next())
            {
                if (state->getName() == "State")
                {
                    const std::string stateOffset = state->findAttribute("offset");
                    if (!stateOffset.empty())
                        continue;

                    state->addAttribute("offset", coord);
                    if (Misc::StringUtils::ciEqual(basisSkinType, "TileRect"))
                    {
                        MyGUI::xml::ElementEnumerator property = state->getElementEnumerator();
                        bool hasTileSize = false;
                        while (property.next("Property"))
                        {
                            const std::string key = property->findAttribute("key");
                            if (key != "TileSize")
                                continue;

                            hasTileSize = true;
                        }

                        if (!hasTileSize)
                        {
                            MyGUI::xml::ElementPtr tileSizeProperty = state->createChild("Property");
                            tileSizeProperty->addAttribute("key", "TileSize");
                            tileSizeProperty->addAttribute("value", textureSize);
                        }
                    }
                }
            }
        }
    }

    void AutoSizedResourceSkin::deserialization(MyGUI::xml::ElementPtr _node, MyGUI::Version _version)
    {
        resizeSkin(_node);
        Base::deserialization(_node, _version);
    }
}
