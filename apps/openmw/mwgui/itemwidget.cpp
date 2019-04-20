#include "itemwidget.hpp"

#include <MyGUI_FactoryManager.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_TextBox.h>

// correctIconPath
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/class.hpp"

namespace
{
    std::string getCountString(int count)
    {
        if (count == 1)
            return "";

        if (count > 999999999)
            return MyGUI::utility::toString(count/1000000000) + "b";
        else if (count > 999999)
            return MyGUI::utility::toString(count/1000000) + "m";
        else if (count > 9999)
            return MyGUI::utility::toString(count/1000) + "k";
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

        Base::initialiseOverride();
    }

    void ItemWidget::setCount(int count)
    {
        if (!mText)
            return;
        mText->setCaption(getCountString(count));
    }

    void ItemWidget::setIcon(const std::string &icon)
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

    void ItemWidget::setFrame(const std::string &frame, const MyGUI::IntCoord &coord)
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

    void ItemWidget::setIcon(const MWWorld::Ptr &ptr)
    {
        std::string invIcon = ptr.getClass().getInventoryIcon(ptr);
        if (invIcon.empty())
            invIcon = "default icon.tga";
        setIcon(MWBase::Environment::get().getWindowManager()->correctIconPath(invIcon));
    }


    void ItemWidget::setItem(const MWWorld::Ptr &ptr, ItemState state)
    {
        if (!mItem)
            return;

        if (ptr.isEmpty())
        {
            if (mFrame)
                mFrame->setImageTexture("");
            if (mItemShadow)
                mItemShadow->setImageTexture("");
            mItem->setImageTexture("");
            mText->setCaption("");
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
                backgroundTex = "";
        }
        else if (state == Equip)
        {
            backgroundTex += "_equip";
        }
        else if (state == Barter)
            backgroundTex += "_barter";

        if (backgroundTex != "")
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

        if (state == Barter && !isMagic)
            setFrame(backgroundTex, MyGUI::IntCoord(2*scale,2*scale,44*scale,44*scale));
        else
            setFrame(backgroundTex, MyGUI::IntCoord(0,0,44*scale,44*scale));

        setIcon(ptr);
    }

    void SpellWidget::setSpellIcon(const std::string& icon)
    {
        if (mFrame && !mCurrentFrame.empty())
        {
            mCurrentFrame.clear();
            mFrame->setImageTexture("");
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
