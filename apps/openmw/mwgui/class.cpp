#include "class.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"
#include "window_manager.hpp"
#include "components/esm_store/store.hpp"

#include <assert.h>
#include <iterator>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

using namespace MWGui;

/* GenerateClassResultDialog */

GenerateClassResultDialog::GenerateClassResultDialog(MWWorld::Environment& environment)
  : Layout("openmw_chargen_generate_class_result_layout.xml")
  , environment(environment)
{
    // Centre dialog
    MyGUI::IntSize gameWindowSize = environment.mWindowManager->getGui()->getViewSize();
    MyGUI::IntCoord coord = mMainWidget->getCoord();
    coord.left = (gameWindowSize.width - coord.width)/2;
    coord.top = (gameWindowSize.height - coord.height)/2;
    mMainWidget->setCoord(coord);

    WindowManager *wm = environment.mWindowManager;
    setText("ReflectT", wm->getGameSettingString("sMessageQuestionAnswer1", ""));

    getWidget(classImage, "ClassImage");
    getWidget(className, "ClassName");

    // TODO: These buttons should be managed by a Dialog class
    MyGUI::ButtonPtr backButton;
    getWidget(backButton, "BackButton");
    backButton->eventMouseButtonClick = MyGUI::newDelegate(this, &GenerateClassResultDialog::onBackClicked);

    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");
    okButton->eventMouseButtonClick = MyGUI::newDelegate(this, &GenerateClassResultDialog::onOkClicked);
}

std::string GenerateClassResultDialog::getClassId() const
{
    return className->getCaption();
}

void GenerateClassResultDialog::setClassId(const std::string &classId)
{
    currentClassId = classId;
    classImage->setImageTexture(std::string("textures\\levelup\\") + currentClassId + ".dds");
    ESMS::ESMStore &store = environment.mWorld->getStore();
    className->setCaption(store.classes.find(currentClassId)->name);
}

// widget controls

void GenerateClassResultDialog::onOkClicked(MyGUI::Widget* _sender)
{
    eventDone();
}

void GenerateClassResultDialog::onBackClicked(MyGUI::Widget* _sender)
{
    eventBack();
}

/* PickClassDialog */

PickClassDialog::PickClassDialog(MWWorld::Environment& environment, MyGUI::IntSize gameWindowSize)
  : Layout("openmw_chargen_class_layout.xml")
  , environment(environment)
{
    // Centre dialog
    MyGUI::IntCoord coord = mMainWidget->getCoord();
    coord.left = (gameWindowSize.width - coord.width)/2;
    coord.top = (gameWindowSize.height - coord.height)/2;
    mMainWidget->setCoord(coord);

    WindowManager *wm = environment.mWindowManager;
    setText("SpecializationT", wm->getGameSettingString("sChooseClassMenu1", "Specialization"));
    getWidget(specializationName, "SpecializationName");

    setText("FavoriteAttributesT", wm->getGameSettingString("sChooseClassMenu2", "Favorite Attributes:"));
    getWidget(favoriteAttribute0, "FavoriteAttribute0");
    getWidget(favoriteAttribute1, "FavoriteAttribute1");
    favoriteAttribute0->setWindowManager(wm);
    favoriteAttribute1->setWindowManager(wm);

    setText("MajorSkillT", wm->getGameSettingString("sChooseClassMenu3", "Major Skills:"));
    getWidget(majorSkill0, "MajorSkill0");
    getWidget(majorSkill1, "MajorSkill1");
    getWidget(majorSkill2, "MajorSkill2");
    getWidget(majorSkill3, "MajorSkill3");
    getWidget(majorSkill4, "MajorSkill4");
    majorSkill0->setWindowManager(wm);
    majorSkill1->setWindowManager(wm);
    majorSkill2->setWindowManager(wm);
    majorSkill3->setWindowManager(wm);
    majorSkill4->setWindowManager(wm);

    setText("MinorSkillT", wm->getGameSettingString("sChooseClassMenu4", "Minor Skills:"));
    getWidget(minorSkill0, "MinorSkill0");
    getWidget(minorSkill1, "MinorSkill1");
    getWidget(minorSkill2, "MinorSkill2");
    getWidget(minorSkill3, "MinorSkill3");
    getWidget(minorSkill4, "MinorSkill4");
    minorSkill0->setWindowManager(wm);
    minorSkill1->setWindowManager(wm);
    minorSkill2->setWindowManager(wm);
    minorSkill3->setWindowManager(wm);
    minorSkill4->setWindowManager(wm);

    getWidget(classList, "ClassList");
    classList->setScrollVisible(true);
    classList->eventListSelectAccept = MyGUI::newDelegate(this, &PickClassDialog::onSelectClass);
    classList->eventListMouseItemActivate = MyGUI::newDelegate(this, &PickClassDialog::onSelectClass);
    classList->eventListChangePosition = MyGUI::newDelegate(this, &PickClassDialog::onSelectClass);

    getWidget(classImage, "ClassImage");

    // TODO: These buttons should be managed by a Dialog class
    MyGUI::ButtonPtr backButton;
    getWidget(backButton, "BackButton");
    backButton->eventMouseButtonClick = MyGUI::newDelegate(this, &PickClassDialog::onBackClicked);

    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");
    okButton->eventMouseButtonClick = MyGUI::newDelegate(this, &PickClassDialog::onOkClicked);

    updateClasses();
    updateStats();
}

