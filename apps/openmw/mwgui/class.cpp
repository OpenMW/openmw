#include "class.hpp"

#include <assert.h>
#include <iterator>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <components/esm_store/store.hpp>

#include "window_manager.hpp"
#include "tooltips.hpp"

#undef min
#undef max

using namespace MWGui;

/* GenerateClassResultDialog */

GenerateClassResultDialog::GenerateClassResultDialog(WindowManager& parWindowManager)
  : WindowBase("openmw_chargen_generate_class_result_layout.xml", parWindowManager)
{
    // Centre dialog
    center();

    setText("ReflectT", mWindowManager.getGameSettingString("sMessageQuestionAnswer1", ""));

    getWidget(classImage, "ClassImage");
    getWidget(className, "ClassName");

    MyGUI::ButtonPtr backButton;
    getWidget(backButton, "BackButton");
    backButton->eventMouseButtonClick += MyGUI::newDelegate(this, &GenerateClassResultDialog::onBackClicked);

    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");
    okButton->setCaption(mWindowManager.getGameSettingString("sOK", ""));
    okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &GenerateClassResultDialog::onOkClicked);

    int okButtonWidth = okButton->getTextSize().width + 24;
    int backButtonWidth = backButton->getTextSize().width + 24;
    okButton->setCoord(315 - okButtonWidth, 219, okButtonWidth, 23);
    backButton->setCoord(315 - okButtonWidth - backButtonWidth - 6, 219, backButtonWidth, 23);
}

void GenerateClassResultDialog::open()
{
    setVisible(true);
}

std::string GenerateClassResultDialog::getClassId() const
{
    return className->getCaption();
}

void GenerateClassResultDialog::setClassId(const std::string &classId)
{
    currentClassId = classId;
    classImage->setImageTexture(std::string("textures\\levelup\\") + currentClassId + ".dds");
    const ESMS::ESMStore &store = mWindowManager.getStore();
    className->setCaption(store.classes.find(currentClassId)->name);
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

PickClassDialog::PickClassDialog(WindowManager& parWindowManager)
  : WindowBase("openmw_chargen_class_layout.xml", parWindowManager)
{
    // Centre dialog
    center();

    getWidget(specializationName, "SpecializationName");

    getWidget(favoriteAttribute[0], "FavoriteAttribute0");
    getWidget(favoriteAttribute[1], "FavoriteAttribute1");
    favoriteAttribute[0]->setWindowManager(&mWindowManager);
    favoriteAttribute[1]->setWindowManager(&mWindowManager);

    for(int i = 0; i < 5; i++)
    {
        char theIndex = '0'+i;
        getWidget(majorSkill[i], std::string("MajorSkill").append(1, theIndex));
        getWidget(minorSkill[i], std::string("MinorSkill").append(1, theIndex));
        majorSkill[i]->setWindowManager(&mWindowManager);
        minorSkill[i]->setWindowManager(&mWindowManager);
    }

    getWidget(classList, "ClassList");
    classList->setScrollVisible(true);
    classList->eventListSelectAccept += MyGUI::newDelegate(this, &PickClassDialog::onSelectClass);
    classList->eventListMouseItemActivate += MyGUI::newDelegate(this, &PickClassDialog::onSelectClass);
    classList->eventListChangePosition += MyGUI::newDelegate(this, &PickClassDialog::onSelectClass);

    getWidget(classImage, "ClassImage");

    MyGUI::ButtonPtr backButton;
    getWidget(backButton, "BackButton");
    backButton->eventMouseButtonClick += MyGUI::newDelegate(this, &PickClassDialog::onBackClicked);

    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");
    okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &PickClassDialog::onOkClicked);

    updateClasses();
    updateStats();
}

