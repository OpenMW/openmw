#include "alchemywindow.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_ComboBox.h>
#include <MyGUI_ControllerManager.h>
#include <MyGUI_ControllerRepeatClick.h>
#include <MyGUI_EditBox.h>
#include <MyGUI_Gui.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_UString.h>

#include <components/esm3/loadappa.hpp>
#include <components/esm3/loadingr.hpp>
#include <components/esm3/loadmgef.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/alchemy.hpp"
#include "../mwmechanics/magiceffects.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include <MyGUI_Macros.h>

#include "inventoryitemmodel.hpp"
#include "itemview.hpp"
#include "itemwidget.hpp"
#include "sortfilteritemmodel.hpp"
#include "widgets.hpp"

namespace MWGui
{
    AlchemyWindow::AlchemyWindow()
        : WindowBase("openmw_alchemy_window.layout")
        , mCurrentFilter(FilterType::ByName)
        , mModel(nullptr)
        , mSortModel(nullptr)
        , mAlchemy(std::make_unique<MWMechanics::Alchemy>())
        , mApparatus(4)
        , mIngredients(4)
    {
        getWidget(mCreateButton, "CreateButton");
        getWidget(mCancelButton, "CancelButton");
        getWidget(mIngredients[0], "Ingredient1");
        getWidget(mIngredients[1], "Ingredient2");
        getWidget(mIngredients[2], "Ingredient3");
        getWidget(mIngredients[3], "Ingredient4");
        getWidget(mApparatus[0], "Apparatus1");
        getWidget(mApparatus[1], "Apparatus2");
        getWidget(mApparatus[2], "Apparatus3");
        getWidget(mApparatus[3], "Apparatus4");
        getWidget(mEffectsBox, "CreatedEffects");
        getWidget(mBrewCountEdit, "BrewCount");
        getWidget(mIncreaseButton, "IncreaseButton");
        getWidget(mDecreaseButton, "DecreaseButton");
        getWidget(mNameEdit, "NameEdit");
        getWidget(mItemView, "ItemView");
        getWidget(mFilterValue, "FilterValue");
        getWidget(mFilterType, "FilterType");

        mBrewCountEdit->eventValueChanged += MyGUI::newDelegate(this, &AlchemyWindow::onCountValueChanged);
        mBrewCountEdit->eventEditSelectAccept += MyGUI::newDelegate(this, &AlchemyWindow::onAccept);
        mBrewCountEdit->setMinValue(1);
        mBrewCountEdit->setValue(1);

        mIncreaseButton->eventMouseButtonPressed += MyGUI::newDelegate(this, &AlchemyWindow::onIncreaseButtonPressed);
        mIncreaseButton->eventMouseButtonReleased += MyGUI::newDelegate(this, &AlchemyWindow::onCountButtonReleased);
        mDecreaseButton->eventMouseButtonPressed += MyGUI::newDelegate(this, &AlchemyWindow::onDecreaseButtonPressed);
        mDecreaseButton->eventMouseButtonReleased += MyGUI::newDelegate(this, &AlchemyWindow::onCountButtonReleased);

        mItemView->eventItemClicked += MyGUI::newDelegate(this, &AlchemyWindow::onSelectedItem);

        mIngredients[0]->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onIngredientSelected);
        mIngredients[1]->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onIngredientSelected);
        mIngredients[2]->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onIngredientSelected);
        mIngredients[3]->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onIngredientSelected);

        mApparatus[0]->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onApparatusSelected);
        mApparatus[1]->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onApparatusSelected);
        mApparatus[2]->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onApparatusSelected);
        mApparatus[3]->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onApparatusSelected);

        mCreateButton->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onCreateButtonClicked);
        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::onCancelButtonClicked);

        mNameEdit->eventEditSelectAccept += MyGUI::newDelegate(this, &AlchemyWindow::onAccept);
        mFilterValue->eventComboChangePosition += MyGUI::newDelegate(this, &AlchemyWindow::onFilterChanged);
        mFilterValue->eventEditTextChange += MyGUI::newDelegate(this, &AlchemyWindow::onFilterEdited);
        mFilterType->eventMouseButtonClick += MyGUI::newDelegate(this, &AlchemyWindow::switchFilterType);

        if (Settings::gui().mControllerMenus)
        {
            mControllerButtons.mA = "#{sSelect}";
            mControllerButtons.mB = "#{Interface:Cancel}";
            mControllerButtons.mX = "#{sCreate}";
            mControllerButtons.mY = "#{sMagicEffects}";
            mControllerButtons.mR3 = "#{sInfo}";
        }

        center();
    }

    void AlchemyWindow::onAccept(MyGUI::EditBox* sender)
    {
        onCreateButtonClicked(sender);

        // To do not spam onAccept() again and again
        MWBase::Environment::get().getWindowManager()->injectKeyRelease(MyGUI::KeyCode::None);
    }

    void AlchemyWindow::onCancelButtonClicked(MyGUI::Widget* _sender)
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Alchemy);
    }

    void AlchemyWindow::onCreateButtonClicked(MyGUI::Widget* _sender)
    {
        mAlchemy->setPotionName(mNameEdit->getCaption());
        int count = mAlchemy->countPotionsToBrew();
        count = std::min(count, mBrewCountEdit->getValue());
        createPotions(count);
    }

    void AlchemyWindow::createPotions(int count)
    {
        MWMechanics::Alchemy::Result result = mAlchemy->create(mNameEdit->getCaption(), count);
        MWBase::WindowManager* winMgr = MWBase::Environment::get().getWindowManager();

        switch (result)
        {
            case MWMechanics::Alchemy::Result_NoName:
                winMgr->messageBox("#{sNotifyMessage37}");
                break;
            case MWMechanics::Alchemy::Result_NoMortarAndPestle:
                winMgr->messageBox("#{sNotifyMessage45}");
                break;
            case MWMechanics::Alchemy::Result_LessThanTwoIngredients:
                winMgr->messageBox("#{sNotifyMessage6a}");
                break;
            case MWMechanics::Alchemy::Result_Success:
                winMgr->playSound(ESM::RefId::stringRefId("potion success"));
                if (count == 1)
                    winMgr->messageBox("#{sPotionSuccess}");
                else
                    winMgr->messageBox(
                        "#{sPotionSuccess} " + mNameEdit->getCaption().asUTF8() + " (" + std::to_string(count) + ")");
                break;
            case MWMechanics::Alchemy::Result_NoEffects:
            case MWMechanics::Alchemy::Result_RandomFailure:
                winMgr->messageBox("#{sNotifyMessage8}");
                winMgr->playSound(ESM::RefId::stringRefId("potion fail"));
                break;
        }

        // remove ingredient slots that have been fully used up
        for (size_t i = 0; i < mIngredients.size(); ++i)
            if (mIngredients[i]->isUserString("ToolTipType"))
            {
                MWWorld::Ptr ingred = *mIngredients[i]->getUserData<MWWorld::Ptr>();
                if (ingred.getCellRef().getCount() == 0)
                    mAlchemy->removeIngredient(i);
            }

        updateFilters();
        update();
    }

    void AlchemyWindow::initFilter()
    {
        auto const& wm = MWBase::Environment::get().getWindowManager();
        std::string_view ingredient = wm->getGameSettingString("sIngredients", "Ingredients");

        if (mFilterType->getCaption() == ingredient)
        {
            if (Settings::gui().mControllerMenus)
                switchFilterType(mFilterType);
            else
                mCurrentFilter = FilterType::ByName;
        }
        else
            mCurrentFilter = FilterType::ByEffect;
        updateFilters();
        mFilterValue->clearIndexSelected();
        updateFilters();
    }

    void AlchemyWindow::switchFilterType(MyGUI::Widget* _sender)
    {
        auto const& wm = MWBase::Environment::get().getWindowManager();
        std::string_view ingredient = wm->getGameSettingString("sIngredients", "Ingredients");
        auto* button = _sender->castType<MyGUI::Button>();

        if (button->getCaption() == ingredient)
        {
            button->setCaption(MyGUI::UString(wm->getGameSettingString("sMagicEffects", "Magic Effects")));
            mCurrentFilter = FilterType::ByEffect;
        }
        else
        {
            button->setCaption(MyGUI::UString(ingredient));
            mCurrentFilter = FilterType::ByName;
        }
        mSortModel->setNameFilter({});
        mSortModel->setEffectFilter({});
        mFilterValue->clearIndexSelected();
        updateFilters();
        mItemView->update();
    }

    void AlchemyWindow::updateFilters()
    {
        std::set<std::string> itemNames, itemEffects;
        for (size_t i = 0; i < mModel->getItemCount(); ++i)
        {
            MWWorld::Ptr item = mModel->getItem(i).mBase;
            if (item.getType() != ESM::Ingredient::sRecordId)
                continue;

            itemNames.emplace(item.getClass().getName(item));

            MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
            auto const alchemySkill = player.getClass().getSkill(player, ESM::Skill::Alchemy);

            auto const effects = MWMechanics::Alchemy::effectsDescription(item, alchemySkill);
            itemEffects.insert(effects.begin(), effects.end());
        }

        mFilterValue->removeAllItems();
        auto const addItems = [&](auto const& container) {
            for (auto const& item : container)
                mFilterValue->addItem(item);
        };
        switch (mCurrentFilter)
        {
            case FilterType::ByName:
                addItems(itemNames);
                break;
            case FilterType::ByEffect:
                addItems(itemEffects);
                break;
        }
    }

    void AlchemyWindow::applyFilter(const std::string& filter)
    {
        switch (mCurrentFilter)
        {
            case FilterType::ByName:
                mSortModel->setNameFilter(filter);
                break;
            case FilterType::ByEffect:
                mSortModel->setEffectFilter(filter);
                break;
        }
        mItemView->update();
    }

    void AlchemyWindow::onFilterChanged(MyGUI::ComboBox* _sender, size_t _index)
    {
        // ignore spurious event fired when one edit the content after selection.
        // onFilterEdited will handle it.
        if (_index != MyGUI::ITEM_NONE)
            applyFilter(_sender->getItemNameAt(_index));
    }

    void AlchemyWindow::onFilterEdited(MyGUI::EditBox* _sender)
    {
        applyFilter(_sender->getCaption());
    }

    void AlchemyWindow::onOpen()
    {
        mAlchemy->clear();
        mAlchemy->setAlchemist(MWMechanics::getPlayer());

        auto model = std::make_unique<InventoryItemModel>(MWMechanics::getPlayer());
        mModel = model.get();
        auto sortModel = std::make_unique<SortFilterItemModel>(std::move(model));
        mSortModel = sortModel.get();
        mSortModel->setFilter(SortFilterItemModel::Filter_OnlyIngredients);
        mItemView->setModel(std::move(sortModel));
        mItemView->resetScrollBars();

        mNameEdit->setCaption({});
        mBrewCountEdit->setValue(1);

        size_t index = 0;
        for (auto iter = mAlchemy->beginTools(); iter != mAlchemy->endTools() && index < mApparatus.size();
             ++iter, ++index)
        {
            const auto& widget = mApparatus[index];
            widget->setItem(*iter);
            widget->clearUserStrings();
            if (!iter->isEmpty())
            {
                widget->setUserString("ToolTipType", "ItemPtr");
                widget->setUserData(MWWorld::Ptr(*iter));
            }
        }

        update();
        initFilter();

        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mNameEdit);

        if (Settings::gui().mControllerMenus)
            mItemView->setActiveControllerWindow(true);
    }

    void AlchemyWindow::onIngredientSelected(MyGUI::Widget* _sender)
    {
        size_t i = std::distance(mIngredients.begin(), std::find(mIngredients.begin(), mIngredients.end(), _sender));
        mAlchemy->removeIngredient(i);
        update();
    }

    void AlchemyWindow::onItemSelected(MWWorld::Ptr item)
    {
        mItemSelectionDialog->setVisible(false);

        int32_t index = item.get<ESM::Apparatus>()->mBase->mData.mType;
        const auto& widget = mApparatus[index];

        widget->setItem(item);

        if (item.isEmpty())
        {
            widget->clearUserStrings();
            return;
        }

        mAlchemy->addApparatus(item);

        widget->setUserString("ToolTipType", "ItemPtr");
        widget->setUserData(MWWorld::Ptr(item));

        MWBase::Environment::get().getWindowManager()->playSound(item.getClass().getDownSoundId(item));
        update();
    }

    void AlchemyWindow::onItemCancel()
    {
        mItemSelectionDialog->setVisible(false);
    }

    void AlchemyWindow::onApparatusSelected(MyGUI::Widget* _sender)
    {
        size_t i = std::distance(mApparatus.begin(), std::find(mApparatus.begin(), mApparatus.end(), _sender));
        if (_sender->getUserData<MWWorld::Ptr>()->isEmpty()) // if this apparatus slot is empty
        {
            std::string title;
            switch (i)
            {
                case ESM::Apparatus::AppaType::MortarPestle:
                    title = "#{sMortar}";
                    break;
                case ESM::Apparatus::AppaType::Alembic:
                    title = "#{sAlembic}";
                    break;
                case ESM::Apparatus::AppaType::Calcinator:
                    title = "#{sCalcinator}";
                    break;
                case ESM::Apparatus::AppaType::Retort:
                    title = "#{sRetort}";
                    break;
                default:
                    title = "#{sApparatus}";
            }

            mItemSelectionDialog = std::make_unique<ItemSelectionDialog>(title);
            mItemSelectionDialog->eventItemSelected += MyGUI::newDelegate(this, &AlchemyWindow::onItemSelected);
            mItemSelectionDialog->eventDialogCanceled += MyGUI::newDelegate(this, &AlchemyWindow::onItemCancel);
            mItemSelectionDialog->setVisible(true);
            mItemSelectionDialog->openContainer(MWMechanics::getPlayer());
            mItemSelectionDialog->getSortModel()->setApparatusTypeFilter(i);
            mItemSelectionDialog->setFilter(SortFilterItemModel::Filter_OnlyAlchemyTools);
        }
        else
        {
            const auto& widget = mApparatus[i];
            mAlchemy->removeApparatus(i);

            if (widget->getChildCount())
                MyGUI::Gui::getInstance().destroyWidget(widget->getChildAt(0));

            widget->clearUserStrings();
            widget->setItem(MWWorld::Ptr());
            widget->setUserData(MWWorld::Ptr());
        }

        update();
    }

    void AlchemyWindow::onSelectedItem(int index)
    {
        MWWorld::Ptr item = mSortModel->getItem(index).mBase;
        int res = mAlchemy->addIngredient(item);

        if (res != -1)
        {
            update();

            const ESM::RefId& sound = item.getClass().getUpSoundId(item);
            MWBase::Environment::get().getWindowManager()->playSound(sound);
        }
    }

    void AlchemyWindow::update()
    {
        std::string suggestedName = mAlchemy->suggestPotionName();
        if (suggestedName != mSuggestedPotionName)
        {
            mNameEdit->setCaptionWithReplacing(suggestedName);
            mSuggestedPotionName = std::move(suggestedName);
        }

        mSortModel->clearDragItems();

        MWMechanics::Alchemy::TIngredientsIterator it = mAlchemy->beginIngredients();
        for (int i = 0; i < 4; ++i)
        {
            ItemWidget* ingredient = mIngredients[i];

            MWWorld::Ptr item;
            if (it != mAlchemy->endIngredients())
            {
                item = *it;
                ++it;
            }

            if (!item.isEmpty())
                mSortModel->addDragItem(item, item.getCellRef().getCount());

            if (ingredient->getChildCount())
                MyGUI::Gui::getInstance().destroyWidget(ingredient->getChildAt(0));

            ingredient->clearUserStrings();

            ingredient->setItem(item);

            if (item.isEmpty())
                continue;

            ingredient->setUserString("ToolTipType", "ItemPtr");
            ingredient->setUserData(MWWorld::Ptr(item));

            ingredient->setCount(item.getCellRef().getCount());
        }

        mItemView->update();

        std::vector<MWMechanics::EffectKey> effectIds = mAlchemy->listEffects();
        Widgets::SpellEffectList list;
        unsigned int effectIndex = 0;
        for (const MWMechanics::EffectKey& effectKey : effectIds)
        {
            Widgets::SpellEffectParams params;
            params.mEffectID = effectKey.mId;
            const ESM::MagicEffect* magicEffect
                = MWBase::Environment::get().getESMStore()->get<ESM::MagicEffect>().find(effectKey.mId);
            if (magicEffect->mData.mFlags & ESM::MagicEffect::TargetSkill)
                params.mSkill = effectKey.mArg;
            else if (magicEffect->mData.mFlags & ESM::MagicEffect::TargetAttribute)
                params.mAttribute = effectKey.mArg;
            params.mIsConstant = true;
            params.mNoTarget = true;
            params.mNoMagnitude = true;

            params.mKnown = mAlchemy->knownEffect(effectIndex, MWBase::Environment::get().getWorld()->getPlayerPtr());

            list.push_back(params);
            ++effectIndex;
        }

        while (mEffectsBox->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mEffectsBox->getChildAt(0));

        MyGUI::IntCoord coord(0, 0, mEffectsBox->getWidth(), 24);
        Widgets::MWEffectListPtr effectsWidget = mEffectsBox->createWidget<Widgets::MWEffectList>(
            "MW_StatName", coord, MyGUI::Align::Left | MyGUI::Align::Top);

        effectsWidget->setEffectList(list);

        std::vector<MyGUI::Widget*> effectItems;
        effectsWidget->createEffectWidgets(effectItems, mEffectsBox, coord, false, 0);
        effectsWidget->setCoord(coord);
    }

    void AlchemyWindow::addRepeatController(MyGUI::Widget* widget)
    {
        MyGUI::ControllerItem* item
            = MyGUI::ControllerManager::getInstance().createItem(MyGUI::ControllerRepeatClick::getClassTypeName());
        MyGUI::ControllerRepeatClick* controller = static_cast<MyGUI::ControllerRepeatClick*>(item);
        controller->eventRepeatClick += newDelegate(this, &AlchemyWindow::onRepeatClick);
        MyGUI::ControllerManager::getInstance().addItem(widget, controller);
    }

    void AlchemyWindow::onIncreaseButtonPressed(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id)
    {
        addRepeatController(_sender);
        onIncreaseButtonTriggered();
    }

    void AlchemyWindow::onDecreaseButtonPressed(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id)
    {
        addRepeatController(_sender);
        onDecreaseButtonTriggered();
    }

    void AlchemyWindow::onRepeatClick(MyGUI::Widget* widget, MyGUI::ControllerItem* controller)
    {
        if (widget == mIncreaseButton)
            onIncreaseButtonTriggered();
        else if (widget == mDecreaseButton)
            onDecreaseButtonTriggered();
    }

    void AlchemyWindow::onCountButtonReleased(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id)
    {
        MyGUI::ControllerManager::getInstance().removeItem(_sender);
    }

    void AlchemyWindow::onCountValueChanged(int value)
    {
        mBrewCountEdit->setValue(std::abs(value));
    }

    void AlchemyWindow::onIncreaseButtonTriggered()
    {
        int currentCount = mBrewCountEdit->getValue();

        // prevent overflows
        if (currentCount == std::numeric_limits<int>::max())
            return;

        mBrewCountEdit->setValue(currentCount + 1);
    }

    void AlchemyWindow::onDecreaseButtonTriggered()
    {
        int currentCount = mBrewCountEdit->getValue();
        if (currentCount > 1)
            mBrewCountEdit->setValue(currentCount - 1);
    }

    void AlchemyWindow::filterListButtonHandler(const SDL_ControllerButtonEvent& arg)
    {
        if (arg.button == SDL_CONTROLLER_BUTTON_A || arg.button == SDL_CONTROLLER_BUTTON_Y)
        {
            // Select the highlighted entry in the combo box and close it. List is closed by focusing on another
            // widget.
            size_t index = mFilterValue->getIndexSelected();
            mFilterValue->setIndexSelected(index);
            onFilterChanged(mFilterValue, index);
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mNameEdit);

            MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Menu Click"));
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_B)
        {
            // Close the list without selecting anything. List is closed by focusing on another widget.
            mFilterValue->clearIndexSelected();
            onFilterEdited(mFilterValue);
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mNameEdit);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_UP)
            MWBase::Environment::get().getWindowManager()->injectKeyPress(MyGUI::KeyCode::ArrowUp, 0, false);
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
            MWBase::Environment::get().getWindowManager()->injectKeyPress(MyGUI::KeyCode::ArrowDown, 0, false);
    }

    bool AlchemyWindow::onControllerButtonEvent(const SDL_ControllerButtonEvent& arg)
    {
        MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getKeyFocusWidget();
        bool isFilterListOpen
            = focus != nullptr && focus->getParent() != nullptr && focus->getParent()->getParent() == mFilterValue;

        if (isFilterListOpen)
        {
            // When the filter list combo box is open, send all inputs to it.
            filterListButtonHandler(arg);
            return true;
        }

        if (arg.button == SDL_CONTROLLER_BUTTON_B)
        {
            // Remove active ingredients or close the window, starting with right-most slot.
            for (int i = mIngredients.size() - 1; i >= 0; --i)
            {
                if (mIngredients[i]->isUserString("ToolTipType"))
                {
                    onIngredientSelected(mIngredients[i]);
                    return true;
                }
            }
            // If the ingredients list is empty, B closes the menu.
            onCancelButtonClicked(mCancelButton);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_X)
            onCreateButtonClicked(mCreateButton);
        else if (arg.button == SDL_CONTROLLER_BUTTON_Y && mFilterValue->getItemCount() > 0)
        {
            // Magical effects/ingredients filter
            if (mFilterValue->getIndexSelected() != MyGUI::ITEM_NONE)
            {
                // Clear the active filter
                mFilterValue->clearIndexSelected();
                onFilterEdited(mFilterValue);
            }
            else
            {
                // Open the combo box to choose the a filter
                MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mFilterValue);
                MWBase::Environment::get().getWindowManager()->injectKeyPress(MyGUI::KeyCode::ArrowDown, 0, false);
            }
            MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Menu Click"));
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_LEFTSHOULDER)
            onDecreaseButtonTriggered();
        else if (arg.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)
            onIncreaseButtonTriggered();
        else
            mItemView->onControllerButton(arg.button);

        return true;
    }
}