void PickClassDialog::setNextButtonShow(bool shown)
{
    MyGUI::ButtonPtr backButton;
    getWidget(backButton, "BackButton");

    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");

    // TODO: All hardcoded coords for buttons are temporary, will be replaced with a dynamic system.
    if (shown)
    {
        okButton->setCaption("Next");

        // Adjust back button when next is shown
        backButton->setCoord(MyGUI::IntCoord(382 - 18, 265, 53, 23));
        okButton->setCoord(MyGUI::IntCoord(434 - 18, 265, 42 + 18, 23));
    }
    else
    {
        okButton->setCaption("OK");
        backButton->setCoord(MyGUI::IntCoord(382, 265, 53, 23));
        okButton->setCoord(MyGUI::IntCoord(434, 265, 42, 23));
    }
}

void PickClassDialog::open()
{
    updateClasses();
    updateStats();
    setVisible(true);
}


void PickClassDialog::setClassId(const std::string &classId)
{
    currentClassId = classId;
    classList->setIndexSelected(MyGUI::ITEM_NONE);
    size_t count = classList->getItemCount();
    for (size_t i = 0; i < count; ++i)
    {
        if (boost::iequals(*classList->getItemDataAt<std::string>(i), classId))
        {
            classList->setIndexSelected(i);
            break;
        }
    }

    updateStats();
}

// widget controls

void PickClassDialog::onOkClicked(MyGUI::Widget* _sender)
{
    eventDone();
}

void PickClassDialog::onBackClicked(MyGUI::Widget* _sender)
{
    eventBack();
}

void PickClassDialog::onSelectClass(MyGUI::List* _sender, size_t _index)
{
    if (_index == MyGUI::ITEM_NONE)
        return;

    const std::string *classId = classList->getItemDataAt<std::string>(_index);
    if (boost::iequals(currentClassId, *classId))
        return;

    currentClassId = *classId;
    updateStats();
}

// update widget content

void PickClassDialog::updateClasses()
{
    classList->removeAllItems();

    ESMS::ESMStore &store = environment.mWorld->getStore();
    
    ESMS::RecListT<ESM::Class>::MapType::const_iterator it = store.classes.list.begin();
    ESMS::RecListT<ESM::Class>::MapType::const_iterator end = store.classes.list.end();
    int index = 0;
    for (; it != end; ++it)
    {
        const ESM::Class &klass = it->second;
        bool playable = (klass.data.isPlayable != 0);
        if (!playable) // Only display playable classes
            continue;

        const std::string &id = it->first;
        classList->addItem(klass.name, id);
        if (boost::iequals(id, currentClassId))
            classList->setIndexSelected(index);
        ++index;
    }
}

void PickClassDialog::updateStats()
{
    if (currentClassId.empty())
        return;
    WindowManager *wm = environment.mWindowManager;
    ESMS::ESMStore &store = environment.mWorld->getStore();
    const ESM::Class *klass = store.classes.find(currentClassId);

    ESM::Class::Specialization specialization = static_cast<ESM::Class::Specialization>(klass->data.specialization);

    static const char *specIds[3] = {
        "sSpecializationCombat",
        "sSpecializationMagic",
        "sSpecializationStealth"
    };
    specializationName->setCaption(wm->getGameSettingString(specIds[specialization], specIds[specialization]));

    favoriteAttribute0->setAttributeId(klass->data.attribute[0]);
    favoriteAttribute1->setAttributeId(klass->data.attribute[1]);

    Widgets::MWSkillPtr majorSkills[5] = {
        majorSkill0,
        majorSkill1,
        majorSkill2,
        majorSkill3,
        majorSkill4
    };
    Widgets::MWSkillPtr minorSkills[5] = {
        minorSkill0,
        minorSkill1,
        minorSkill2,
        minorSkill3,
        minorSkill4
    };

    for (int i = 0; i < 5; ++i)
    {
        majorSkills[i]->setSkillNumber(klass->data.skills[i][0]);
        minorSkills[i]->setSkillNumber(klass->data.skills[i][1]);
    }

    classImage->setImageTexture(std::string("textures\\levelup\\") + currentClassId + ".dds");
}

