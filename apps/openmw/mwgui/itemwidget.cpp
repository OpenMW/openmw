#include "itemwidget.hpp"

#include <MyGUI_FactoryManager.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_TextBox.h>

#include <components/debug/debuglog.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/settings/values.hpp>
#include <components/vfs/manager.hpp>

#include "../mwbase/environment.hpp"

#include "../mwworld/class.hpp"

namespace
{
    std::string getCountString(int count)
    {
        if (count == 1)
            return {};

        // With small text size we can use up to 4 characters, while with large ones - only up to 3.
        if (Settings::gui().mFontSize > 16)
        {
            if (count > 999999999)
                return MyGUI::utility::toString(count / 1000000000) + "b";
            else if (count > 99999999)
                return ">9m";
            else if (count > 999999)
                return MyGUI::utility::toString(count / 1000000) + "m";
            else if (count > 99999)
                return ">9k";
            else if (count > 999)
                return MyGUI::utility::toString(count / 1000) + "k";
            else
                return MyGUI::utility::toString(count);
        }

        if (count > 999999999)
            return MyGUI::utility::toString(count / 1000000000) + "b";
        else if (count > 999999)
            return MyGUI::utility::toString(count / 1000000) + "m";
        else if (count > 9999)
            return MyGUI::utility::toString(count / 1000) + "k";
        else
            return MyGUI::utility::toString(count);
    }
}

namespace MWGui
{
    std::map<std::string, float> ItemWidget::mScales;

    ItemWidget::ItemWidget()
        : mItem(nullptr)
        , mItemShadow(nullptr)
        , mFrame(nullptr)
        , mControllerBorder(nullptr)
        , mText(nullptr)
    {
    }

    void ItemWidget::registerComponents()
    {
        MyGUI::FactoryManager::getInstance().registerFactory<ItemWidget>("Widget");
        MyGUI::FactoryManager::getInstance().registerFactory<SpellWidget>("Widget");
    }

    void ItemWidget::initialiseOverride()
    {
        assignWidget(mItem, "Item");
        if (mItem)
            mItem->setNeedMouseFocus(false);
        assignWidget(mItemShadow, "ItemShadow");
        if (mItemShadow)
            mItemShadow->setNeedMouseFocus(false);
        assignWidget(mFrame, "Frame");
        if (mFrame)
            mFrame->setNeedMouseFocus(false);
        assignWidget(mText, "Text");
        if (mText)
            mText->setNeedMouseFocus(false);
        if (Settings::gui().mControllerMenus)
        {
            assignWidget(mControllerBorder, "ControllerBorder");
            if (mControllerBorder)
                mControllerBorder->setNeedMouseFocus(false);
        }

        Base::initialiseOverride();
    }

    void ItemWidget::setControllerFocus(bool focus)
    {
        if (mControllerBorder)
            mControllerBorder->setVisible(focus);
    }

    void ItemWidget::setCount(int count)
    {
        if (!mText)
            return;
        mText->setCaption(getCountString(count));
    }

    void ItemWidget::setIcon(const std::string& icon)
    {
        if (mCurrentIcon != icon)
        {
            mCurrentIcon = icon;

            if (mItemShadow)
                mItemShadow->setImageTexture(icon);
            if (mItem)
                mItem->setImageTexture(icon);
        }
    }

    void ItemWidget::setFrame(const std::string& frame, const MyGUI::IntCoord& coord)
    {
        if (mFrame)
        {
            mFrame->setImageTile(MyGUI::IntSize(coord.width, coord.height)); // Why is this needed? MyGUI bug?
            mFrame->setImageCoord(coord);
        }

        if (mCurrentFrame != frame)
        {
            mCurrentFrame = frame;
            mFrame->setImageTexture(frame);
        }
    }

    void ItemWidget::setIcon(const MWWorld::Ptr& ptr)
    {
        constexpr VFS::Path::NormalizedView defaultIcon("default icon.tga");
        std::string_view icon = ptr.getClass().getInventoryIcon(ptr);
        if (icon.empty())
            icon = defaultIcon.value();
        const VFS::Manager* const vfs = MWBase::Environment::get().getResourceSystem()->getVFS();
        std::string invIcon = Misc::ResourceHelpers::correctIconPath(VFS::Path::toNormalized(icon), *vfs);
        if (!vfs->exists(invIcon))
        {
            Log(Debug::Error) << "Failed to open image: '" << invIcon << "' not found, falling back to '"
                              << defaultIcon.value() << "'";
            invIcon = Misc::ResourceHelpers::correctIconPath(defaultIcon, *vfs);
        }
        setIcon(invIcon);
    }

    void ItemWidget::setItem(const MWWorld::Ptr& ptr, ItemState state)
    {
        if (!mItem)
            return;

        if (ptr.isEmpty())
        {
            if (mFrame)
                mFrame->setImageTexture({});
            if (mItemShadow)
                mItemShadow->setImageTexture({});
            mItem->setImageTexture({});
            mText->setCaption({});
            mCurrentIcon.clear();
            mCurrentFrame.clear();
            return;
        }

        bool isMagic = !ptr.getClass().getEnchantment(ptr).empty();

        std::string backgroundTex = "textures\\menu_icon";
        if (isMagic)
            backgroundTex += "_magic";
        if (state == None)
        {
            if (!isMagic)
                backgroundTex.clear();
        }
        else if (state == Equip)
        {
            backgroundTex += "_equip";
        }
        else if (state == Barter)
            backgroundTex += "_barter";

        if (!backgroundTex.empty())
            backgroundTex += ".dds";

        float scale = 1.f;
        if (!backgroundTex.empty())
        {
            auto found = mScales.find(backgroundTex);
            if (found == mScales.end())
            {
                // By default, background icons are supposed to use the 42x42 part of 64x64 image.
                // If the image has a different size, we should use a different chunk size
                // Cache result to do not retrieve background texture every frame.
                MyGUI::ITexture* texture = MyGUI::RenderManager::getInstance().getTexture(backgroundTex);
                if (texture)
                    scale = texture->getHeight() / 64.f;

                mScales[backgroundTex] = scale;
            }
            else
                scale = found->second;
        }

        const int diameter = static_cast<int>(44 * scale);
        if (state == Barter && !isMagic)
            setFrame(backgroundTex,
                MyGUI::IntCoord(static_cast<int>(2 * scale), static_cast<int>(2 * scale), diameter, diameter));
        else
            setFrame(backgroundTex, MyGUI::IntCoord(0, 0, diameter, diameter));

        setIcon(ptr);
    }

    void SpellWidget::setSpellIcon(std::string_view icon)
    {
        if (mFrame && !mCurrentFrame.empty())
        {
            mCurrentFrame.clear();
            mFrame->setImageTexture({});
        }
        if (mCurrentIcon != icon)
        {
            mCurrentIcon = icon;
            if (mItemShadow)
                mItemShadow->setImageTexture(icon);
            if (mItem)
                mItem->setImageTexture(icon);
        }
    }

}
