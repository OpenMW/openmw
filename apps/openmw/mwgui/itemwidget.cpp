#include "itemwidget.hpp"

#include <MyGUI_FactoryManager.h>
#include <MyGUI_ImageBox.h>
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

    ItemWidget::ItemWidget()
        : mItem(NULL)
        , mItemShadow(NULL)
        , mFrame(NULL)
        , mText(NULL)
    {

    }

    void ItemWidget::registerComponents()
    {
        MyGUI::FactoryManager::getInstance().registerFactory<ItemWidget>("Widget");
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

        if (state == Barter && !isMagic)
            setFrame(backgroundTex, MyGUI::IntCoord(2,2,42,42));
        else
            setFrame(backgroundTex, MyGUI::IntCoord(0,0,42,42));

        setIcon(ptr);
    }

}
