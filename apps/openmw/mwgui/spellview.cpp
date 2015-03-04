#include "spellview.hpp"

#include <MyGUI_FactoryManager.h>
#include <MyGUI_ScrollView.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_Gui.h>

#include <components/widgets/sharedstatebutton.hpp>

namespace MWGui
{

    SpellView::SpellView()
        : mShowCostColumn(true)
        , mHighlightSelected(true)
        , mScrollView(NULL)
    {
    }

    void SpellView::initialiseOverride()
    {
        Base::initialiseOverride();

        assignWidget(mScrollView, "ScrollView");
        if (mScrollView == NULL)
            throw std::runtime_error("Item view needs a scroll view");

        mScrollView->setCanvasAlign(MyGUI::Align::Left | MyGUI::Align::Top);
    }

    void SpellView::registerComponents()
    {
        MyGUI::FactoryManager::getInstance().registerFactory<SpellView>("Widget");
    }

    void SpellView::setModel(SpellModel *model)
    {
        mModel.reset(model);
        update();
    }

    SpellModel* SpellView::getModel()
    {
        return mModel.get();
    }

    void SpellView::setShowCostColumn(bool show)
    {
        if (show != mShowCostColumn)
        {
            mShowCostColumn = show;
            update();
        }
    }

    void SpellView::setHighlightSelected(bool highlight)
    {
        if (highlight != mHighlightSelected)
        {
            mHighlightSelected = highlight;
            update();
        }
    }

    void SpellView::update()
    {
        if (!mModel.get())
            return;

        mModel->update();

        int curType = -1;

        const int spellHeight = 18;

        mLines.clear();

        while (mScrollView->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mScrollView->getChildAt(0));

        for (SpellModel::ModelIndex i = 0; i<int(mModel->getItemCount()); ++i)
        {
            const Spell& spell = mModel->getItem(i);
            if (curType != spell.mType)
            {
                if (spell.mType == Spell::Type_Power)
                    addGroup("#{sPowers}", "");
                else if (spell.mType == Spell::Type_Spell)
                    addGroup("#{sSpells}", "#{sCostChance}");
                else
                    addGroup("#{sMagicItem}", "#{sCostCharge}");
                curType = spell.mType;
            }

            const std::string skin = spell.mActive ? "SandTextButton" : "SpellTextUnequipped";

            Gui::SharedStateButton* t = mScrollView->createWidget<Gui::SharedStateButton>(skin,
                MyGUI::IntCoord(0, 0, 0, spellHeight), MyGUI::Align::Left | MyGUI::Align::Top);
            t->setCaption(spell.mName);
            t->setTextAlign(MyGUI::Align::Left);
            adjustSpellWidget(spell, i, t);

            if (!spell.mCostColumn.empty() && mShowCostColumn)
            {
                Gui::SharedStateButton* costChance = mScrollView->createWidget<Gui::SharedStateButton>(skin,
                    MyGUI::IntCoord(0, 0, 0, spellHeight), MyGUI::Align::Left | MyGUI::Align::Top);
                costChance->setCaption(spell.mCostColumn);
                costChance->setTextAlign(MyGUI::Align::Right);
                adjustSpellWidget(spell, i, costChance);

                Gui::ButtonGroup group;
                group.push_back(t);
                group.push_back(costChance);
                Gui::SharedStateButton::createButtonGroup(group);

                mLines.push_back(std::make_pair(t, costChance));
            }
            else
                mLines.push_back(std::make_pair(t, (MyGUI::Widget*)NULL));

            t->setStateSelected(spell.mSelected);
        }

        layoutWidgets();
    }