/* InfoBoxDialog */

void fitToText(MyGUI::StaticTextPtr widget)
{
    MyGUI::IntCoord inner = widget->getTextRegion();
    MyGUI::IntCoord outer = widget->getCoord();
    MyGUI::IntSize size = widget->getTextSize();
    size.width += outer.width - inner.width;
    size.height += outer.height - inner.height;
    widget->setSize(size);
}

void layoutVertically(MyGUI::WidgetPtr widget, int margin)
{
    size_t count = widget->getChildCount();
    int pos = 0;
    pos += margin;
    int width = 0;
    for (unsigned i = 0; i < count; ++i)
    {
        MyGUI::WidgetPtr child = widget->getChildAt(i);
        if (!child->isVisible())
            continue;

        child->setPosition(child->getLeft(), pos);
        width = std::max(width, child->getWidth());
        pos += child->getHeight() + margin;
    }
    width += margin*2;
    widget->setSize(width, pos);
}

InfoBoxDialog::InfoBoxDialog(MWWorld::Environment& environment)
    : Layout("openmw_infobox_layout.xml")
    , environment(environment)
    , currentButton(-1)
{
    getWidget(textBox, "TextBox");
    getWidget(text, "Text");
    text->getSubWidgetText()->setWordWrap(true);
    getWidget(buttonBar, "ButtonBar");

    center();
}

void InfoBoxDialog::setText(const std::string &str)
{
    text->setCaption(str);
    textBox->setVisible(!str.empty());
    fitToText(text);
}

std::string InfoBoxDialog::getText() const
{
    return text->getCaption();
}

void InfoBoxDialog::setButtons(ButtonList &buttons)
{
    for (std::vector<MyGUI::ButtonPtr>::iterator it = this->buttons.begin(); it != this->buttons.end(); ++it)
    {
        MyGUI::Gui::getInstance().destroyWidget(*it);
    }
    this->buttons.clear();
    currentButton = -1;

    // TODO: The buttons should be generated from a template in the layout file, ie. cloning an existing widget
    MyGUI::ButtonPtr button;
    MyGUI::IntCoord coord = MyGUI::IntCoord(0, 0, buttonBar->getWidth(), 10);
    ButtonList::const_iterator end = buttons.end();
    for (ButtonList::const_iterator it = buttons.begin(); it != end; ++it)
    {
        const std::string &text = *it;
        button = buttonBar->createWidget<MyGUI::Button>("MW_Button", coord, MyGUI::Align::Top | MyGUI::Align::HCenter, "");
        button->getSubWidgetText()->setWordWrap(true);
        button->setCaption(text);
        fitToText(button);
        button->eventMouseButtonClick = MyGUI::newDelegate(this, &InfoBoxDialog::onButtonClicked);
        coord.top += button->getHeight();
        this->buttons.push_back(button);
    }
}

void InfoBoxDialog::update()
{
    // Fix layout
    layoutVertically(textBox, 4);
    layoutVertically(buttonBar, 6);
    layoutVertically(mMainWidget, 4 + 6);

    center();
}

int InfoBoxDialog::getChosenButton() const
{
    return currentButton;
}

void InfoBoxDialog::onButtonClicked(MyGUI::WidgetPtr _sender)
{
    std::vector<MyGUI::ButtonPtr>::const_iterator end = buttons.end();
    int i = 0;
    for (std::vector<MyGUI::ButtonPtr>::const_iterator it = buttons.begin(); it != end; ++it)
    {
        if (*it == _sender)
        {
            currentButton = i;
            eventButtonSelected(_sender, i);
            return;
        }
        ++i;
    }
}

void InfoBoxDialog::center()
{
    // Centre dialog
    MyGUI::IntSize gameWindowSize = environment.mWindowManager->getGui()->getViewSize();
    MyGUI::IntCoord coord = mMainWidget->getCoord();
    coord.left = (gameWindowSize.width - coord.width)/2;
    coord.top = (gameWindowSize.height - coord.height)/2;
    mMainWidget->setCoord(coord);
}

/* ClassChoiceDialog */

ClassChoiceDialog::ClassChoiceDialog(MWWorld::Environment& environment)
    : InfoBoxDialog(environment)
{
    WindowManager *mw = environment.mWindowManager;
    setText("");
    ButtonList buttons;
    buttons.push_back(mw->getGameSettingString("sClassChoiceMenu1", ""));
    buttons.push_back(mw->getGameSettingString("sClassChoiceMenu2", ""));
    buttons.push_back(mw->getGameSettingString("sClassChoiceMenu3", ""));
    buttons.push_back(mw->getGameSettingString("sBack", ""));
    setButtons(buttons);

    update();
}

/* CreateClassDialog */

