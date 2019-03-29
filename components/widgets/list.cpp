#include "list.hpp"

#include <MyGUI_Gui.h>
#include <MyGUI_Button.h>
#include <MyGUI_ImageBox.h>

namespace Gui
{

    MWList::MWList() :
        mScrollView(0)
        ,mClient(0)
        , mItemHeight(0)
    {
    }

    void MWList::initialiseOverride()
    {
        Base::initialiseOverride();

        assignWidget(mClient, "Client");
        if (mClient == 0)
            mClient = this;

        mScrollView = mClient->createWidgetReal<MyGUI::ScrollView>(
            "MW_ScrollView", MyGUI::FloatCoord(0.0, 0.0, 1.0, 1.0),
            MyGUI::Align::Top | MyGUI::Align::Left | MyGUI::Align::Stretch, getName() + "_ScrollView");
    }

    void MWList::addItem(const std::string& name)
    {
        mItems.push_back(name);
    }

    void MWList::addSeparator()
    {
        mItems.push_back("");
    }

    void MWList::adjustSize()
    {
        redraw();
    }

    void MWList::redraw(bool scrollbarShown)
    {
        const int _scrollBarWidth = 20; // fetch this from skin?
        const int scrollBarWidth = scrollbarShown ? _scrollBarWidth : 0;
        const int spacing = 3;
        int viewPosition = -mScrollView->getViewOffset().top;

        while (mScrollView->getChildCount())
        {
            MyGUI::Gui::getInstance().destroyWidget(mScrollView->getChildAt(0));
        }

        mItemHeight = 0;
        int i=0;
        for (std::vector<std::string>::const_iterator it=mItems.begin();
            it!=mItems.end(); ++it)
        {
            if (*it != "")
            {
                if (mListItemSkin.empty())
                    return;
                MyGUI::Button* button = mScrollView->createWidget<MyGUI::Button>(
                    mListItemSkin, MyGUI::IntCoord(0, mItemHeight, mScrollView->getSize().width - scrollBarWidth - 2, 24),
                    MyGUI::Align::Left | MyGUI::Align::Top, getName() + "_item_" + (*it));
                button->setCaption((*it));
                button->getSubWidgetText()->setWordWrap(true);
                button->getSubWidgetText()->setTextAlign(MyGUI::Align::Left);
                button->eventMouseWheel += MyGUI::newDelegate(this, &MWList::onMouseWheelMoved);
                button->eventMouseButtonClick += MyGUI::newDelegate(this, &MWList::onItemSelected);
                button->setNeedKeyFocus(true);

                int height = button->getTextSize().height;
                button->setSize(MyGUI::IntSize(button->getSize().width, height));
                button->setUserData(i);

                mItemHeight += height + spacing;
            }
            else
            {
                MyGUI::ImageBox* separator = mScrollView->createWidget<MyGUI::ImageBox>("MW_HLine",
                    MyGUI::IntCoord(2, mItemHeight, mScrollView->getWidth() - scrollBarWidth - 4, 18),
                    MyGUI::Align::Left | MyGUI::Align::Top | MyGUI::Align::HStretch);
                separator->setNeedMouseFocus(false);

                mItemHeight += 18 + spacing;
            }
            ++i;
        }

        // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the scrollbar is hidden
        mScrollView->setVisibleVScroll(false);
        mScrollView->setCanvasSize(mClient->getSize().width, std::max(mItemHeight, mClient->getSize().height));
        mScrollView->setVisibleVScroll(true);

        if (!scrollbarShown && mItemHeight > mClient->getSize().height)
            redraw(true);

        int viewRange = mScrollView->getCanvasSize().height;
        if(viewPosition > viewRange)
            viewPosition = viewRange;
        mScrollView->setViewOffset(MyGUI::IntPoint(0, -viewPosition));
    }

    void MWList::setPropertyOverride(const std::string &_key, const std::string &_value)
    {
        if (_key == "ListItemSkin")
            mListItemSkin = _value;
        else
            Base::setPropertyOverride(_key, _value);
    }

    unsigned int MWList::getItemCount()
    {
        return mItems.size();
    }

    std::string MWList::getItemNameAt(unsigned int at)
    {
        assert(at < mItems.size() && "List item out of bounds");
        return mItems[at];
    }

    void MWList::removeItem(const std::string& name)
    {
        assert( std::find(mItems.begin(), mItems.end(), name) != mItems.end() );
        mItems.erase( std::find(mItems.begin(), mItems.end(), name) );
    }

    void MWList::clear()
    {
        mItems.clear();
    }

    void MWList::onMouseWheelMoved(MyGUI::Widget* _sender, int _rel)
    {
        //NB view offset is negative
        if (mScrollView->getViewOffset().top + _rel*0.3f > 0)
            mScrollView->setViewOffset(MyGUI::IntPoint(0, 0));
        else
            mScrollView->setViewOffset(MyGUI::IntPoint(0, static_cast<int>(mScrollView->getViewOffset().top + _rel*0.3)));
    }

    void MWList::onItemSelected(MyGUI::Widget* _sender)
    {
        std::string name = _sender->castType<MyGUI::Button>()->getCaption();
        int id = *_sender->getUserData<int>();
        eventItemSelected(name, id);
        eventWidgetSelected(_sender);
    }

    MyGUI::Button *MWList::getItemWidget(const std::string& name)
    {
        return mScrollView->findWidget (getName() + "_item_" + name)->castType<MyGUI::Button>();
    }

    void MWList::scrollToTop()
    {
        mScrollView->setViewOffset(MyGUI::IntPoint(0, 0));
    }
}
