#include "image.hpp"

#include <MyGUI_RenderManager.h>

#include "resources.hpp"

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
        if (mTileSize.width <= 0)
            mTileSize.width = 1e7;
        if (mTileSize.height <= 0)
            mTileSize.height = 1e7;
    }

    void LuaImage::initialize()
    {
        changeWidgetSkin("LuaImage");
        mTileRect = dynamic_cast<LuaTileRect*>(getSubWidgetMain());
        WidgetExtension::initialize();
    }

    void LuaImage::updateProperties()
    {
        deleteAllItems();
        TextureResource* resource = propertyValue<TextureResource*>("resource", nullptr);
        MyGUI::IntCoord atlasCoord;
        if (resource)
        {
            atlasCoord = MyGUI::IntCoord(
                static_cast<int>(resource->mOffset.x()),
                static_cast<int>(resource->mOffset.y()),
                static_cast<int>(resource->mSize.x()),
                static_cast<int>(resource->mSize.y()));
            setImageTexture(resource->mPath);
        }

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
        setImageTile(textureSize);

        if (atlasCoord.width == 0)
            atlasCoord.width = textureSize.width;
        if (atlasCoord.height == 0)
            atlasCoord.height = textureSize.height;
        setImageCoord(atlasCoord);

        setColour(propertyValue("color", MyGUI::Colour(1,1,1,1)));

        WidgetExtension::updateProperties();
    }
}