CreateClassDialog::CreateClassDialog(MWWorld::Environment& environment, MyGUI::IntSize gameWindowSize)
  : Layout("openmw_chargen_create_class_layout.xml")
  , environment(environment)
  , specDialog(nullptr)
  , attribDialog(nullptr)
  , skillDialog(nullptr)
  , descDialog(nullptr)
{
    // Centre dialog
    MyGUI::IntCoord coord = mMainWidget->getCoord();
    coord.left = (gameWindowSize.width - coord.width)/2;
    coord.top = (gameWindowSize.height - coord.height)/2;
    mMainWidget->setCoord(coord);

    WindowManager *wm = environment.mWindowManager;
    setText("SpecializationT", wm->getGameSettingString("sChooseClassMenu1", "Specialization"));
    getWidget(specializationName, "SpecializationName");
    specializationName->setCaption(wm->getGameSettingString(ESM::Class::gmstSpecializationIds[ESM::Class::Combat], ""));
    specializationName->eventMouseButtonClick = MyGUI::newDelegate(this, &CreateClassDialog::onSpecializationClicked);

    setText("FavoriteAttributesT", wm->getGameSettingString("sChooseClassMenu2", "Favorite Attributes:"));
    getWidget(favoriteAttribute0, "FavoriteAttribute0");
    getWidget(favoriteAttribute1, "FavoriteAttribute1");
    favoriteAttribute0->setWindowManager(wm);
    favoriteAttribute1->setWindowManager(wm);
    favoriteAttribute0->eventClicked = MyGUI::newDelegate(this, &CreateClassDialog::onAttributeClicked);
    favoriteAttribute1->eventClicked = MyGUI::newDelegate(this, &CreateClassDialog::onAttributeClicked);

    setText("MajorSkillT", wm->getGameSettingString("sSkillClassMajor", ""));
    getWidget(majorSkill0, "MajorSkill0");
    getWidget(majorSkill1, "MajorSkill1");
    getWidget(majorSkill2, "MajorSkill2");
    getWidget(majorSkill3, "MajorSkill3");
    getWidget(majorSkill4, "MajorSkill4");
    skills.push_back(majorSkill0);
    skills.push_back(majorSkill1);
    skills.push_back(majorSkill2);
    skills.push_back(majorSkill3);
    skills.push_back(majorSkill4);

    setText("MinorSkillT", wm->getGameSettingString("sSkillClassMinor", ""));
    getWidget(minorSkill0, "MinorSkill0");
    getWidget(minorSkill1, "MinorSkill1");
    getWidget(minorSkill2, "MinorSkill2");
    getWidget(minorSkill3, "MinorSkill3");
    getWidget(minorSkill4, "MinorSkill4");
    skills.push_back(minorSkill0);
    skills.push_back(minorSkill1);
    skills.push_back(minorSkill2);
    skills.push_back(minorSkill3);
    skills.push_back(minorSkill4);

    std::vector<Widgets::MWSkillPtr>::const_iterator end = skills.end();
    for (std::vector<Widgets::MWSkillPtr>::const_iterator it = skills.begin(); it != end; ++it)
    {
        (*it)->setWindowManager(wm);
        (*it)->eventClicked = MyGUI::newDelegate(this, &CreateClassDialog::onSkillClicked);
    }

    setText("LabelT", wm->getGameSettingString("sName", ""));
    getWidget(editName, "EditName");

    // Make sure the edit box has focus
    MyGUI::InputManager::getInstance().setKeyFocusWidget(editName);

    // TODO: These buttons should be managed by a Dialog class
    MyGUI::ButtonPtr descriptionButton;
    getWidget(descriptionButton, "DescriptionButton");
    descriptionButton->eventMouseButtonClick = MyGUI::newDelegate(this, &CreateClassDialog::onDescriptionClicked);

    MyGUI::ButtonPtr backButton;
    getWidget(backButton, "BackButton");
    backButton->eventMouseButtonClick = MyGUI::newDelegate(this, &CreateClassDialog::onBackClicked);

    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");
    okButton->eventMouseButtonClick = MyGUI::newDelegate(this, &CreateClassDialog::onOkClicked);

    // Set default skills, attributes

    favoriteAttribute0->setAttributeId(ESM::Attribute::Strength);
    favoriteAttribute1->setAttributeId(ESM::Attribute::Agility);

    majorSkill0->setSkillId(ESM::Skill::Block);
    majorSkill1->setSkillId(ESM::Skill::Armorer);
    majorSkill2->setSkillId(ESM::Skill::MediumArmor);
    majorSkill3->setSkillId(ESM::Skill::HeavyArmor);
    majorSkill4->setSkillId(ESM::Skill::BluntWeapon);

    minorSkill0->setSkillId(ESM::Skill::LongBlade);
    minorSkill1->setSkillId(ESM::Skill::Axe);
    minorSkill2->setSkillId(ESM::Skill::Spear);
    minorSkill3->setSkillId(ESM::Skill::Athletics);
    minorSkill4->setSkillId(ESM::Skill::Enchant);
}

