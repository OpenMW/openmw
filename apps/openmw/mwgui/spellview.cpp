#include "spellview.hpp"

#include <MyGUI_FactoryManager.h>
#include <MyGUI_Gui.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_ScrollView.h>

#include <components/settings/values.hpp>
#include <components/widgets/box.hpp>
#include <components/widgets/sharedstatebutton.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "tooltips.hpp"
#include "windowbase.hpp"

namespace MWGui
{

    SpellView::LineInfo::LineInfo(
        MyGUI::Widget* leftWidget, MyGUI::Widget* rightWidget, SpellModel::ModelIndex spellIndex)
        : mLeftWidget(leftWidget)
        , mRightWidget(rightWidget)
        , mSpellIndex(spellIndex)
    {
    }

    void SpellView::initialiseOverride()
    {
        Base::initialiseOverride();

        assignWidget(mScrollView, "ScrollView");
        if (mScrollView == nullptr)
            throw std::runtime_error("Item view needs a scroll view");

        mScrollView->setCanvasAlign(MyGUI::Align::Left | MyGUI::Align::Top);
    }

    void SpellView::registerComponents()
    {
        MyGUI::FactoryManager::getInstance().registerFactory<SpellView>("Widget");
    }

    void SpellView::setModel(SpellModel* model)
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

        const int spellHeight = Settings::gui().mFontSize + 2;

        mLines.clear();
        mButtons.clear();
        mGroupIndices.clear();

        while (mScrollView->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mScrollView->getChildAt(0));

        for (SpellModel::ModelIndex i = 0; i < int(mModel->getItemCount()); ++i)
        {
            const Spell& spell = mModel->getItem(i);
            if (curType != spell.mType)
            {
                if (spell.mType == Spell::Type_Power)
                    addGroup("#{sPowers}", {});
                else if (spell.mType == Spell::Type_Spell)
                    addGroup("#{sSpells}", mShowCostColumn ? "#{sCostChance}" : "");
                else
                    addGroup("#{sMagicItem}", mShowCostColumn ? "#{sCostCharge}" : "");
                curType = spell.mType;
            }

            std::string skin = spell.mActive ? "SandTextButton" : "SpellTextUnequipped";
            if (Settings::gui().mControllerMenus)
                skin = spell.mActive ? "SpellTextEquippedController" : "SpellTextUnequippedController";
            const std::string captionSuffix = MWGui::ToolTips::getCountString(spell.mCount);

            Gui::SharedStateButton* t = mScrollView->createWidget<Gui::SharedStateButton>(
                skin, MyGUI::IntCoord(0, 0, 0, spellHeight), MyGUI::Align::Left | MyGUI::Align::Top);
            t->setNeedKeyFocus(true);
            t->setCaption(spell.mName + captionSuffix);
            t->setTextAlign(MyGUI::Align::Left);
            adjustSpellWidget(spell, i, t);
            mButtons.emplace_back(t, i);

            if (!spell.mCostColumn.empty() && mShowCostColumn)
            {
                Gui::SharedStateButton* costChance = mScrollView->createWidget<Gui::SharedStateButton>(
                    skin, MyGUI::IntCoord(0, 0, 0, spellHeight), MyGUI::Align::Left | MyGUI::Align::Top);
                costChance->setCaption(spell.mCostColumn);
                costChance->setTextAlign(MyGUI::Align::Right);
                adjustSpellWidget(spell, i, costChance);

                Gui::ButtonGroup group;
                group.push_back(t);
                group.push_back(costChance);
                Gui::SharedStateButton::createButtonGroup(group);

                mLines.emplace_back(t, costChance, i);
            }
            else
                mLines.emplace_back(t, static_cast<MyGUI::Widget*>(nullptr), i);

            t->setStateSelected(spell.mSelected);
        }

