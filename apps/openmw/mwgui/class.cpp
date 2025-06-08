#include "class.hpp"

#include <MyGUI_Gui.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_ListBox.h>
#include <MyGUI_ScrollView.h>
#include <MyGUI_UString.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/actorutil.hpp"

#include "../mwworld/esmstore.hpp"

#include <components/debug/debuglog.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/settings/values.hpp>
#include <components/vfs/manager.hpp>

#include "tooltips.hpp"

namespace
{

    bool sortClasses(const std::pair<ESM::RefId, std::string>& left, const std::pair<ESM::RefId, std::string>& right)
    {
        return left.second.compare(right.second) < 0;
    }

}

namespace MWGui
{

    /* GenerateClassResultDialog */

    GenerateClassResultDialog::GenerateClassResultDialog()
        : WindowModal("openmw_chargen_generate_class_result.layout")
    {
        setText("ReflectT",
            MWBase::Environment::get().getWindowManager()->getGameSettingString("sMessageQuestionAnswer1", {}));

        getWidget(mClassImage, "ClassImage");
        getWidget(mClassName, "ClassName");

        getWidget(mBackButton, "BackButton");
        mBackButton->setCaptionWithReplacing("#{sMessageQuestionAnswer3}");
        mBackButton->eventMouseButtonClick += MyGUI::newDelegate(this, &GenerateClassResultDialog::onBackClicked);

        getWidget(mOkButton, "OKButton");
        mOkButton->setCaptionWithReplacing("#{sMessageQuestionAnswer2}");
        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &GenerateClassResultDialog::onOkClicked);

        if (Settings::gui().mControllerMenus)
        {
            mOkButton->setStateSelected(true);
            mDisableGamepadCursor = true;
            mControllerButtons.a = "#{sSelect}";
            mControllerButtons.b = "#{sBack}";
        }