void PickClassDialog::setNextButtonShow(bool shown)
{
    MyGUI::ButtonPtr backButton;
    getWidget(backButton, "BackButton");

    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");

    if (shown)
        okButton->setCaption(mWindowManager.getGameSettingString("sNext", ""));
    else
        okButton->setCaption(mWindowManager.getGameSettingString("sOK", ""));

    int okButtonWidth = okButton->getTextSize().width + 24;
    int backButtonWidth = backButton->getTextSize().width + 24;

    okButton->setCoord(476 - okButtonWidth, 265, okButtonWidth, 23);
    backButton->setCoord(476 - okButtonWidth - backButtonWidth - 6, 265, backButtonWidth, 23);
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
    eventDone(this);
}

void PickClassDialog::onBackClicked(MyGUI::Widget* _sender)
{
    eventBack();
}

void PickClassDialog::onSelectClass(MyGUI::ListBox* _sender, size_t _index)
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

    const ESMS::ESMStore &store = mWindowManager.getStore();

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
    const ESMS::ESMStore &store = mWindowManager.getStore();
    const ESM::Class *klass = store.classes.search(currentClassId);
    if (!klass)
        return;

    ESM::Class::Specialization specialization = static_cast<ESM::Class::Specialization>(klass->data.specialization);

    static const char *specIds[3] = {
        "sSpecializationCombat",
        "sSpecializationMagic",
        "sSpecializationStealth"
    };
    std::string specName = mWindowManager.getGameSettingString(specIds[specialization], specIds[specialization]);
    specializationName->setCaption(specName);
    ToolTips::createSpecializationToolTip(specializationName, specName, specialization);

    favoriteAttribute[0]->setAttributeId(klass->data.attribute[0]);
    favoriteAttribute[1]->setAttributeId(klass->data.attribute[1]);
    ToolTips::createAttributeToolTip(favoriteAttribute[0], favoriteAttribute[0]->getAttributeId());
    ToolTips::createAttributeToolTip(favoriteAttribute[1], favoriteAttribute[1]->getAttributeId());

    for (int i = 0; i < 5; ++i)
    {
        majorSkill[i]->setSkillNumber(klass->data.skills[i][0]);
        minorSkill[i]->setSkillNumber(klass->data.skills[i][1]);
        ToolTips::createSkillToolTip(majorSkill[i], klass->data.skills[i][0]);
        ToolTips::createSkillToolTip(minorSkill[i], klass->data.skills[i][1]);
    }

    classImage->setImageTexture(std::string("textures\\levelup\\") + currentClassId + ".dds");
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

void InfoBoxDialog::layoutVertically(MyGUI::WidgetPtr widget, int margin)
{
    size_t count = widget->getChildCount();
    int pos = 0;
    pos += margin;
    int width = 0;
    for (unsigned i = 0; i < count; ++i)
    {
        MyGUI::WidgetPtr child = widget->getChildAt(i);
        if (!child->getVisible())
            continue;

        child->setPosition(child->getLeft(), pos);
        width = std::max(width, child->getWidth());
        pos += child->getHeight() + margin;
    }
    width += margin*2;
    widget->setSize(width, pos);
}

InfoBoxDialog::InfoBoxDialog(WindowManager& parWindowManager)
    : WindowBase("openmw_infobox_layout.xml", parWindowManager)
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
        button->eventMouseButtonClick += MyGUI::newDelegate(this, &InfoBoxDialog::onButtonClicked);
        coord.top += button->getHeight();
        this->buttons.push_back(button);
    }
}

void InfoBoxDialog::open()
{
    // Fix layout
    layoutVertically(textBox, 4);
    layoutVertically(buttonBar, 6);
    layoutVertically(mMainWidget, 4 + 6);

    center();
    setVisible(true);
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
            eventButtonSelected(i);
            return;
        }
        ++i;
    }
}

/* ClassChoiceDialog */

