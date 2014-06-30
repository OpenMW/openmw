#include "itemwidget.hpp"

#include <MyGUI_FactoryManager.h>
#include <MyGUI_ImageBox.h>

#include "../mwworld/class.hpp"

namespace MWGui
{

    ItemWidget::ItemWidget()
        : mItem(NULL)
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

        Base::initialiseOverride();
    }

    void ItemWidget::setIcon(const std::string &icon)
    {
        // HACK HACK HACK: Don't setImageTexture if it hasn't changed.
        // There is a leak in MyGUI for each setImageTexture on the same widget.
        // http://www.ogre3d.org/addonforums/viewtopic.php?f=17&t=30251
        if (mCurrentItemTexture == icon)
            return;

        mCurrentItemTexture = icon;
        if (mItem)
            mItem->setImageTexture(icon);
    }

    void ItemWidget::setFrame(const std::string &frame, const MyGUI::IntCoord &coord)
    {
        // HACK HACK HACK: Don't setImageTexture if it hasn't changed.
        // There is a leak in MyGUI for each setImageTexture on the same widget.
        // http://www.ogre3d.org/addonforums/viewtopic.php?f=17&t=30251
        if (mCurrentFrameTexture == frame)
            return;

        mCurrentFrameTexture = frame;
        if (mFrame)
        {
            mFrame->setImageTexture(frame);
            mFrame->setImageTile(MyGUI::IntSize(coord.width, coord.height)); // Why is this needed? MyGUI bug?
            mFrame->setImageCoord(coord);
        }
    }

    void ItemWidget::setIcon(const MWWorld::Ptr &ptr)
    {
        // image
        std::string path = std::string("icons\\");
        path += ptr.getClass().getInventoryIcon(ptr);

        std::string::size_type pos = path.rfind(".");
        if(pos != std::string::npos)
            path.erase(pos);
        path.append(".dds");
        setIcon(path);
    }


    void ItemWidget::setItem(const MWWorld::Ptr &ptr, ItemState state)
    {
        if (!mItem)
            return;

        if (ptr.isEmpty())
        {
            if (mFrame)
            {
                // HACK HACK HACK: Don't setImageTexture if it hasn't changed.
                // There is a leak in MyGUI for each setImageTexture on the same widget.
                // http://www.ogre3d.org/addonforums/viewtopic.php?f=17&t=30251
                if (!mCurrentFrameTexture.empty())
                {
                    mFrame->setImageTexture("");
                    mCurrentFrameTexture = "";
                }
            }
            if (!mCurrentItemTexture.empty())
            {
                mCurrentItemTexture = "";
                mItem->setImageTexture("");
            }
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