CreateClassDialog::~CreateClassDialog()
{
    delete specDialog;
    delete attribDialog;
    delete skillDialog;
    delete descDialog;
}

std::string CreateClassDialog::getName() const
{
    return editName->getOnlyText();
}

std::string CreateClassDialog::getDescription() const
{
    return description;
}

ESM::Class::Specialization CreateClassDialog::getSpecializationId() const
{
    return specializationId;
}

std::vector<int> CreateClassDialog::getFavoriteAttributes() const
{
    std::vector<int> v;
    v.push_back(favoriteAttribute0->getAttributeId());
    v.push_back(favoriteAttribute1->getAttributeId());
    return v;
}

std::vector<ESM::Skill::SkillEnum> CreateClassDialog::getMajorSkills() const
{
    std::vector<ESM::Skill::SkillEnum> v;
    v.push_back(majorSkill0->getSkillId());
    v.push_back(majorSkill1->getSkillId());
    v.push_back(majorSkill2->getSkillId());
    v.push_back(majorSkill3->getSkillId());
    v.push_back(majorSkill4->getSkillId());
    return v;
}

std::vector<ESM::Skill::SkillEnum> CreateClassDialog::getMinorSkills() const
{
    std::vector<ESM::Skill::SkillEnum> v;
    v.push_back(majorSkill0->getSkillId());
    v.push_back(majorSkill1->getSkillId());
    v.push_back(majorSkill2->getSkillId());
    v.push_back(majorSkill3->getSkillId());
    v.push_back(majorSkill4->getSkillId());
    return v;
}

void CreateClassDialog::setNextButtonShow(bool shown)
{
    MyGUI::ButtonPtr descriptionButton;
    getWidget(descriptionButton, "DescriptionButton");

    MyGUI::ButtonPtr backButton;
    getWidget(backButton, "BackButton");

    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");

    // TODO: All hardcoded coords for buttons are temporary, will be replaced with a dynamic system.
    if (shown)
    {
        okButton->setCaption("Next");

        // Adjust back button when next is shown
        descriptionButton->setCoord(MyGUI::IntCoord(207 - 18, 158, 143, 23));
        backButton->setCoord(MyGUI::IntCoord(356 - 18, 158, 53, 23));
        okButton->setCoord(MyGUI::IntCoord(417 - 18, 158, 42 + 18, 23));
    }
    else
    {
        okButton->setCaption("OK");
        descriptionButton->setCoord(MyGUI::IntCoord(207, 158, 143, 23));
        backButton->setCoord(MyGUI::IntCoord(356, 158, 53, 23));
        okButton->setCoord(MyGUI::IntCoord(417, 158, 42, 23));
    }
}

void CreateClassDialog::open()
{
    setVisible(true);
}

// widget controls

void CreateClassDialog::onDialogCancel()
{
    if (specDialog)
        specDialog->setVisible(false);
    if (attribDialog)
        attribDialog->setVisible(false);
    if (skillDialog)
        skillDialog->setVisible(false);
    if (descDialog)
        descDialog->setVisible(false);
    // TODO: Delete dialogs here
}

void CreateClassDialog::onSpecializationClicked(MyGUI::WidgetPtr _sender)
{
    if (specDialog)
        delete specDialog;
    specDialog = new SelectSpecializationDialog(environment, environment.mWindowManager->getGui()->getViewSize());
    specDialog->eventCancel = MyGUI::newDelegate(this, &CreateClassDialog::onDialogCancel);
    specDialog->eventItemSelected = MyGUI::newDelegate(this, &CreateClassDialog::onSpecializationSelected);
    specDialog->setVisible(true);
}

void CreateClassDialog::onSpecializationSelected()
{
    specializationId = specDialog->getSpecializationId();
    specializationName->setCaption(environment.mWindowManager->getGameSettingString(ESM::Class::gmstSpecializationIds[specializationId], ""));
    specDialog->setVisible(false);
}

void CreateClassDialog::onAttributeClicked(Widgets::MWAttributePtr _sender)
{
    if (attribDialog)
        delete attribDialog;
    attribDialog = new SelectAttributeDialog(environment, environment.mWindowManager->getGui()->getViewSize());
    attribDialog->setAffectedWidget(_sender);
    attribDialog->eventCancel = MyGUI::newDelegate(this, &CreateClassDialog::onDialogCancel);
    attribDialog->eventItemSelected = MyGUI::newDelegate(this, &CreateClassDialog::onAttributeSelected);
    attribDialog->setVisible(true);
}