    void SpellView::layoutWidgets()
    {
        int height = 0;
        for (std::vector< std::pair<MyGUI::Widget*, MyGUI::Widget*> >::iterator it = mLines.begin();
             it != mLines.end(); ++it)
        {
            height += (it->first)->getHeight();
        }

        bool scrollVisible = height > mScrollView->getHeight();
        int width = mScrollView->getWidth() - (scrollVisible ? 18 : 0);

        height = 0;
        for (std::vector< std::pair<MyGUI::Widget*, MyGUI::Widget*> >::iterator it = mLines.begin();
             it != mLines.end(); ++it)
        {
            int lineHeight = (it->first)->getHeight();
            (it->first)->setCoord(4, height, width-8, lineHeight);
            if (it->second)
            {
                (it->second)->setCoord(4, height, width-8, lineHeight);
                MyGUI::TextBox* second = (it->second)->castType<MyGUI::TextBox>(false);
                if (second)
                    (it->first)->setSize(width-8-second->getTextSize().width, lineHeight);
            }

            height += lineHeight;
        }

        // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the scrollbar is hidden
        mScrollView->setVisibleVScroll(false);
        mScrollView->setCanvasSize(mScrollView->getWidth(), std::max(mScrollView->getHeight(), height));
        mScrollView->setVisibleVScroll(true);
    }

    void SpellView::addGroup(const std::string &label, const std::string& label2)
    {
        if (mScrollView->getChildCount() > 0)
        {
            MyGUI::ImageBox* separator = mScrollView->createWidget<MyGUI::ImageBox>("MW_HLine",
                MyGUI::IntCoord(0, 0, mScrollView->getWidth(), 18),
                MyGUI::Align::Left | MyGUI::Align::Top);
            separator->setNeedMouseFocus(false);
            mLines.push_back(std::make_pair(separator, (MyGUI::Widget*)NULL));
        }

        MyGUI::TextBox* groupWidget = mScrollView->createWidget<MyGUI::TextBox>("SandBrightText",
            MyGUI::IntCoord(0, 0, mScrollView->getWidth(), 24),
            MyGUI::Align::Left | MyGUI::Align::Top);
        groupWidget->setCaptionWithReplacing(label);
        groupWidget->setTextAlign(MyGUI::Align::Left);
        groupWidget->setNeedMouseFocus(false);

        if (label2 != "")
        {
            MyGUI::TextBox* groupWidget2 = mScrollView->createWidget<MyGUI::TextBox>("SandBrightText",
                MyGUI::IntCoord(0, 0, mScrollView->getWidth(), 24),
                MyGUI::Align::Left | MyGUI::Align::Top);
            groupWidget2->setCaptionWithReplacing(label2);
            groupWidget2->setTextAlign(MyGUI::Align::Right);
            groupWidget2->setNeedMouseFocus(false);

            mLines.push_back(std::make_pair(groupWidget, groupWidget2));
        }
        else
            mLines.push_back(std::make_pair(groupWidget, (MyGUI::Widget*)NULL));
    }


    void SpellView::setSize(const MyGUI::IntSize &_value)
    {
        bool changed = (_value.width != getWidth() || _value.height != getHeight());
        Base::setSize(_value);
        if (changed)
            layoutWidgets();
    }

    void SpellView::setCoord(const MyGUI::IntCoord &_value)
    {
        bool changed = (_value.width != getWidth() || _value.height != getHeight());
        Base::setCoord(_value);
        if (changed)
            layoutWidgets();
    }

    void SpellView::adjustSpellWidget(const Spell &spell, SpellModel::ModelIndex index, MyGUI::Widget *widget)
    {
        if (spell.mType == Spell::Type_EnchantedItem)
        {
            widget->setUserData(spell.mItem);
            widget->setUserString("ToolTipType", "ItemPtr");
        }
        else
        {
            widget->setUserString("ToolTipType", "Spell");
            widget->setUserString("Spell", spell.mId);
        }

        widget->setUserString("SpellModelIndex", MyGUI::utility::toString(index));

        widget->eventMouseWheel += MyGUI::newDelegate(this, &SpellView::onMouseWheel);
        widget->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellView::onSpellSelected);
    }

    void SpellView::onSpellSelected(MyGUI::Widget* _sender)
    {
        SpellModel::ModelIndex i = MyGUI::utility::parseInt(_sender->getUserString("SpellModelIndex"));
        eventSpellClicked(i);
    }

    void SpellView::onMouseWheel(MyGUI::Widget* _sender, int _rel)
    {
        if (mScrollView->getViewOffset().top + _rel*0.3 > 0)
            mScrollView->setViewOffset(MyGUI::IntPoint(0, 0));
        else
            mScrollView->setViewOffset(MyGUI::IntPoint(0, mScrollView->getViewOffset().top + _rel*0.3));
    }

}