ClassChoiceDialog::ClassChoiceDialog(WindowManager& parWindowManager)
    : InfoBoxDialog(parWindowManager)
{
    setText("");
    ButtonList buttons;
    buttons.push_back(mWindowManager.getGameSettingString("sClassChoiceMenu1", ""));
    buttons.push_back(mWindowManager.getGameSettingString("sClassChoiceMenu2", ""));
    buttons.push_back(mWindowManager.getGameSettingString("sClassChoiceMenu3", ""));
    buttons.push_back(mWindowManager.getGameSettingString("sBack", ""));
    setButtons(buttons);
}

/* CreateClassDialog */

CreateClassDialog::CreateClassDialog(WindowManager& parWindowManager)
  : WindowBase("openmw_chargen_create_class_layout.xml", parWindowManager)
  , specDialog(nullptr)
  , attribDialog(nullptr)
  , skillDialog(nullptr)
  , descDialog(nullptr)
{
    // Centre dialog
    center();

    setText("SpecializationT", mWindowManager.getGameSettingString("sChooseClassMenu1", "Specialization"));
    getWidget(specializationName, "SpecializationName");
    specializationName->eventMouseButtonClick += MyGUI::newDelegate(this, &CreateClassDialog::onSpecializationClicked);

    setText("FavoriteAttributesT", mWindowManager.getGameSettingString("sChooseClassMenu2", "Favorite Attributes:"));
    getWidget(favoriteAttribute0, "FavoriteAttribute0");
    getWidget(favoriteAttribute1, "FavoriteAttribute1");
    favoriteAttribute0->setWindowManager(&mWindowManager);
    favoriteAttribute1->setWindowManager(&mWindowManager);
    favoriteAttribute0->eventClicked += MyGUI::newDelegate(this, &CreateClassDialog::onAttributeClicked);
    favoriteAttribute1->eventClicked += MyGUI::newDelegate(this, &CreateClassDialog::onAttributeClicked);

    setText("MajorSkillT", mWindowManager.getGameSettingString("sSkillClassMajor", ""));
    setText("MinorSkillT", mWindowManager.getGameSettingString("sSkillClassMinor", ""));
    for(int i = 0; i < 5; i++)
    {
        char theIndex = '0'+i;
        getWidget(majorSkill[i], std::string("MajorSkill").append(1, theIndex));
        getWidget(minorSkill[i], std::string("MinorSkill").append(1, theIndex));
        skills.push_back(majorSkill[i]);
        skills.push_back(minorSkill[i]);
    }

    std::vector<Widgets::MWSkillPtr>::const_iterator end = skills.end();
    for (std::vector<Widgets::MWSkillPtr>::const_iterator it = skills.begin(); it != end; ++it)
    {
        (*it)->setWindowManager(&mWindowManager);
        (*it)->eventClicked += MyGUI::newDelegate(this, &CreateClassDialog::onSkillClicked);
    }

    setText("LabelT", mWindowManager.getGameSettingString("sName", ""));
    getWidget(editName, "EditName");

    // Make sure the edit box has focus
    MyGUI::InputManager::getInstance().setKeyFocusWidget(editName);

    MyGUI::ButtonPtr descriptionButton;
    getWidget(descriptionButton, "DescriptionButton");
    descriptionButton->setCaption(mWindowManager.getGameSettingString("sCreateClassMenu1", ""));
    descriptionButton->eventMouseButtonClick += MyGUI::newDelegate(this, &CreateClassDialog::onDescriptionClicked);

    MyGUI::ButtonPtr backButton;
    getWidget(backButton, "BackButton");
    backButton->eventMouseButtonClick += MyGUI::newDelegate(this, &CreateClassDialog::onBackClicked);

    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");
    okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &CreateClassDialog::onOkClicked);

    // Set default skills, attributes

    favoriteAttribute0->setAttributeId(ESM::Attribute::Strength);
    favoriteAttribute1->setAttributeId(ESM::Attribute::Agility);

    majorSkill[0]->setSkillId(ESM::Skill::Block);
    majorSkill[1]->setSkillId(ESM::Skill::Armorer);
    majorSkill[2]->setSkillId(ESM::Skill::MediumArmor);
    majorSkill[3]->setSkillId(ESM::Skill::HeavyArmor);
    majorSkill[4]->setSkillId(ESM::Skill::BluntWeapon);

    minorSkill[0]->setSkillId(ESM::Skill::LongBlade);
    minorSkill[1]->setSkillId(ESM::Skill::Axe);
    minorSkill[2]->setSkillId(ESM::Skill::Spear);
    minorSkill[3]->setSkillId(ESM::Skill::Athletics);
    minorSkill[4]->setSkillId(ESM::Skill::Enchant);

    setSpecialization(0);
    update();
}