void CreateClassDialog::onAttributeSelected()
{
    ESM::Attribute::AttributeID id = attribDialog->getAttributeId();
    Widgets::MWAttributePtr attribute = attribDialog->getAffectedWidget();
    if (attribute == favoriteAttribute0)
    {
        if (favoriteAttribute1->getAttributeId() == id)
            favoriteAttribute1->setAttributeId(favoriteAttribute0->getAttributeId());
    }
    else if (attribute == favoriteAttribute1)
    {
        if (favoriteAttribute0->getAttributeId() == id)
            favoriteAttribute0->setAttributeId(favoriteAttribute1->getAttributeId());
    }
    attribute->setAttributeId(id);
    attribDialog->setVisible(false);
}

void CreateClassDialog::onSkillClicked(Widgets::MWSkillPtr _sender)
{
    if (skillDialog)
        delete skillDialog;
    skillDialog = new SelectSkillDialog(environment, environment.mWindowManager->getGui()->getViewSize());
    skillDialog->setAffectedWidget(_sender);
    skillDialog->eventCancel = MyGUI::newDelegate(this, &CreateClassDialog::onDialogCancel);
    skillDialog->eventItemSelected = MyGUI::newDelegate(this, &CreateClassDialog::onSkillSelected);
    skillDialog->setVisible(true);
}

void CreateClassDialog::onSkillSelected()
{
    ESM::Skill::SkillEnum id = skillDialog->getSkillId();
    Widgets::MWSkillPtr skill = skillDialog->getAffectedWidget();

    // Avoid duplicate skills by swapping any skill field that matches the selected one
    std::vector<Widgets::MWSkillPtr>::const_iterator end = skills.end();
    for (std::vector<Widgets::MWSkillPtr>::const_iterator it = skills.begin(); it != end; ++it)
    {
        if (*it == skill)
            continue;
        if ((*it)->getSkillId() == id)
        {
            (*it)->setSkillId(skill->getSkillId());
            break;
        }
    }

    skill->setSkillId(skillDialog->getSkillId());
    skillDialog->setVisible(false);
}

void CreateClassDialog::onDescriptionClicked(MyGUI::Widget* _sender)
{
    descDialog = new DescriptionDialog(environment, environment.mWindowManager->getGui()->getViewSize());
    descDialog->setTextInput(description);
    descDialog->eventDone = MyGUI::newDelegate(this, &CreateClassDialog::onDescriptionEntered);
    descDialog->setVisible(true);
}

void CreateClassDialog::onDescriptionEntered()
{
    description = descDialog->getTextInput();
    environment.mWindowManager->removeDialog(descDialog);
}

void CreateClassDialog::onOkClicked(MyGUI::Widget* _sender)
{
    eventDone();
}

void CreateClassDialog::onBackClicked(MyGUI::Widget* _sender)
{
    eventBack();
}

/* SelectSpecializationDialog */

SelectSpecializationDialog::SelectSpecializationDialog(MWWorld::Environment& environment, MyGUI::IntSize gameWindowSize)
  : Layout("openmw_chargen_select_specialization_layout.xml")
{
    // Centre dialog
    MyGUI::IntCoord coord = mMainWidget->getCoord();
    coord.left = (gameWindowSize.width - coord.width)/2;
    coord.top = (gameWindowSize.height - coord.height)/2;
    mMainWidget->setCoord(coord);

    WindowManager *wm = environment.mWindowManager;

    setText("LabelT", wm->getGameSettingString("sSpecializationMenu1", ""));

    getWidget(specialization0, "Specialization0");
    getWidget(specialization1, "Specialization1");
    getWidget(specialization2, "Specialization2");
    specialization0->setCaption(wm->getGameSettingString(ESM::Class::gmstSpecializationIds[ESM::Class::Combat], ""));
    specialization0->eventMouseButtonClick = MyGUI::newDelegate(this, &SelectSpecializationDialog::onSpecializationClicked);
    specialization1->setCaption(wm->getGameSettingString(ESM::Class::gmstSpecializationIds[ESM::Class::Magic], ""));
    specialization1->eventMouseButtonClick = MyGUI::newDelegate(this, &SelectSpecializationDialog::onSpecializationClicked);
    specialization2->setCaption(wm->getGameSettingString(ESM::Class::gmstSpecializationIds[ESM::Class::Stealth], ""));
    specialization2->eventMouseButtonClick = MyGUI::newDelegate(this, &SelectSpecializationDialog::onSpecializationClicked);
    specializationId = ESM::Class::Combat;

    // TODO: These buttons should be managed by a Dialog class
    MyGUI::ButtonPtr cancelButton;
    getWidget(cancelButton, "CancelButton");
    cancelButton->setCaption(wm->getGameSettingString("sCancel", ""));
    cancelButton->eventMouseButtonClick = MyGUI::newDelegate(this, &SelectSpecializationDialog::onCancelClicked);
}

