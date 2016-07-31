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
        if (count > 9999)
            return MyGUI::utility::toString(int(count/1000.f)) + "k";
        else
            return MyGUI::utility::toString(count);
    }
}

namespace MWGui
{

    ItemWidget::ItemWidget()
        : mItem(NULL)
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
        if (mItem)
            mItem->setImageTexture(icon);
    }

    void ItemWidget::setFrame(const std::string &frame, const MyGUI::IntCoord &coord)
    {
        if (mFrame)
        {
            mFrame->setImageTexture(frame);
            mFrame->setImageTile(MyGUI::IntSize(coord.width, coord.height)); // Why is this needed? MyGUI bug?
            mFrame->setImageCoord(coord);
        }
    }

    void ItemWidget::setIcon(const MWWorld::Ptr &ptr)
    {
        setIcon(MWBase::Environment::get().getWindowManager()->correctIconPath(ptr.getClass().getInventoryIcon(ptr)));
    }


    void ItemWidget::setItem(const MWWorld::Ptr &ptr, ItemState state)
    {
        if (!mItem)
            return;

        if (ptr.isEmpty())
        {
            if (mFrame)
                mFrame->setImageTexture("");
            mItem->setImageTexture("");
            mText->setCaption("");
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
