#include "image.hpp"

#include <MyGUI_RenderManager.h>

namespace LuaUi
{
    void LuaTileRect::_setAlign(const MyGUI::IntSize& _oldsize)
    {
        mCurrentCoord.set(0, 0, mCroppedParent->getWidth(), mCroppedParent->getHeight());
        mAlign = MyGUI::Align::Stretch;
        MyGUI::TileRect::_setAlign(_oldsize);
        mTileSize = mSetTileSize;

        // zero tilesize stands for not tiling
        if (mTileSize.width == 0)
            mTileSize.width = mCoord.width;
        if (mTileSize.height == 0)
            mTileSize.height = mCoord.height;

        // mCoord could be zero, prevent division by 0
        // use arbitrary large numbers to prevent performance issues
        if (mTileSize.width == 0)
            mTileSize.width = 1e7;
        if (mTileSize.height == 0)
            mTileSize.height = 1e7;
    }

    LuaImage::LuaImage()
    {
        changeWidgetSkin("LuaImage");
        mTileRect = dynamic_cast<LuaTileRect*>(getSubWidgetMain());
    }

    void LuaImage::updateProperties()
    {
        setImageTexture(propertyValue("path", std::string()));

        bool tileH = propertyValue("tileH", false);
        bool tileV = propertyValue("tileV", false);
        MyGUI::ITexture* texture = MyGUI::RenderManager::getInstance().getTexture(_getTextureName());
        MyGUI::IntSize textureSize;
        if (texture != nullptr)
            textureSize = MyGUI::IntSize(texture->getWidth(), texture->getHeight());
        mTileRect->updateSize(MyGUI::IntSize(
            tileH ? textureSize.width : 0,
            tileV ? textureSize.height : 0
        ));

        WidgetExtension::updateProperties();
    }
}