// widget controls

void SelectSpecializationDialog::onSpecializationClicked(MyGUI::WidgetPtr _sender)
{
    if (_sender == specialization0)
        specializationId = ESM::Class::Combat;
    else if (_sender == specialization1)
        specializationId = ESM::Class::Magic;
    else if (_sender == specialization2)
        specializationId = ESM::Class::Stealth;
    else
        return;

    eventItemSelected();
}

void SelectSpecializationDialog::onCancelClicked(MyGUI::Widget* _sender)
{
    eventCancel();
}

/* SelectAttributeDialog */

SelectAttributeDialog::SelectAttributeDialog(MWWorld::Environment& environment, MyGUI::IntSize gameWindowSize)
  : Layout("openmw_chargen_select_attribute_layout.xml")
{
    // Centre dialog
    MyGUI::IntCoord coord = mMainWidget->getCoord();
    coord.left = (gameWindowSize.width - coord.width)/2;
    coord.top = (gameWindowSize.height - coord.height)/2;
    mMainWidget->setCoord(coord);

    WindowManager *wm = environment.mWindowManager;

    setText("LabelT", wm->getGameSettingString("sAttributesMenu1", ""));

    getWidget(attribute0, "Attribute0");
    getWidget(attribute1, "Attribute1");
    getWidget(attribute2, "Attribute2");
    getWidget(attribute3, "Attribute3");
    getWidget(attribute4, "Attribute4");
    getWidget(attribute5, "Attribute5");
    getWidget(attribute6, "Attribute6");
    getWidget(attribute7, "Attribute7");

    Widgets::MWAttributePtr attributes[8] = {
        attribute0,
        attribute1,
        attribute2,
        attribute3,
        attribute4,
        attribute5,
        attribute6,
        attribute7
    };

    for (int i = 0; i < 8; ++i)
    {
        attributes[i]->setWindowManager(wm);
        attributes[i]->setAttributeId(ESM::Attribute::attributeIds[i]);
        attributes[i]->eventClicked = MyGUI::newDelegate(this, &SelectAttributeDialog::onAttributeClicked);
    }

    // TODO: These buttons should be managed by a Dialog class
    MyGUI::ButtonPtr cancelButton;
    getWidget(cancelButton, "CancelButton");
    cancelButton->setCaption(wm->getGameSettingString("sCancel", ""));
    cancelButton->eventMouseButtonClick = MyGUI::newDelegate(this, &SelectAttributeDialog::onCancelClicked);
}

// widget controls

void SelectAttributeDialog::onAttributeClicked(Widgets::MWAttributePtr _sender)
{
    // TODO: Change MWAttribute to set and get AttributeID enum instead of int
    attributeId = static_cast<ESM::Attribute::AttributeID>(_sender->getAttributeId());
    eventItemSelected();
}

void SelectAttributeDialog::onCancelClicked(MyGUI::Widget* _sender)
{
    eventCancel();
}


/* SelectSkillDialog */