CreateClassDialog::~CreateClassDialog()
{
    delete specDialog;
    delete attribDialog;
    delete skillDialog;
    delete descDialog;
}

void CreateClassDialog::update()
{
    for (int i = 0; i < 5; ++i)
    {
        ToolTips::createSkillToolTip(majorSkill[i], majorSkill[i]->getSkillId());
        ToolTips::createSkillToolTip(minorSkill[i], minorSkill[i]->getSkillId());
    }

    ToolTips::createAttributeToolTip(favoriteAttribute0, favoriteAttribute0->getAttributeId());
    ToolTips::createAttributeToolTip(favoriteAttribute1, favoriteAttribute1->getAttributeId());
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
    for(int i = 0; i < 5; i++)
    {
        v.push_back(majorSkill[i]->getSkillId());
    }
    return v;
}

std::vector<ESM::Skill::SkillEnum> CreateClassDialog::getMinorSkills() const
{
    std::vector<ESM::Skill::SkillEnum> v;
    for(int i=0; i < 5; i++)
    {
        v.push_back(majorSkill[i]->getSkillId());
    }
    return v;
}

void CreateClassDialog::setNextButtonShow(bool shown)
{
    MyGUI::ButtonPtr backButton;
    getWidget(backButton, "BackButton");

    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");

    MyGUI::ButtonPtr descriptionButton;
    getWidget(descriptionButton, "DescriptionButton");

    if (shown)
        okButton->setCaption(mWindowManager.getGameSettingString("sNext", ""));
    else
        okButton->setCaption(mWindowManager.getGameSettingString("sOK", ""));

    int okButtonWidth = okButton->getTextSize().width + 24;
    int backButtonWidth = backButton->getTextSize().width + 24;
    int descriptionButtonWidth = descriptionButton->getTextSize().width + 24;

    okButton->setCoord(459 - okButtonWidth, 158, okButtonWidth, 23);
    backButton->setCoord(459 - okButtonWidth - backButtonWidth - 6, 158, backButtonWidth, 23);
    descriptionButton->setCoord(459 - okButtonWidth - backButtonWidth - descriptionButtonWidth - 12, 158, descriptionButtonWidth, 23);
}

void CreateClassDialog::open()
{
    setVisible(true);
}

// widget controls

void CreateClassDialog::onDialogCancel()
{
    if (specDialog)
    {
        mWindowManager.removeDialog(specDialog);
        specDialog = 0;
    }
    if (attribDialog)
    {
        mWindowManager.removeDialog(attribDialog);
        attribDialog = 0;
    }
    if (skillDialog)
    {
        mWindowManager.removeDialog(skillDialog);
        skillDialog = 0;
    }
    if (descDialog)
    {
        mWindowManager.removeDialog(descDialog);
        descDialog = 0;
    }
}

void CreateClassDialog::onSpecializationClicked(MyGUI::WidgetPtr _sender)
{
    if (specDialog)
        delete specDialog;
    specDialog = new SelectSpecializationDialog(mWindowManager);
    specDialog->eventCancel += MyGUI::newDelegate(this, &CreateClassDialog::onDialogCancel);
    specDialog->eventItemSelected += MyGUI::newDelegate(this, &CreateClassDialog::onSpecializationSelected);
    specDialog->setVisible(true);
}

