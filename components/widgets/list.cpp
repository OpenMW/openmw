#include "list.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_Gui.h>
#include <MyGUI_ImageBox.h>

#include <components/misc/strings/algorithm.hpp>

namespace Gui
{

    MWList::MWList()
        : mScrollView(nullptr)
        , mClient(nullptr)
        , mItemHeight(0)
    {
    }

    void MWList::initialiseOverride()
    {
        Base::initialiseOverride();

        assignWidget(mClient, "Client");
        if (mClient == nullptr)
            mClient = this;

        mScrollView
            = mClient->createWidgetReal<MyGUI::ScrollView>("MW_ScrollView", MyGUI::FloatCoord(0.0, 0.0, 1.0, 1.0),
                MyGUI::Align::Top | MyGUI::Align::Left | MyGUI::Align::Stretch, getName() + "_ScrollView");
    }

    void MWList::addItem(std::string_view name, int verticalPadding)
    {
        mItems.emplace_back(name, verticalPadding);
    }

    void MWList::addSeparator()
    {
        addItem({});
    }

    void MWList::adjustSize()
    {
        redraw();
    }

    void MWList::redraw(bool scrollbarShown)
    {
        constexpr int scrollbarShownScrollBarWidth = 20; // fetch this from skin?
        const int scrollBarWidth = scrollbarShown ? scrollbarShownScrollBarWidth : 0;
        int viewPosition = -mScrollView->getViewOffset().top;

        while (mScrollView->getChildCount())
        {
            MyGUI::Gui::getInstance().destroyWidget(mScrollView->getChildAt(0));
        }

        mItemHeight = 0;
        int i = 0;
        for (const auto& item : mItems)
        {
            mItemHeight += item.mVPadding;
            if (!item.mName.empty())
            {
                if (mListItemSkin.empty())
                    return;
                MyGUI::Button* button = mScrollView->createWidget<MyGUI::Button>(mListItemSkin,
                    MyGUI::IntCoord(0, mItemHeight, mScrollView->getSize().width - scrollBarWidth - 2, 24),
                    MyGUI::Align::Left | MyGUI::Align::Top, getName() + "_item_" + item.mName);
                button->setCaption(item.mName);
                button->getSubWidgetText()->setWordWrap(true);
                button->getSubWidgetText()->setTextAlign(MyGUI::Align::Left);
                button->eventMouseWheel += MyGUI::newDelegate(this, &MWList::onMouseWheelMoved);
                button->eventMouseButtonClick += MyGUI::newDelegate(this, &MWList::onItemSelected);
                button->setNeedKeyFocus(true);

                // Morrowind list item text widgets are typically 18 pixels tall
                int height = button->getTextSize().height + 2;
                button->setSize(MyGUI::IntSize(button->getSize().width, height));
                button->setUserData(i);

                mItemHeight += height;
            }
            else
            {
                MyGUI::ImageBox* separator = mScrollView->createWidget<MyGUI::ImageBox>("MW_HLine",
                    MyGUI::IntCoord(2, mItemHeight, mScrollView->getWidth() - scrollBarWidth - 4, 18),
                    MyGUI::Align::Left | MyGUI::Align::Top | MyGUI::Align::HStretch);
                separator->setNeedMouseFocus(false);

                mItemHeight += 18;
            }
            mItemHeight += item.mVPadding;
            ++i;
        }

        // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the
        // scrollbar is hidden
        mScrollView->setVisibleVScroll(false);
        mScrollView->setCanvasSize(mClient->getSize().width, std::max(mItemHeight, mClient->getSize().height));
        mScrollView->setVisibleVScroll(true);

        if (!scrollbarShown && mItemHeight > mClient->getSize().height)
            redraw(true);

        int viewRange = mScrollView->getCanvasSize().height;
        if (viewPosition > viewRange)
            viewPosition = viewRange;
        mScrollView->setViewOffset(MyGUI::IntPoint(0, -viewPosition));
    }

    void MWList::setPropertyOverride(std::string_view _key, std::string_view _value)
    {
        if (_key == "ListItemSkin")
            mListItemSkin = _value;
        else
            Base::setPropertyOverride(_key, _value);
    }

    size_t MWList::getItemCount()
    {
        return mItems.size();
    }

    const std::string& MWList::getItemNameAt(size_t at)
    {
        assert(at < mItems.size() && "List item out of bounds");
        return mItems[at].mName;
    }

    void MWList::sort()
    {
        // A special case for separators is not needed for now
        std::sort(mItems.begin(), mItems.end(),
            [](const auto& left, const auto& right) { return Misc::StringUtils::ciLess(left.mName, right.mName); });
    }

    void MWList::removeItem(const std::string& name)
    {
        auto it = std::find_if(mItems.begin(), mItems.end(), [&name](const auto& item) { return item.mName == name; });
        assert(it != mItems.end());
        mItems.erase(it);
    }

    void MWList::clear()
    {
        mItems.clear();
    }

    void MWList::onMouseWheelMoved(MyGUI::Widget* _sender, int _rel)
    {
        // NB view offset is negative
        if (mScrollView->getViewOffset().top + _rel * 0.3f > 0)
            mScrollView->setViewOffset(MyGUI::IntPoint(0, 0));
        else
            mScrollView->setViewOffset(
                MyGUI::IntPoint(0, static_cast<int>(mScrollView->getViewOffset().top + _rel * 0.3)));
    }

    void MWList::onItemSelected(MyGUI::Widget* _sender)
    {
        std::string name = _sender->castType<MyGUI::Button>()->getCaption();
        int id = *_sender->getUserData<int>();
        eventItemSelected(name, id);
        eventWidgetSelected(_sender);
    }

    MyGUI::Button* MWList::getItemWidget(std::string_view name)
    {
        std::string search = getName() + "_item_";
        search += name;
        return mScrollView->findWidget(search)->castType<MyGUI::Button>();
    }

    void MWList::scrollToTop()
    {
        mScrollView->setViewOffset(MyGUI::IntPoint(0, 0));
    }
}
