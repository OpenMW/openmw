#include "resourceskin.hpp"

#include <MyGUI_RenderManager.h>

#include <components/misc/strings/algorithm.hpp>

namespace MWGui
{
    void resizeSkin(MyGUI::xml::ElementPtr _node)
    {
        _node->setAttribute("type", "ResourceSkin");
        if (!_node->findAttribute("size").empty())
            return;

        auto textureName = _node->findAttribute("texture");
        if (textureName.empty())
            return;

        MyGUI::ITexture* texture = MyGUI::RenderManager::getInstance().getTexture(std::string{ textureName });
        if (!texture)
            return;

        MyGUI::IntCoord coord(0, 0, texture->getWidth(), texture->getHeight());
        MyGUI::xml::ElementEnumerator basis = _node->getElementEnumerator();
        const std::string textureSize = std::to_string(coord.width) + " " + std::to_string(coord.height);
        _node->addAttribute("size", textureSize);
        while (basis.next())
        {
            if (basis->getName() != "BasisSkin")
                continue;

            auto basisSkinType = basis->findAttribute("type");
            if (Misc::StringUtils::ciEqual(basisSkinType, "SimpleText"))
                continue;
            bool isTileRect = Misc::StringUtils::ciEqual(basisSkinType, "TileRect");

            if (!basis->findAttribute("offset").empty())
                continue;

            basis->addAttribute("offset", coord);

            MyGUI::xml::ElementEnumerator state = basis->getElementEnumerator();
            while (state.next())
            {
                if (state->getName() == "State")
                {
                    if (!state->findAttribute("offset").empty())
                        continue;

                    state->addAttribute("offset", coord);
                    if (isTileRect)
                    {
                        MyGUI::xml::ElementEnumerator property = state->getElementEnumerator();
                        bool hasTileSize = false;
                        while (property.next("Property"))
                        {
                            if (property->findAttribute("key") != "TileSize")
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