void CreateClassDialog::onSpecializationSelected()
{
    specializationId = specDialog->getSpecializationId();
    setSpecialization(specializationId);

    mWindowManager.removeDialog(specDialog);
    specDialog = 0;
}

void CreateClassDialog::setSpecialization(int id)
{
    specializationId = (ESM::Class::Specialization) id;
    static const char *specIds[3] = {
        "sSpecializationCombat",
        "sSpecializationMagic",
        "sSpecializationStealth"
    };
    std::string specName = mWindowManager.getGameSettingString(specIds[specializationId], specIds[specializationId]);
    specializationName->setCaption(specName);
    ToolTips::createSpecializationToolTip(specializationName, specName, specializationId);
}

void CreateClassDialog::onAttributeClicked(Widgets::MWAttributePtr _sender)
{
    if (attribDialog)
        delete attribDialog;
    attribDialog = new SelectAttributeDialog(mWindowManager);
    attribDialog->setAffectedWidget(_sender);
    attribDialog->eventCancel += MyGUI::newDelegate(this, &CreateClassDialog::onDialogCancel);
    attribDialog->eventItemSelected += MyGUI::newDelegate(this, &CreateClassDialog::onAttributeSelected);
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
    mWindowManager.removeDialog(attribDialog);
    attribDialog = 0;

    update();
}

void CreateClassDialog::onSkillClicked(Widgets::MWSkillPtr _sender)
{
    if (skillDialog)
        delete skillDialog;
    skillDialog = new SelectSkillDialog(mWindowManager);
    skillDialog->setAffectedWidget(_sender);
    skillDialog->eventCancel += MyGUI::newDelegate(this, &CreateClassDialog::onDialogCancel);
    skillDialog->eventItemSelected += MyGUI::newDelegate(this, &CreateClassDialog::onSkillSelected);
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
    mWindowManager.removeDialog(skillDialog);
    skillDialog = 0;
    update();
}

void CreateClassDialog::onDescriptionClicked(MyGUI::Widget* _sender)
{
    descDialog = new DescriptionDialog(mWindowManager);
    descDialog->setTextInput(description);
    descDialog->eventDone += MyGUI::newDelegate(this, &CreateClassDialog::onDescriptionEntered);
    descDialog->setVisible(true);
}

void CreateClassDialog::onDescriptionEntered(WindowBase* parWindow)
{
    description = descDialog->getTextInput();
    mWindowManager.removeDialog(descDialog);
    descDialog = 0;
}

void CreateClassDialog::onOkClicked(MyGUI::Widget* _sender)
{
    eventDone(this);
}

void CreateClassDialog::onBackClicked(MyGUI::Widget* _sender)
{
    eventBack();
}

/* SelectSpecializationDialog */

SelectSpecializationDialog::SelectSpecializationDialog(WindowManager& parWindowManager)
  : WindowBase("openmw_chargen_select_specialization_layout.xml", parWindowManager)
{
    // Centre dialog
    center();

    setText("LabelT", mWindowManager.getGameSettingString("sSpecializationMenu1", ""));

    getWidget(specialization0, "Specialization0");
    getWidget(specialization1, "Specialization1");
    getWidget(specialization2, "Specialization2");
    std::string combat = mWindowManager.getGameSettingString(ESM::Class::gmstSpecializationIds[ESM::Class::Combat], "");
    std::string magic = mWindowManager.getGameSettingString(ESM::Class::gmstSpecializationIds[ESM::Class::Magic], "");
    std::string stealth = mWindowManager.getGameSettingString(ESM::Class::gmstSpecializationIds[ESM::Class::Stealth], "");

    specialization0->setCaption(combat);
    specialization0->eventMouseButtonClick += MyGUI::newDelegate(this, &SelectSpecializationDialog::onSpecializationClicked);
    specialization1->setCaption(magic);
    specialization1->eventMouseButtonClick += MyGUI::newDelegate(this, &SelectSpecializationDialog::onSpecializationClicked);
    specialization2->setCaption(stealth);
    specialization2->eventMouseButtonClick += MyGUI::newDelegate(this, &SelectSpecializationDialog::onSpecializationClicked);
    specializationId = ESM::Class::Combat;

    ToolTips::createSpecializationToolTip(specialization0, combat, ESM::Class::Combat);
    ToolTips::createSpecializationToolTip(specialization1, magic, ESM::Class::Magic);
    ToolTips::createSpecializationToolTip(specialization2, stealth, ESM::Class::Stealth);

    MyGUI::ButtonPtr cancelButton;
    getWidget(cancelButton, "CancelButton");
    cancelButton->setCaption(mWindowManager.getGameSettingString("sCancel", ""));
    cancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SelectSpecializationDialog::onCancelClicked);
    int buttonWidth = cancelButton->getTextSize().width + 24;
    cancelButton->setCoord(216 - buttonWidth, 90, buttonWidth, 21);

    MyGUI::InputManager::getInstance().addWidgetModal(mMainWidget);
}