        layoutWidgets();
    }

    void SpellView::incrementalUpdate()
    {
        if (!mModel.get())
        {
            return;
        }

        mModel->update();
        bool fullUpdateRequired = false;
        SpellModel::ModelIndex maxSpellIndexFound = -1;
        for (LineInfo& line : mLines)
        {
            // only update the lines that are "updateable"
            SpellModel::ModelIndex spellIndex(line.mSpellIndex);
            if (spellIndex != NoSpellIndex)
            {
                Gui::SharedStateButton* nameButton = reinterpret_cast<Gui::SharedStateButton*>(line.mLeftWidget);

                // match model against line
                // if don't match, then major change has happened, so do a full update
                if (mModel->getItemCount() <= static_cast<size_t>(spellIndex))
                {
                    fullUpdateRequired = true;
                    break;
                }

                // more checking for major change.
                const Spell& spell = mModel->getItem(spellIndex);
                const std::string captionSuffix = MWGui::ToolTips::getCountString(spell.mCount);
                if (nameButton->getCaption() != (spell.mName + captionSuffix))
                {
                    fullUpdateRequired = true;
                    break;
                }
                else
                {
                    maxSpellIndexFound = spellIndex;
                    Gui::SharedStateButton* costButton = reinterpret_cast<Gui::SharedStateButton*>(line.mRightWidget);
                    if ((costButton != nullptr) && (costButton->getCaption() != spell.mCostColumn))
                    {
                        costButton->setCaption(spell.mCostColumn);
                    }
                }
            }
        }

        // special case, look for spells added to model that are beyond last updatable item
        auto topSpellIndex = static_cast<SpellModel::ModelIndex>(mModel->getItemCount() - 1);
        if (fullUpdateRequired || ((0 <= topSpellIndex) && (maxSpellIndexFound < topSpellIndex)))
        {
            update();
        }
    }

    void SpellView::layoutWidgets()
    {
        int height = 0;
        for (LineInfo& line : mLines)
        {
            height += line.mLeftWidget->getHeight();
        }

        bool scrollVisible = height > mScrollView->getHeight();
        int width = mScrollView->getWidth() - (scrollVisible ? 18 : 0);

        height = 0;
        for (LineInfo& line : mLines)
        {
            int lineHeight = line.mLeftWidget->getHeight();
            line.mLeftWidget->setCoord(4, height, width - 8, lineHeight);
            if (line.mRightWidget)
            {
                line.mRightWidget->setCoord(4, height, width - 8, lineHeight);
                MyGUI::TextBox* second = line.mRightWidget->castType<MyGUI::TextBox>(false);
                if (second)
                    line.mLeftWidget->setSize(width - 8 - second->getTextSize().width, lineHeight);
            }

            height += lineHeight;
        }

        if (Settings::gui().mControllerMenus)
        {
            mControllerFocus = std::min(mControllerFocus, mButtons.size());
            updateControllerFocus(mButtons.size(), mControllerFocus);
        }

        // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the
        // scrollbar is hidden
        mScrollView->setVisibleVScroll(false);
        mScrollView->setCanvasSize(mScrollView->getWidth(), std::max(mScrollView->getHeight(), height));
        mScrollView->setVisibleVScroll(true);
    }

    void SpellView::addGroup(const std::string& label, const std::string& label2)
    {
        if (mScrollView->getChildCount() > 0)
        {
            MyGUI::ImageBox* separator = mScrollView->createWidget<MyGUI::ImageBox>(
                "MW_HLine", MyGUI::IntCoord(0, 0, mScrollView->getWidth(), 18), MyGUI::Align::Left | MyGUI::Align::Top);
            separator->setNeedMouseFocus(false);
            mLines.emplace_back(separator, static_cast<MyGUI::Widget*>(nullptr), NoSpellIndex);
        }

        MyGUI::TextBox* groupWidget = mScrollView->createWidget<Gui::TextBox>("SandBrightText",
            MyGUI::IntCoord(0, 0, mScrollView->getWidth(), 24), MyGUI::Align::Left | MyGUI::Align::Top);
        groupWidget->setCaptionWithReplacing(label);
        groupWidget->setTextAlign(MyGUI::Align::Left);
        groupWidget->setNeedMouseFocus(false);

        if (!label2.empty())
        {
            MyGUI::TextBox* groupWidget2 = mScrollView->createWidget<Gui::TextBox>("SandBrightText",
                MyGUI::IntCoord(0, 0, mScrollView->getWidth(), 24), MyGUI::Align::Left | MyGUI::Align::Top);
            groupWidget2->setCaptionWithReplacing(label2);
            groupWidget2->setTextAlign(MyGUI::Align::Right);
            groupWidget2->setNeedMouseFocus(false);

            mLines.emplace_back(groupWidget, groupWidget2, NoSpellIndex);
        }
        else
            mLines.emplace_back(groupWidget, static_cast<MyGUI::Widget*>(nullptr), NoSpellIndex);

        mGroupIndices.push_back(mButtons.size());
    }

    void SpellView::setSize(const MyGUI::IntSize& value)
    {
        bool changed = (value.width != getWidth() || value.height != getHeight());
        Base::setSize(value);
        if (changed)
            layoutWidgets();
    }

    void SpellView::setCoord(const MyGUI::IntCoord& value)
    {
        bool changed = (value.width != getWidth() || value.height != getHeight());
        Base::setCoord(value);
        if (changed)
            layoutWidgets();
    }

    void SpellView::adjustSpellWidget(const Spell& spell, SpellModel::ModelIndex index, MyGUI::Widget* widget)
    {
        if (spell.mType == Spell::Type_EnchantedItem)
        {
            widget->setUserData(MWWorld::Ptr(spell.mItem));
            widget->setUserString("ToolTipType", "ItemPtr");
        }
        else
        {
            widget->setUserString("ToolTipType", "Spell");
            widget->setUserString("Spell", spell.mId.serialize());
        }

        widget->setUserString("SpellModelIndex", MyGUI::utility::toString(index));

        widget->eventMouseWheel += MyGUI::newDelegate(this, &SpellView::onMouseWheelMoved);
        widget->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellView::onSpellSelected);
    }

    SpellModel::ModelIndex SpellView::getSpellModelIndex(MyGUI::Widget* widget)
    {
        return MyGUI::utility::parseInt(widget->getUserString("SpellModelIndex"));
    }

    void SpellView::onSpellSelected(MyGUI::Widget* sender)
    {
        eventSpellClicked(getSpellModelIndex(sender));
    }

    void SpellView::onMouseWheelMoved(MyGUI::Widget* /*sender*/, int rel)
    {
        if (mScrollView->getViewOffset().top + rel * 0.3f > 0)
            mScrollView->setViewOffset(MyGUI::IntPoint(0, 0));
        else
            mScrollView->setViewOffset(
                MyGUI::IntPoint(0, static_cast<int>(mScrollView->getViewOffset().top + rel * 0.3f)));
    }

    void SpellView::resetScrollbars()
    {
        mScrollView->setViewOffset(MyGUI::IntPoint(0, 0));
    }

    void SpellView::setActiveControllerWindow(bool active)
    {
        mControllerActiveWindow = active;
        if (active)
            update();
    }

    void SpellView::onControllerButton(const unsigned char button)
    {
        if (mButtons.empty())
            return;

        size_t prevFocus = mControllerFocus;
        MWBase::WindowManager* winMgr = MWBase::Environment::get().getWindowManager();
        int delta = 0;
        switch (button)
        {
            case SDL_CONTROLLER_BUTTON_A:
                // Select the focused item, if any.
                if (mControllerFocus < mButtons.size())
                {
                    onSpellSelected(mButtons[mControllerFocus].first);
                    MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Menu Click"));
                }
                break;
            case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
                // Toggle info tooltip
                winMgr->setControllerTooltipEnabled(!winMgr->getControllerTooltipEnabled());
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
                winMgr->restoreControllerTooltips();
                delta = -1;
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                winMgr->restoreControllerTooltips();
                delta = 1;
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                winMgr->restoreControllerTooltips();
                delta = -10;
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                winMgr->restoreControllerTooltips();
                delta = 10;
                break;
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
            {
                // Jump to first item in previous group
                size_t prevGroupIndex = 0;
                for (size_t groupIndex : mGroupIndices)
                {
                    if (groupIndex >= mControllerFocus)
                        break;
                    else
                        prevGroupIndex = groupIndex;
                }
                mControllerFocus = prevGroupIndex;
            }
            break;
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
            {
                // Jump to first item in next group
                size_t newFocus = mControllerFocus;
                for (size_t groupIndex : mGroupIndices)
                {
                    if (groupIndex > mControllerFocus)
                    {
                        newFocus = groupIndex;
                        break;
                    }
                }
                // If on last group, jump to bottom of whole list
                if (newFocus == mControllerFocus)
                    newFocus = mButtons.size() - 1;
                mControllerFocus = newFocus;
            }
            break;
            default:
                return;
        }

        mControllerFocus = wrap(mControllerFocus, mButtons.size(), delta);

        if (prevFocus != mControllerFocus)
            updateControllerFocus(prevFocus, mControllerFocus);
        else
            updateControllerFocus(mButtons.size(), mControllerFocus);
    }

    void SpellView::updateControllerFocus(size_t prevFocus, size_t newFocus)
    {
        MWBase::Environment::get().getWindowManager()->setCursorVisible(
            !MWBase::Environment::get().getWindowManager()->getControllerTooltipVisible());

        if (mButtons.empty())
            return;

        if (prevFocus < mButtons.size())
        {
            Gui::SharedStateButton* prev = mButtons[prevFocus].first;
            if (prev)
                prev->onMouseLostFocus(nullptr);
        }

        if (mControllerActiveWindow && newFocus < mButtons.size())
        {
            Gui::SharedStateButton* focused = mButtons[newFocus].first;
            if (focused)
            {
                focused->onMouseSetFocus(nullptr);

                // Scroll the list to keep the active item in view
                int line = mButtons[newFocus].second;
                if (line <= 5)
                    mScrollView->setViewOffset(MyGUI::IntPoint(0, 0));
                else
                {
                    const int lineHeight = focused->getHeight();
                    mScrollView->setViewOffset(MyGUI::IntPoint(0, -lineHeight * (line - 5)));
                }

                MWBase::WindowManager* winMgr = MWBase::Environment::get().getWindowManager();
                winMgr->restoreControllerTooltips();

                if (winMgr->getControllerTooltipVisible())
                    MWBase::Environment::get().getInputManager()->warpMouseToWidget(focused);
            }
        }
    }
}