SelectSkillDialog::SelectSkillDialog(MWWorld::Environment& environment, MyGUI::IntSize gameWindowSize)
  : Layout("openmw_chargen_select_skill_layout.xml")
{
    // Centre dialog
    MyGUI::IntCoord coord = mMainWidget->getCoord();
    coord.left = (gameWindowSize.width - coord.width)/2;
    coord.top = (gameWindowSize.height - coord.height)/2;
    mMainWidget->setCoord(coord);

    WindowManager *wm = environment.mWindowManager;

    setText("LabelT", wm->getGameSettingString("sSkillsMenu1", ""));
    setText("CombatLabelT", wm->getGameSettingString("sSpecializationCombat", ""));
    setText("MagicLabelT", wm->getGameSettingString("sSpecializationMagic", ""));
    setText("StealthLabelT", wm->getGameSettingString("sSpecializationStealth", ""));

    getWidget(combatSkill0, "CombatSkill0");
    getWidget(combatSkill1, "CombatSkill1");
    getWidget(combatSkill2, "CombatSkill2");
    getWidget(combatSkill3, "CombatSkill3");
    getWidget(combatSkill4, "CombatSkill4");
    getWidget(combatSkill5, "CombatSkill5");
    getWidget(combatSkill6, "CombatSkill6");
    getWidget(combatSkill7, "CombatSkill7");
    getWidget(combatSkill8, "CombatSkill8");

    getWidget(magicSkill0, "MagicSkill0");
    getWidget(magicSkill1, "MagicSkill1");
    getWidget(magicSkill2, "MagicSkill2");
    getWidget(magicSkill3, "MagicSkill3");
    getWidget(magicSkill4, "MagicSkill4");
    getWidget(magicSkill5, "MagicSkill5");
    getWidget(magicSkill6, "MagicSkill6");
    getWidget(magicSkill7, "MagicSkill7");
    getWidget(magicSkill8, "MagicSkill8");

    getWidget(stealthSkill0, "StealthSkill0");
    getWidget(stealthSkill1, "StealthSkill1");
    getWidget(stealthSkill2, "StealthSkill2");
    getWidget(stealthSkill3, "StealthSkill3");
    getWidget(stealthSkill4, "StealthSkill4");
    getWidget(stealthSkill5, "StealthSkill5");
    getWidget(stealthSkill6, "StealthSkill6");
    getWidget(stealthSkill7, "StealthSkill7");
    getWidget(stealthSkill8, "StealthSkill8");

    struct {Widgets::MWSkillPtr widget; ESM::Skill::SkillEnum skillId;} skills[3][9] = {
        {
            {combatSkill0, ESM::Skill::Block},
            {combatSkill1, ESM::Skill::Armorer},
            {combatSkill2, ESM::Skill::MediumArmor},
            {combatSkill3, ESM::Skill::HeavyArmor},
            {combatSkill4, ESM::Skill::BluntWeapon},
            {combatSkill5, ESM::Skill::LongBlade},
            {combatSkill6, ESM::Skill::Axe},
            {combatSkill7, ESM::Skill::Spear},
            {combatSkill8, ESM::Skill::Athletics}
        },   
        {    
            {magicSkill0, ESM::Skill::Enchant},
            {magicSkill1, ESM::Skill::Destruction},
            {magicSkill2, ESM::Skill::Alteration},
            {magicSkill3, ESM::Skill::Illusion},
            {magicSkill4, ESM::Skill::Conjuration},
            {magicSkill5, ESM::Skill::Mysticism},
            {magicSkill6, ESM::Skill::Restoration},
            {magicSkill7, ESM::Skill::Alchemy},
            {magicSkill8, ESM::Skill::Unarmored}
        },   
        {    
            {stealthSkill0, ESM::Skill::Security},
            {stealthSkill1, ESM::Skill::Sneak},
            {stealthSkill2, ESM::Skill::Acrobatics},
            {stealthSkill3, ESM::Skill::LightArmor},
            {stealthSkill4, ESM::Skill::ShortBlade},
            {stealthSkill5 ,ESM::Skill::Marksman},
            {stealthSkill6 ,ESM::Skill::Mercantile},
            {stealthSkill7 ,ESM::Skill::Speechcraft},
            {stealthSkill8 ,ESM::Skill::HandToHand}
        }
    };

    for (int spec = 0; spec < 3; ++spec)
    {
        for (int i = 0; i < 9; ++i)
        {
            skills[spec][i].widget->setWindowManager(wm);
            skills[spec][i].widget->setSkillId(skills[spec][i].skillId);
            skills[spec][i].widget->eventClicked = MyGUI::newDelegate(this, &SelectSkillDialog::onSkillClicked);
        }
    }

    // TODO: These buttons should be managed by a Dialog class
    MyGUI::ButtonPtr cancelButton;
    getWidget(cancelButton, "CancelButton");
    cancelButton->setCaption(wm->getGameSettingString("sCancel", ""));
    cancelButton->eventMouseButtonClick = MyGUI::newDelegate(this, &SelectSkillDialog::onCancelClicked);
}

// widget controls

void SelectSkillDialog::onSkillClicked(Widgets::MWSkillPtr _sender)
{
    skillId = _sender->getSkillId();
    eventItemSelected();
}

void SelectSkillDialog::onCancelClicked(MyGUI::Widget* _sender)
{
    eventCancel();
}

/* DescriptionDialog */

DescriptionDialog::DescriptionDialog(MWWorld::Environment& environment, MyGUI::IntSize gameWindowSize)
  : Layout("openmw_chargen_class_description_layout.xml")
  , environment(environment)
{
    // Centre dialog
    MyGUI::IntCoord coord = mMainWidget->getCoord();
    coord.left = (gameWindowSize.width - coord.width)/2;
    coord.top = (gameWindowSize.height - coord.height)/2;
    mMainWidget->setCoord(coord);

    getWidget(textEdit, "TextEdit");

    // TODO: These buttons should be managed by a Dialog class
    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");
    okButton->eventMouseButtonClick = MyGUI::newDelegate(this, &DescriptionDialog::onOkClicked);
    okButton->setCaption(environment.mWindowManager->getGameSettingString("sInputMenu1", ""));

    // Make sure the edit box has focus
    MyGUI::InputManager::getInstance().setKeyFocusWidget(textEdit);
}

// widget controls

void DescriptionDialog::onOkClicked(MyGUI::Widget* _sender)
{
    eventDone();
}