SelectSpecializationDialog::~SelectSpecializationDialog()
{
    MyGUI::InputManager::getInstance().removeWidgetModal(mMainWidget);
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

SelectAttributeDialog::SelectAttributeDialog(WindowManager& parWindowManager)
  : WindowBase("openmw_chargen_select_attribute_layout.xml", parWindowManager)
{
    // Centre dialog
    center();

    setText("LabelT", mWindowManager.getGameSettingString("sAttributesMenu1", ""));

    for (int i = 0; i < 8; ++i)
    {
        Widgets::MWAttributePtr attribute;
        char theIndex = '0'+i;

        getWidget(attribute,  std::string("Attribute").append(1, theIndex));
        attribute->setWindowManager(&parWindowManager);
        attribute->setAttributeId(ESM::Attribute::attributeIds[i]);
        attribute->eventClicked += MyGUI::newDelegate(this, &SelectAttributeDialog::onAttributeClicked);
        ToolTips::createAttributeToolTip(attribute, attribute->getAttributeId());
    }

    MyGUI::ButtonPtr cancelButton;
    getWidget(cancelButton, "CancelButton");
    cancelButton->setCaption(mWindowManager.getGameSettingString("sCancel", ""));
    cancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SelectAttributeDialog::onCancelClicked);
    int buttonWidth = cancelButton->getTextSize().width + 24;
    cancelButton->setCoord(186 - buttonWidth, 180, buttonWidth, 21);

    MyGUI::InputManager::getInstance().addWidgetModal(mMainWidget);
}