        center();
    }

    void GenerateClassResultDialog::setClassId(const ESM::RefId& classId)
    {
        mCurrentClassId = classId;

        setClassImage(mClassImage, mCurrentClassId);

        mClassName->setCaption(
            MWBase::Environment::get().getESMStore()->get<ESM::Class>().find(mCurrentClassId)->mName);

        center();
    }

    bool GenerateClassResultDialog::onControllerButtonEvent(const SDL_ControllerButtonEvent& arg)
    {
        if (arg.button == SDL_CONTROLLER_BUTTON_A)
        {
            if (mOkButtonFocus)
                onOkClicked(mOkButton);
            else
                onBackClicked(mBackButton);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_B)
        {
            onBackClicked(mBackButton);
        }
        else if ((arg.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT && mOkButtonFocus)
            || (arg.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT && !mOkButtonFocus))
        {
            mOkButtonFocus = !mOkButtonFocus;
            mOkButton->setStateSelected(mOkButtonFocus);
            mBackButton->setStateSelected(!mOkButtonFocus);
        }

        return true;
    }

    // widget controls

    void GenerateClassResultDialog::onOkClicked(MyGUI::Widget* _sender)
    {
        eventDone(this);
    }

    void GenerateClassResultDialog::onBackClicked(MyGUI::Widget* _sender)
    {
        eventBack();
    }

    /* PickClassDialog */

    PickClassDialog::PickClassDialog()
        : WindowModal("openmw_chargen_class.layout")
    {
        // Centre dialog
        center();

        getWidget(mSpecializationName, "SpecializationName");

        getWidget(mFavoriteAttribute[0], "FavoriteAttribute0");
        getWidget(mFavoriteAttribute[1], "FavoriteAttribute1");

        for (int i = 0; i < 5; i++)
        {
            char theIndex = '0' + i;
            getWidget(mMajorSkill[i], std::string("MajorSkill").append(1, theIndex));
            getWidget(mMinorSkill[i], std::string("MinorSkill").append(1, theIndex));
        }

        getWidget(mClassList, "ClassList");
        mClassList->setScrollVisible(true);
        mClassList->eventListSelectAccept += MyGUI::newDelegate(this, &PickClassDialog::onAccept);
        mClassList->eventListChangePosition += MyGUI::newDelegate(this, &PickClassDialog::onSelectClass);

        getWidget(mClassImage, "ClassImage");

        getWidget(mBackButton, "BackButton");
        mBackButton->eventMouseButtonClick += MyGUI::newDelegate(this, &PickClassDialog::onBackClicked);

        getWidget(mOkButton, "OKButton");
        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &PickClassDialog::onOkClicked);

        if (Settings::gui().mControllerMenus)
        {
            mControllerButtons.lStick = "#{sMouse}";
            mControllerButtons.a = "#{sSelect}";
            mControllerButtons.b = "#{sBack}";
        }

        updateClasses();
        updateStats();
    }

    void PickClassDialog::setNextButtonShow(bool shown)
    {
        MyGUI::Button* okButton;
        getWidget(okButton, "OKButton");

        if (shown)
        {
            okButton->setCaption(
                MyGUI::UString(MWBase::Environment::get().getWindowManager()->getGameSettingString("sNext", {})));
            mControllerButtons.x = "#{sNext}";
        }
        else if (Settings::gui().mControllerMenus)
        {
            okButton->setCaption(
                MyGUI::UString(MWBase::Environment::get().getWindowManager()->getGameSettingString("sDone", {})));
            mControllerButtons.x = "#{sDone}";
        }
        else
            okButton->setCaption(
                MyGUI::UString(MWBase::Environment::get().getWindowManager()->getGameSettingString("sOK", {})));
    }

    void PickClassDialog::onOpen()
    {
        WindowModal::onOpen();
        updateClasses();
        updateStats();
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mClassList);

        // Show the current class by default
        MWWorld::Ptr player = MWMechanics::getPlayer();

        const ESM::RefId& classId = player.get<ESM::NPC>()->mBase->mClass;

        if (!classId.empty())
            setClassId(classId);
    }

    void PickClassDialog::setClassId(const ESM::RefId& classId)
    {
        mCurrentClassId = classId;
        mClassList->setIndexSelected(MyGUI::ITEM_NONE);
        size_t count = mClassList->getItemCount();
        for (size_t i = 0; i < count; ++i)
        {
            if (*mClassList->getItemDataAt<ESM::RefId>(i) == classId)
            {
                mClassList->setIndexSelected(i);
                break;
            }
        }

        updateStats();
    }

    // widget controls

    void PickClassDialog::onOkClicked(MyGUI::Widget* _sender)
    {
        if (mClassList->getIndexSelected() == MyGUI::ITEM_NONE)
            return;
        eventDone(this);
    }

    void PickClassDialog::onBackClicked(MyGUI::Widget* _sender)
    {
        eventBack();
    }

    void PickClassDialog::onAccept(MyGUI::ListBox* _sender, size_t _index)
    {
        onSelectClass(_sender, _index);
        if (mClassList->getIndexSelected() == MyGUI::ITEM_NONE)
            return;
        eventDone(this);
    }

    void PickClassDialog::onSelectClass(MyGUI::ListBox* _sender, size_t _index)
    {
        if (_index == MyGUI::ITEM_NONE)
            return;

        const ESM::RefId& classId = *mClassList->getItemDataAt<ESM::RefId>(_index);
        if (mCurrentClassId == classId)
            return;

        mCurrentClassId = classId;
        updateStats();
    }

    // update widget content

    void PickClassDialog::updateClasses()
    {
        mClassList->removeAllItems();

        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();

        std::vector<std::pair<ESM::RefId, std::string>> items; // class id, class name
        for (const ESM::Class& classInfo : store.get<ESM::Class>())
        {
            bool playable = (classInfo.mData.mIsPlayable != 0);
            if (!playable) // Only display playable classes
                continue;

            if (store.get<ESM::Class>().isDynamic(classInfo.mId))
                continue; // custom-made class not relevant for this dialog

            items.emplace_back(classInfo.mId, classInfo.mName);
        }
        std::sort(items.begin(), items.end(), sortClasses);

        int index = 0;
        for (auto& itemPair : items)
        {
            const ESM::RefId& id = itemPair.first;
            mClassList->addItem(itemPair.second, id);
            if (mCurrentClassId.empty())
            {
                mCurrentClassId = id;
                mClassList->setIndexSelected(index);
            }
            else if (id == mCurrentClassId)
            {
                mClassList->setIndexSelected(index);
            }
            ++index;
        }
    }

    void PickClassDialog::updateStats()
    {
        if (mCurrentClassId.empty())
            return;
        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
        const ESM::Class* currentClass = store.get<ESM::Class>().search(mCurrentClassId);
        if (!currentClass)
            return;

        ESM::Class::Specialization specialization
            = static_cast<ESM::Class::Specialization>(currentClass->mData.mSpecialization);

        std::string specName{ MWBase::Environment::get().getWindowManager()->getGameSettingString(
            ESM::Class::sGmstSpecializationIds[specialization], ESM::Class::sGmstSpecializationIds[specialization]) };
        mSpecializationName->setCaption(specName);
        ToolTips::createSpecializationToolTip(mSpecializationName, specName, specialization);

        mFavoriteAttribute[0]->setAttributeId(ESM::Attribute::indexToRefId(currentClass->mData.mAttribute[0]));
        mFavoriteAttribute[1]->setAttributeId(ESM::Attribute::indexToRefId(currentClass->mData.mAttribute[1]));
        ToolTips::createAttributeToolTip(mFavoriteAttribute[0], mFavoriteAttribute[0]->getAttributeId());
        ToolTips::createAttributeToolTip(mFavoriteAttribute[1], mFavoriteAttribute[1]->getAttributeId());

        for (size_t i = 0; i < currentClass->mData.mSkills.size(); ++i)
        {
            ESM::RefId minor = ESM::Skill::indexToRefId(currentClass->mData.mSkills[i][0]);
            ESM::RefId major = ESM::Skill::indexToRefId(currentClass->mData.mSkills[i][1]);
            mMinorSkill[i]->setSkillId(minor);
            mMajorSkill[i]->setSkillId(major);
            ToolTips::createSkillToolTip(mMinorSkill[i], minor);
            ToolTips::createSkillToolTip(mMajorSkill[i], major);
        }

        setClassImage(mClassImage, mCurrentClassId);
    }

    bool PickClassDialog::onControllerButtonEvent(const SDL_ControllerButtonEvent& arg)
    {
        if (arg.button == SDL_CONTROLLER_BUTTON_B)
        {
            onBackClicked(mBackButton);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_X)
        {
            onOkClicked(mOkButton);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_UP)
        {
            MWBase::WindowManager* winMgr = MWBase::Environment::get().getWindowManager();
            winMgr->setKeyFocusWidget(mClassList);
            winMgr->injectKeyPress(MyGUI::KeyCode::ArrowUp, 0, false);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
        {
            MWBase::WindowManager* winMgr = MWBase::Environment::get().getWindowManager();
            winMgr->setKeyFocusWidget(mClassList);
            winMgr->injectKeyPress(MyGUI::KeyCode::ArrowDown, 0, false);
        }

        return true;
    }

    /* InfoBoxDialog */

    void InfoBoxDialog::fitToText(MyGUI::TextBox* widget)
    {
        MyGUI::IntCoord inner = widget->getTextRegion();
        MyGUI::IntCoord outer = widget->getCoord();
        MyGUI::IntSize size = widget->getTextSize();
        size.width += outer.width - inner.width;
        size.height += outer.height - inner.height;
        widget->setSize(size);
    }

    void InfoBoxDialog::layoutVertically(MyGUI::Widget* widget, int margin)
    {
        size_t count = widget->getChildCount();
        int pos = 0;
        pos += margin;
        int width = 0;
        for (unsigned i = 0; i < count; ++i)
        {
            MyGUI::Widget* child = widget->getChildAt(i);
            if (!child->getVisible())
                continue;

            child->setPosition(child->getLeft(), pos);
            width = std::max(width, child->getWidth());
            pos += child->getHeight() + margin;
        }
        width += margin * 2;
        widget->setSize(width, pos);
    }

    InfoBoxDialog::InfoBoxDialog()
        : WindowModal("openmw_infobox.layout")
    {
        getWidget(mTextBox, "TextBox");
        getWidget(mText, "Text");
        mText->getSubWidgetText()->setWordWrap(true);
        getWidget(mButtonBar, "ButtonBar");

        center();

        mDisableGamepadCursor = Settings::gui().mControllerMenus;
        mControllerButtons.a = "#{sSelect}";
    }

    void InfoBoxDialog::setText(const std::string& str)
    {
        mText->setCaption(str);
        mTextBox->setVisible(!str.empty());
        fitToText(mText);
    }

    std::string InfoBoxDialog::getText() const
    {
        return mText->getCaption();
    }

    void InfoBoxDialog::setButtons(ButtonList& buttons)
    {
        for (MyGUI::Button* button : this->mButtons)
        {
            MyGUI::Gui::getInstance().destroyWidget(button);
        }
        this->mButtons.clear();

        // TODO: The buttons should be generated from a template in the layout file, ie. cloning an existing widget
        MyGUI::Button* button;
        MyGUI::IntCoord coord = MyGUI::IntCoord(0, 0, mButtonBar->getWidth(), 10);
        for (const std::string& text : buttons)
        {
            button = mButtonBar->createWidget<MyGUI::Button>(
                "MW_Button", coord, MyGUI::Align::Top | MyGUI::Align::HCenter, {});
            button->getSubWidgetText()->setWordWrap(true);
            button->setCaption(text);
            fitToText(button);
            button->eventMouseButtonClick += MyGUI::newDelegate(this, &InfoBoxDialog::onButtonClicked);
            coord.top += button->getHeight();

            if (Settings::gui().mControllerMenus && buttons.size() > 1 && this->mButtons.empty())
            {
                // First button is selected by default
                button->setStateSelected(true);
            }

            this->mButtons.push_back(button);
        }
    }

    void InfoBoxDialog::onOpen()
    {
        WindowModal::onOpen();
        // Fix layout
        layoutVertically(mTextBox, 4);
        layoutVertically(mButtonBar, 6);
        layoutVertically(mMainWidget, 4 + 6);

        center();
    }

    void InfoBoxDialog::onButtonClicked(MyGUI::Widget* _sender)
    {
        int i = 0;
        for (MyGUI::Button* button : mButtons)
        {
            if (button == _sender)
            {
                eventButtonSelected(i);
                return;
            }
            ++i;
        }
    }

    bool InfoBoxDialog::onControllerButtonEvent(const SDL_ControllerButtonEvent& arg)
    {
        if (arg.button == SDL_CONTROLLER_BUTTON_A)
        {
            if (mControllerFocus >= 0 && mControllerFocus < mButtons.size())
                onButtonClicked(mButtons[mControllerFocus]);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_B)
        {
            if (mButtons.size() == 1)
                onButtonClicked(mButtons[0]);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_UP)
        {
            if (mButtons.size() <= 1)
                return true;
            if (mButtons.size() == 2 && mControllerFocus == 0)
                return true;

            setControllerFocus(mButtons, mControllerFocus, false);
            mControllerFocus = wrap(mControllerFocus - 1, mButtons.size());
            setControllerFocus(mButtons, mControllerFocus, true);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
        {
            if (mButtons.size() <= 1)
                return true;
            if (mButtons.size() == 2 && mControllerFocus == mButtons.size() - 1)
                return true;

            setControllerFocus(mButtons, mControllerFocus, false);
            mControllerFocus = wrap(mControllerFocus + 1, mButtons.size());
            setControllerFocus(mButtons, mControllerFocus, true);
        }

        return true;
    }

    /* ClassChoiceDialog */

    ClassChoiceDialog::ClassChoiceDialog()
        : InfoBoxDialog()
    {
        setText({});
        ButtonList buttons;
        buttons.emplace_back(
            MWBase::Environment::get().getWindowManager()->getGameSettingString("sClassChoiceMenu1", {}));
        buttons.emplace_back(
            MWBase::Environment::get().getWindowManager()->getGameSettingString("sClassChoiceMenu2", {}));
        buttons.emplace_back(
            MWBase::Environment::get().getWindowManager()->getGameSettingString("sClassChoiceMenu3", {}));
        buttons.emplace_back(MWBase::Environment::get().getWindowManager()->getGameSettingString("sBack", {}));
        setButtons(buttons);
    }

    /* CreateClassDialog */

    CreateClassDialog::CreateClassDialog()
        : WindowModal("openmw_chargen_create_class.layout")
        , mAffectedAttribute(nullptr)
        , mAffectedSkill(nullptr)
    {
        // Centre dialog
        center();

        setText("SpecializationT",
            MWBase::Environment::get().getWindowManager()->getGameSettingString("sChooseClassMenu1", "Specialization"));
        getWidget(mSpecializationName, "SpecializationName");
        mSpecializationName->eventMouseButtonClick
            += MyGUI::newDelegate(this, &CreateClassDialog::onSpecializationClicked);

        setText("FavoriteAttributesT",
            MWBase::Environment::get().getWindowManager()->getGameSettingString(
                "sChooseClassMenu2", "Favorite Attributes:"));
        getWidget(mFavoriteAttribute0, "FavoriteAttribute0");
        getWidget(mFavoriteAttribute1, "FavoriteAttribute1");
        mFavoriteAttribute0->eventClicked += MyGUI::newDelegate(this, &CreateClassDialog::onAttributeClicked);
        mFavoriteAttribute1->eventClicked += MyGUI::newDelegate(this, &CreateClassDialog::onAttributeClicked);

        setText(
            "MajorSkillT", MWBase::Environment::get().getWindowManager()->getGameSettingString("sSkillClassMajor", {}));
        setText(
            "MinorSkillT", MWBase::Environment::get().getWindowManager()->getGameSettingString("sSkillClassMinor", {}));
        for (int i = 0; i < 5; i++)
        {
            char theIndex = '0' + i;
            getWidget(mMajorSkill[i], std::string("MajorSkill").append(1, theIndex));
            getWidget(mMinorSkill[i], std::string("MinorSkill").append(1, theIndex));
            mSkills.push_back(mMajorSkill[i]);
            mSkills.push_back(mMinorSkill[i]);
        }

        for (Widgets::MWSkillPtr& skill : mSkills)
        {
            skill->eventClicked += MyGUI::newDelegate(this, &CreateClassDialog::onSkillClicked);
        }

        setText("LabelT", MWBase::Environment::get().getWindowManager()->getGameSettingString("sName", {}));
        getWidget(mEditName, "EditName");

        // Make sure the edit box has focus
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mEditName);

        MyGUI::Button* descriptionButton;
        getWidget(descriptionButton, "DescriptionButton");
        descriptionButton->eventMouseButtonClick += MyGUI::newDelegate(this, &CreateClassDialog::onDescriptionClicked);
        mButtons.push_back(descriptionButton);

        MyGUI::Button* backButton;
        getWidget(backButton, "BackButton");
        backButton->eventMouseButtonClick += MyGUI::newDelegate(this, &CreateClassDialog::onBackClicked);
        mButtons.push_back(backButton);

        MyGUI::Button* okButton;
        getWidget(okButton, "OKButton");
        okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &CreateClassDialog::onOkClicked);
        mButtons.push_back(okButton);

        if (Settings::gui().mControllerMenus)
        {
            okButton->setStateSelected(true);
            mControllerButtons.lStick = "#{sMouse}";
            mControllerButtons.a = "#{sSelect}";
            mControllerButtons.b = "#{sBack}";
        }

        // Set default skills, attributes

        mFavoriteAttribute0->setAttributeId(ESM::Attribute::Strength);
        mFavoriteAttribute1->setAttributeId(ESM::Attribute::Agility);

        mMajorSkill[0]->setSkillId(ESM::Skill::Block);
        mMajorSkill[1]->setSkillId(ESM::Skill::Armorer);
        mMajorSkill[2]->setSkillId(ESM::Skill::MediumArmor);
        mMajorSkill[3]->setSkillId(ESM::Skill::HeavyArmor);
        mMajorSkill[4]->setSkillId(ESM::Skill::BluntWeapon);

        mMinorSkill[0]->setSkillId(ESM::Skill::LongBlade);
        mMinorSkill[1]->setSkillId(ESM::Skill::Axe);
        mMinorSkill[2]->setSkillId(ESM::Skill::Spear);
        mMinorSkill[3]->setSkillId(ESM::Skill::Athletics);
        mMinorSkill[4]->setSkillId(ESM::Skill::Enchant);

        setSpecialization(0);
        update();
    }

    CreateClassDialog::~CreateClassDialog() = default;

    void CreateClassDialog::update()
    {
        for (int i = 0; i < 5; ++i)
        {
            ToolTips::createSkillToolTip(mMajorSkill[i], mMajorSkill[i]->getSkillId());
            ToolTips::createSkillToolTip(mMinorSkill[i], mMinorSkill[i]->getSkillId());
        }

        ToolTips::createAttributeToolTip(mFavoriteAttribute0, mFavoriteAttribute0->getAttributeId());
        ToolTips::createAttributeToolTip(mFavoriteAttribute1, mFavoriteAttribute1->getAttributeId());
    }

    std::string CreateClassDialog::getName() const
    {
        return mEditName->getCaption();
    }

    std::string CreateClassDialog::getDescription() const
    {
        return mDescription;
    }

    ESM::Class::Specialization CreateClassDialog::getSpecializationId() const
    {
        return mSpecializationId;
    }

    std::vector<ESM::RefId> CreateClassDialog::getFavoriteAttributes() const
    {
        std::vector<ESM::RefId> v;
        v.push_back(mFavoriteAttribute0->getAttributeId());
        v.push_back(mFavoriteAttribute1->getAttributeId());
        return v;
    }

    std::vector<ESM::RefId> CreateClassDialog::getMajorSkills() const
    {
        std::vector<ESM::RefId> v;
        v.reserve(mMajorSkill.size());
        for (const auto& widget : mMajorSkill)
        {
            v.push_back(widget->getSkillId());
        }
        return v;
    }

    std::vector<ESM::RefId> CreateClassDialog::getMinorSkills() const
    {
        std::vector<ESM::RefId> v;
        v.reserve(mMinorSkill.size());
        for (const auto& widget : mMinorSkill)
        {
            v.push_back(widget->getSkillId());
        }
        return v;
    }

    void CreateClassDialog::setNextButtonShow(bool shown)
    {
        MyGUI::Button* okButton;
        getWidget(okButton, "OKButton");

        if (shown)
        {
            okButton->setCaption(
                MyGUI::UString(MWBase::Environment::get().getWindowManager()->getGameSettingString("sNext", {})));
            mControllerButtons.x = "#{sNext}";
        }
        else if (Settings::gui().mControllerMenus)
        {
            okButton->setCaption(
                MyGUI::UString(MWBase::Environment::get().getWindowManager()->getGameSettingString("sDone", {})));
            mControllerButtons.x = "#{sDone}";
        }
        else
            okButton->setCaption(
                MyGUI::UString(MWBase::Environment::get().getWindowManager()->getGameSettingString("sOK", {})));
    }

    bool CreateClassDialog::onControllerButtonEvent(const SDL_ControllerButtonEvent& arg)
    {
        if (arg.button == SDL_CONTROLLER_BUTTON_A)
        {
            if (mControllerFocus == 0)
                onDescriptionClicked(mButtons[0]);
            else if (mControllerFocus == 1)
                onBackClicked(mButtons[1]);
            else
                onOkClicked(mButtons[2]);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_B)
        {
            onBackClicked(mButtons[1]);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_X)
        {
            onOkClicked(mButtons[2]);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
        {
            setControllerFocus(mButtons, mControllerFocus, false);
            mControllerFocus = wrap(mControllerFocus - 1, mButtons.size());
            setControllerFocus(mButtons, mControllerFocus, true);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
        {
            setControllerFocus(mButtons, mControllerFocus, false);
            mControllerFocus = wrap(mControllerFocus + 1, mButtons.size());
            setControllerFocus(mButtons, mControllerFocus, true);
        }
        return true;
    }

    // widget controls

    void CreateClassDialog::onDialogCancel()
    {
        MWBase::Environment::get().getWindowManager()->removeDialog(std::move(mSpecDialog));
        MWBase::Environment::get().getWindowManager()->removeDialog(std::move(mAttribDialog));
        MWBase::Environment::get().getWindowManager()->removeDialog(std::move(mSkillDialog));
        MWBase::Environment::get().getWindowManager()->removeDialog(std::move(mDescDialog));
    }

    void CreateClassDialog::onSpecializationClicked(MyGUI::Widget* _sender)
    {
        mSpecDialog = std::make_unique<SelectSpecializationDialog>();
        mSpecDialog->eventCancel += MyGUI::newDelegate(this, &CreateClassDialog::onDialogCancel);
        mSpecDialog->eventItemSelected += MyGUI::newDelegate(this, &CreateClassDialog::onSpecializationSelected);
        mSpecDialog->setVisible(true);
    }

    void CreateClassDialog::onSpecializationSelected()
    {
        mSpecializationId = mSpecDialog->getSpecializationId();
        setSpecialization(mSpecializationId);

        MWBase::Environment::get().getWindowManager()->removeDialog(std::move(mSpecDialog));
    }

    void CreateClassDialog::setSpecialization(int id)
    {
        mSpecializationId = ESM::Class::Specialization(id);
        std::string specName{ MWBase::Environment::get().getWindowManager()->getGameSettingString(
            ESM::Class::sGmstSpecializationIds[mSpecializationId],
            ESM::Class::sGmstSpecializationIds[mSpecializationId]) };
        mSpecializationName->setCaption(specName);
        ToolTips::createSpecializationToolTip(mSpecializationName, specName, mSpecializationId);
    }

    void CreateClassDialog::onAttributeClicked(Widgets::MWAttributePtr _sender)
    {
        mAttribDialog = std::make_unique<SelectAttributeDialog>();
        mAffectedAttribute = _sender;
        mAttribDialog->eventCancel += MyGUI::newDelegate(this, &CreateClassDialog::onDialogCancel);
        mAttribDialog->eventItemSelected += MyGUI::newDelegate(this, &CreateClassDialog::onAttributeSelected);
        mAttribDialog->setVisible(true);
    }

    void CreateClassDialog::onAttributeSelected()
    {
        ESM::RefId id = mAttribDialog->getAttributeId();
        if (mAffectedAttribute == mFavoriteAttribute0)
        {
            if (mFavoriteAttribute1->getAttributeId() == id)
                mFavoriteAttribute1->setAttributeId(mFavoriteAttribute0->getAttributeId());
        }
        else if (mAffectedAttribute == mFavoriteAttribute1)
        {
            if (mFavoriteAttribute0->getAttributeId() == id)
                mFavoriteAttribute0->setAttributeId(mFavoriteAttribute1->getAttributeId());
        }
        mAffectedAttribute->setAttributeId(id);
        MWBase::Environment::get().getWindowManager()->removeDialog(std::move(mAttribDialog));

        update();
    }

    void CreateClassDialog::onSkillClicked(Widgets::MWSkillPtr _sender)
    {
        mSkillDialog = std::make_unique<SelectSkillDialog>();
        mAffectedSkill = _sender;
        mSkillDialog->eventCancel += MyGUI::newDelegate(this, &CreateClassDialog::onDialogCancel);
        mSkillDialog->eventItemSelected += MyGUI::newDelegate(this, &CreateClassDialog::onSkillSelected);
        mSkillDialog->setVisible(true);
    }

    void CreateClassDialog::onSkillSelected()
    {
        ESM::RefId id = mSkillDialog->getSkillId();

        // Avoid duplicate skills by swapping any skill field that matches the selected one
        for (Widgets::MWSkillPtr& skill : mSkills)
        {
            if (skill == mAffectedSkill)
                continue;
            if (skill->getSkillId() == id)
            {
                skill->setSkillId(mAffectedSkill->getSkillId());
                break;
            }
        }

        mAffectedSkill->setSkillId(mSkillDialog->getSkillId());
        MWBase::Environment::get().getWindowManager()->removeDialog(std::move(mSkillDialog));
        update();
    }

    void CreateClassDialog::onDescriptionClicked(MyGUI::Widget* _sender)
    {
        mDescDialog = std::make_unique<DescriptionDialog>();
        mDescDialog->setTextInput(mDescription);
        mDescDialog->eventDone += MyGUI::newDelegate(this, &CreateClassDialog::onDescriptionEntered);
        mDescDialog->setVisible(true);
    }

    void CreateClassDialog::onDescriptionEntered(WindowBase* parWindow)
    {
        mDescription = mDescDialog->getTextInput();
        MWBase::Environment::get().getWindowManager()->removeDialog(std::move(mDescDialog));
    }

    void CreateClassDialog::onOkClicked(MyGUI::Widget* _sender)
    {
        if (getName().size() <= 0)
            return;
        eventDone(this);
    }

    void CreateClassDialog::onBackClicked(MyGUI::Widget* _sender)
    {
        eventBack();
    }

    /* SelectSpecializationDialog */

    SelectSpecializationDialog::SelectSpecializationDialog()
        : WindowModal("openmw_chargen_select_specialization.layout")
    {
        // Centre dialog
        center();

        getWidget(mSpecialization0, "Specialization0");
        getWidget(mSpecialization1, "Specialization1");
        getWidget(mSpecialization2, "Specialization2");
        std::string combat{ MWBase::Environment::get().getWindowManager()->getGameSettingString(
            ESM::Class::sGmstSpecializationIds[ESM::Class::Combat], {}) };
        std::string magic{ MWBase::Environment::get().getWindowManager()->getGameSettingString(
            ESM::Class::sGmstSpecializationIds[ESM::Class::Magic], {}) };
        std::string stealth{ MWBase::Environment::get().getWindowManager()->getGameSettingString(
            ESM::Class::sGmstSpecializationIds[ESM::Class::Stealth], {}) };

        mSpecialization0->setCaption(combat);
        mSpecialization0->eventMouseButtonClick
            += MyGUI::newDelegate(this, &SelectSpecializationDialog::onSpecializationClicked);
        mSpecialization1->setCaption(magic);
        mSpecialization1->eventMouseButtonClick
            += MyGUI::newDelegate(this, &SelectSpecializationDialog::onSpecializationClicked);
        mSpecialization2->setCaption(stealth);
        mSpecialization2->eventMouseButtonClick
            += MyGUI::newDelegate(this, &SelectSpecializationDialog::onSpecializationClicked);
        mSpecializationId = ESM::Class::Combat;

        ToolTips::createSpecializationToolTip(mSpecialization0, combat, ESM::Class::Combat);
        ToolTips::createSpecializationToolTip(mSpecialization1, magic, ESM::Class::Magic);
        ToolTips::createSpecializationToolTip(mSpecialization2, stealth, ESM::Class::Stealth);

        MyGUI::Button* cancelButton;
        getWidget(cancelButton, "CancelButton");
        cancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SelectSpecializationDialog::onCancelClicked);

        mControllerButtons.a = "#{sSelect}";
        mControllerButtons.b = "#{sCancel}";
    }

    SelectSpecializationDialog::~SelectSpecializationDialog() {}

    // widget controls

    void SelectSpecializationDialog::onSpecializationClicked(MyGUI::Widget* _sender)
    {
        if (_sender == mSpecialization0)
            mSpecializationId = ESM::Class::Combat;
        else if (_sender == mSpecialization1)
            mSpecializationId = ESM::Class::Magic;
        else if (_sender == mSpecialization2)
            mSpecializationId = ESM::Class::Stealth;
        else
            return;

        eventItemSelected();
    }

    void SelectSpecializationDialog::onCancelClicked(MyGUI::Widget* _sender)
    {
        exit();
    }

    bool SelectSpecializationDialog::exit()
    {
        eventCancel();
        return true;
    }

    bool SelectSpecializationDialog::onControllerButtonEvent(const SDL_ControllerButtonEvent& arg)
    {
        if (arg.button == SDL_CONTROLLER_BUTTON_B)
        {
            onCancelClicked(nullptr);
            return true;
        }
        return false;
    }

    /* SelectAttributeDialog */

    SelectAttributeDialog::SelectAttributeDialog()
        : WindowModal("openmw_chargen_select_attribute.layout")
        , mAttributeId(ESM::Attribute::Strength)
    {
        // Centre dialog
        center();

        const auto& store = MWBase::Environment::get().getWorld()->getStore().get<ESM::Attribute>();
        MyGUI::ScrollView* attributes;
        getWidget(attributes, "Attributes");
        MyGUI::IntCoord coord{ 0, 0, attributes->getWidth(), 18 };
        for (const ESM::Attribute& attribute : store)
        {
            auto* widget
                = attributes->createWidget<Widgets::MWAttribute>("MW_StatNameButtonC", coord, MyGUI::Align::Default);
            coord.top += coord.height;
            widget->setAttributeId(attribute.mId);
            widget->eventClicked += MyGUI::newDelegate(this, &SelectAttributeDialog::onAttributeClicked);
            ToolTips::createAttributeToolTip(widget, attribute.mId);
            mAttributeButtons.emplace_back(widget);
        }

        attributes->setVisibleVScroll(false);
        attributes->setCanvasSize(MyGUI::IntSize(attributes->getWidth(), std::max(attributes->getHeight(), coord.top)));
        attributes->setVisibleVScroll(true);
        attributes->setViewOffset(MyGUI::IntPoint());

        MyGUI::Button* cancelButton;
        getWidget(cancelButton, "CancelButton");
        cancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SelectAttributeDialog::onCancelClicked);

        if (Settings::gui().mControllerMenus)
        {
            mControllerFocus = 0;
            if (mAttributeButtons.size() > 0)
                mAttributeButtons[0]->setStateSelected(true);

            mControllerButtons.a = "#{sSelect}";
            mControllerButtons.b = "#{sCancel}";
        }
    }

    // widget controls

    void SelectAttributeDialog::onAttributeClicked(Widgets::MWAttributePtr _sender)
    {
        mAttributeId = _sender->getAttributeId();
        eventItemSelected();
    }

    void SelectAttributeDialog::onCancelClicked(MyGUI::Widget* _sender)
    {
        exit();
    }

    bool SelectAttributeDialog::exit()
    {
        eventCancel();
        return true;
    }

    bool SelectAttributeDialog::onControllerButtonEvent(const SDL_ControllerButtonEvent& arg)
    {
        if (arg.button == SDL_CONTROLLER_BUTTON_A)
        {
            if (mControllerFocus >= 0 && mControllerFocus < mAttributeButtons.size())
                onAttributeClicked(mAttributeButtons[mControllerFocus]);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_B)
        {
            onCancelClicked(nullptr);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_UP)
        {
            mAttributeButtons[mControllerFocus]->setStateSelected(false);
            mControllerFocus = wrap(mControllerFocus - 1, mAttributeButtons.size());
            mAttributeButtons[mControllerFocus]->setStateSelected(true);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
        {
            mAttributeButtons[mControllerFocus]->setStateSelected(false);
            mControllerFocus = wrap(mControllerFocus + 1, mAttributeButtons.size());
            mAttributeButtons[mControllerFocus]->setStateSelected(true);
        }

        return true;
    }

    /* SelectSkillDialog */

    SelectSkillDialog::SelectSkillDialog()
        : WindowModal("openmw_chargen_select_skill.layout")
        , mSkillId(ESM::Skill::Block)
    {
        // Centre dialog
        center();

        std::array<std::pair<MyGUI::ScrollView*, MyGUI::IntCoord>, 3> specializations;
        getWidget(specializations[ESM::Class::Combat].first, "CombatSkills");
        getWidget(specializations[ESM::Class::Magic].first, "MagicSkills");
        getWidget(specializations[ESM::Class::Stealth].first, "StealthSkills");
        for (auto& [widget, coord] : specializations)
        {
            coord.width = widget->getCoord().width;
            coord.height = 18;
            while (widget->getChildCount() > 0)
                MyGUI::Gui::getInstance().destroyWidget(widget->getChildAt(0));
        }
        for (const ESM::Skill& skill : MWBase::Environment::get().getESMStore()->get<ESM::Skill>())
        {
            auto& [widget, coord] = specializations[skill.mData.mSpecialization];
            auto* skillWidget
                = widget->createWidget<Widgets::MWSkill>("MW_StatNameButton", coord, MyGUI::Align::Default);
            coord.top += coord.height;
            skillWidget->setSkillId(skill.mId);
            skillWidget->eventClicked += MyGUI::newDelegate(this, &SelectSkillDialog::onSkillClicked);
            ToolTips::createSkillToolTip(skillWidget, skill.mId);
            mSkillButtons.emplace_back(skillWidget);
        }
        for (const auto& [widget, coord] : specializations)
        {
            widget->setVisibleVScroll(false);
            widget->setCanvasSize(MyGUI::IntSize(widget->getWidth(), std::max(widget->getHeight(), coord.top)));
            widget->setVisibleVScroll(true);
            widget->setViewOffset(MyGUI::IntPoint());
        }

        MyGUI::Button* cancelButton;
        getWidget(cancelButton, "CancelButton");
        cancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SelectSkillDialog::onCancelClicked);

        if (Settings::gui().mControllerMenus)
        {
            mControllerFocus = 0;
            if (mSkillButtons.size() > 0)
                mSkillButtons[0]->setStateSelected(true);

            mControllerButtons.a = "#{sSelect}";
            mControllerButtons.b = "#{sCancel}";
        }
    }

    SelectSkillDialog::~SelectSkillDialog() {}

    // widget controls

    void SelectSkillDialog::onSkillClicked(Widgets::MWSkillPtr _sender)
    {
        mSkillId = _sender->getSkillId();
        eventItemSelected();
    }

    void SelectSkillDialog::onCancelClicked(MyGUI::Widget* _sender)
    {
        exit();
    }

    bool SelectSkillDialog::exit()
    {
        eventCancel();
        return true;
    }

    bool SelectSkillDialog::onControllerButtonEvent(const SDL_ControllerButtonEvent& arg)
    {
        if (arg.button == SDL_CONTROLLER_BUTTON_A)
        {
            if (mControllerFocus >= 0 && mControllerFocus < mSkillButtons.size())
                onSkillClicked(mSkillButtons[mControllerFocus]);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_B)
        {
            onCancelClicked(nullptr);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_UP)
        {
            mSkillButtons[mControllerFocus]->setStateSelected(false);
            mControllerFocus = wrap(mControllerFocus - 1, mSkillButtons.size());
            mSkillButtons[mControllerFocus]->setStateSelected(true);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
        {
            mSkillButtons[mControllerFocus]->setStateSelected(false);
            mControllerFocus = wrap(mControllerFocus + 1, mSkillButtons.size());
            mSkillButtons[mControllerFocus]->setStateSelected(true);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
        {
            mSkillButtons[mControllerFocus]->setStateSelected(false);
            if (mControllerFocus < 9)
                mControllerFocus += 18;
            else
                mControllerFocus -= 9;
            mSkillButtons[mControllerFocus]->setStateSelected(true);
        }
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
        {
            mSkillButtons[mControllerFocus]->setStateSelected(false);
            if (mControllerFocus >= 18)
                mControllerFocus -= 18;
            else
                mControllerFocus += 9;
            mSkillButtons[mControllerFocus]->setStateSelected(true);
        }

        return true;
    }

    /* DescriptionDialog */

    DescriptionDialog::DescriptionDialog()
        : WindowModal("openmw_chargen_class_description.layout")
    {
        // Centre dialog
        center();

        getWidget(mTextEdit, "TextEdit");

        MyGUI::Button* okButton;
        getWidget(okButton, "OKButton");
        okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DescriptionDialog::onOkClicked);
        okButton->setCaption(
            MyGUI::UString(MWBase::Environment::get().getWindowManager()->getGameSettingString("sInputMenu1", {})));

        // Make sure the edit box has focus
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mTextEdit);

        mControllerButtons.a = "#{sOk}";
    }

    DescriptionDialog::~DescriptionDialog() {}

    // widget controls

    void DescriptionDialog::onOkClicked(MyGUI::Widget* _sender)
    {
        eventDone(this);
    }

    void setClassImage(MyGUI::ImageBox* imageBox, const ESM::RefId& classId)
    {
        std::string_view fallback = "textures\\levelup\\warrior.dds";
        std::string classImage;
        if (const auto* id = classId.getIf<ESM::StringRefId>())
        {
            const VFS::Manager* const vfs = MWBase::Environment::get().getResourceSystem()->getVFS();
            classImage
                = Misc::ResourceHelpers::correctTexturePath("textures\\levelup\\" + id->getValue() + ".dds", vfs);
            if (!vfs->exists(classImage))
            {
                Log(Debug::Warning) << "No class image for " << classId << ", falling back to default";
                classImage = fallback;
            }
        }
        else
            classImage = fallback;
        imageBox->setImageTexture(classImage);
    }

    bool DescriptionDialog::onControllerButtonEvent(const SDL_ControllerButtonEvent& arg)
    {
        if (arg.button == SDL_CONTROLLER_BUTTON_A || arg.button == SDL_CONTROLLER_BUTTON_B)
        {
            onOkClicked(nullptr);
            return true;
        }
        return false;
    }
}