SelectAttributeDialog::~SelectAttributeDialog()
{
    MyGUI::InputManager::getInstance().removeWidgetModal(mMainWidget);
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

SelectSkillDialog::SelectSkillDialog(WindowManager& parWindowManager)
  : WindowBase("openmw_chargen_select_skill_layout.xml", parWindowManager)
{
    // Centre dialog
    center();

    setText("LabelT", mWindowManager.getGameSettingString("sSkillsMenu1", ""));
    setText("CombatLabelT", mWindowManager.getGameSettingString("sSpecializationCombat", ""));
    setText("MagicLabelT", mWindowManager.getGameSettingString("sSpecializationMagic", ""));
    setText("StealthLabelT", mWindowManager.getGameSettingString("sSpecializationStealth", ""));

    for(int i = 0; i < 9; i++)
    {
        char theIndex = '0'+i;
        getWidget(combatSkill[i],  std::string("CombatSkill").append(1, theIndex));
        getWidget(magicSkill[i],   std::string("MagicSkill").append(1, theIndex));
        getWidget(stealthSkill[i], std::string("StealthSkill").append(1, theIndex));
    }

    struct {Widgets::MWSkillPtr widget; ESM::Skill::SkillEnum skillId;} skills[3][9] = {
        {
            {combatSkill[0], ESM::Skill::Block},
            {combatSkill[1], ESM::Skill::Armorer},
            {combatSkill[2], ESM::Skill::MediumArmor},
            {combatSkill[3], ESM::Skill::HeavyArmor},
            {combatSkill[4], ESM::Skill::BluntWeapon},
            {combatSkill[5], ESM::Skill::LongBlade},
            {combatSkill[6], ESM::Skill::Axe},
            {combatSkill[7], ESM::Skill::Spear},
            {combatSkill[8], ESM::Skill::Athletics}
        },
        {
            {magicSkill[0], ESM::Skill::Enchant},
            {magicSkill[1], ESM::Skill::Destruction},
            {magicSkill[2], ESM::Skill::Alteration},
            {magicSkill[3], ESM::Skill::Illusion},
            {magicSkill[4], ESM::Skill::Conjuration},
            {magicSkill[5], ESM::Skill::Mysticism},
            {magicSkill[6], ESM::Skill::Restoration},
            {magicSkill[7], ESM::Skill::Alchemy},
            {magicSkill[8], ESM::Skill::Unarmored}
        },
        {
            {stealthSkill[0], ESM::Skill::Security},
            {stealthSkill[1], ESM::Skill::Sneak},
            {stealthSkill[2], ESM::Skill::Acrobatics},
            {stealthSkill[3], ESM::Skill::LightArmor},
            {stealthSkill[4], ESM::Skill::ShortBlade},
            {stealthSkill[5] ,ESM::Skill::Marksman},
            {stealthSkill[6] ,ESM::Skill::Mercantile},
            {stealthSkill[7] ,ESM::Skill::Speechcraft},
            {stealthSkill[8] ,ESM::Skill::HandToHand}
        }
    };

    for (int spec = 0; spec < 3; ++spec)
    {
        for (int i = 0; i < 9; ++i)
        {
            skills[spec][i].widget->setWindowManager(&mWindowManager);
            skills[spec][i].widget->setSkillId(skills[spec][i].skillId);
            skills[spec][i].widget->eventClicked += MyGUI::newDelegate(this, &SelectSkillDialog::onSkillClicked);
            ToolTips::createSkillToolTip(skills[spec][i].widget, skills[spec][i].widget->getSkillId());
        }
    }

    MyGUI::ButtonPtr cancelButton;
    getWidget(cancelButton, "CancelButton");
    cancelButton->setCaption(mWindowManager.getGameSettingString("sCancel", ""));
    cancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SelectSkillDialog::onCancelClicked);
    int buttonWidth = cancelButton->getTextSize().width + 24;
    cancelButton->setCoord(447 - buttonWidth, 218, buttonWidth, 21);

    MyGUI::InputManager::getInstance().addWidgetModal(mMainWidget);
}

SelectSkillDialog::~SelectSkillDialog()
{
    MyGUI::InputManager::getInstance().removeWidgetModal(mMainWidget);
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

DescriptionDialog::DescriptionDialog(WindowManager& parWindowManager)
  : WindowBase("openmw_chargen_class_description_layout.xml", parWindowManager)
{
    // Centre dialog
    center();

    getWidget(textEdit, "TextEdit");

    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");
    okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DescriptionDialog::onOkClicked);
    okButton->setCaption(mWindowManager.getGameSettingString("sInputMenu1", ""));
    int buttonWidth = okButton->getTextSize().width + 24;
    okButton->setCoord(234 - buttonWidth, 214, buttonWidth, 24);

    // Make sure the edit box has focus
    MyGUI::InputManager::getInstance().setKeyFocusWidget(textEdit);

    MyGUI::InputManager::getInstance().addWidgetModal(mMainWidget);
}

DescriptionDialog::~DescriptionDialog()
{
    MyGUI::InputManager::getInstance().removeWidgetModal(mMainWidget);
}

// widget controls

void DescriptionDialog::onOkClicked(MyGUI::Widget* _sender)
{
    eventDone(this);
}
