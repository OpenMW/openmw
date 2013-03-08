#include "class.hpp"

#include <iterator>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "tooltips.hpp"

#undef min
#undef max

using namespace MWGui;

/* GenerateClassResultDialog */

GenerateClassResultDialog::GenerateClassResultDialog(MWBase::WindowManager& parWindowManager)
  : WindowModal("openmw_chargen_generate_class_result.layout", parWindowManager)
{
    // Centre dialog
    center();

    setText("ReflectT", mWindowManager.getGameSettingString("sMessageQuestionAnswer1", ""));

    getWidget(mClassImage, "ClassImage");
    getWidget(mClassName, "ClassName");

    MyGUI::Button* backButton;
    getWidget(backButton, "BackButton");
    backButton->eventMouseButtonClick += MyGUI::newDelegate(this, &GenerateClassResultDialog::onBackClicked);

    MyGUI::Button* okButton;
    getWidget(okButton, "OKButton");
    okButton->setCaption(mWindowManager.getGameSettingString("sOK", ""));
    okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &GenerateClassResultDialog::onOkClicked);
}

std::string GenerateClassResultDialog::getClassId() const
{
    return mClassName->getCaption();
}

void GenerateClassResultDialog::setClassId(const std::string &classId)
{
    mCurrentClassId = classId;
    mClassImage->setImageTexture(std::string("textures\\levelup\\") + mCurrentClassId + ".dds");
    mClassName->setCaption(MWBase::Environment::get().getWorld()->getStore().get<ESM::Class>().find(mCurrentClassId)->mName);
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

PickClassDialog::PickClassDialog(MWBase::WindowManager& parWindowManager)
  : WindowModal("openmw_chargen_class.layout", parWindowManager)
{
    // Centre dialog
    center();

    getWidget(mSpecializationName, "SpecializationName");

    getWidget(mFavoriteAttribute[0], "FavoriteAttribute0");
    getWidget(mFavoriteAttribute[1], "FavoriteAttribute1");
    mFavoriteAttribute[0]->setWindowManager(&mWindowManager);
    mFavoriteAttribute[1]->setWindowManager(&mWindowManager);

    for(int i = 0; i < 5; i++)
    {
        char theIndex = '0'+i;
        getWidget(mMajorSkill[i], std::string("MajorSkill").append(1, theIndex));
        getWidget(mMinorSkill[i], std::string("MinorSkill").append(1, theIndex));
        mMajorSkill[i]->setWindowManager(&mWindowManager);
        mMinorSkill[i]->setWindowManager(&mWindowManager);
    }

    getWidget(mClassList, "ClassList");
    mClassList->setScrollVisible(true);
    mClassList->eventListSelectAccept += MyGUI::newDelegate(this, &PickClassDialog::onSelectClass);
    mClassList->eventListMouseItemActivate += MyGUI::newDelegate(this, &PickClassDialog::onSelectClass);
    mClassList->eventListChangePosition += MyGUI::newDelegate(this, &PickClassDialog::onSelectClass);

    getWidget(mClassImage, "ClassImage");

    MyGUI::Button* backButton;
    getWidget(backButton, "BackButton");
    backButton->eventMouseButtonClick += MyGUI::newDelegate(this, &PickClassDialog::onBackClicked);

    MyGUI::Button* okButton;
    getWidget(okButton, "OKButton");
    okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &PickClassDialog::onOkClicked);

    updateClasses();
    updateStats();
}

void PickClassDialog::setNextButtonShow(bool shown)
{
    MyGUI::Button* okButton;
    getWidget(okButton, "OKButton");

    if (shown)
        okButton->setCaption(mWindowManager.getGameSettingString("sNext", ""));
    else
        okButton->setCaption(mWindowManager.getGameSettingString("sOK", ""));
}

void PickClassDialog::open()
{
    WindowModal::open ();
    updateClasses();
    updateStats();
}


void PickClassDialog::setClassId(const std::string &classId)
{
    mCurrentClassId = classId;
    mClassList->setIndexSelected(MyGUI::ITEM_NONE);
    size_t count = mClassList->getItemCount();
    for (size_t i = 0; i < count; ++i)
    {
        if (boost::iequals(*mClassList->getItemDataAt<std::string>(i), classId))
        {
            mClassList->setIndexSelected(i);
            MyGUI::Button* okButton;
            getWidget(okButton, "OKButton");
            break;
        }
    }

    updateStats();
}

// widget controls

void PickClassDialog::onOkClicked(MyGUI::Widget* _sender)
{
    if(mClassList->getIndexSelected() == MyGUI::ITEM_NONE)
        return;
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

    MyGUI::Button* okButton;
    getWidget(okButton, "OKButton");

    const std::string *classId = mClassList->getItemDataAt<std::string>(_index);
    if (boost::iequals(mCurrentClassId, *classId))
        return;

    mCurrentClassId = *classId;
    updateStats();
}

// update widget content

void PickClassDialog::updateClasses()
{
    mClassList->removeAllItems();

    const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();

    int index = 0;
    MWWorld::Store<ESM::Class>::iterator it = store.get<ESM::Class>().begin();
    for (; it != store.get<ESM::Class>().end(); ++it)
    {
        bool playable = (it->mData.mIsPlayable != 0);
        if (!playable) // Only display playable classes
            continue;

        const std::string &id = it->mId;
        mClassList->addItem(it->mName, id);
        if (mCurrentClassId.empty())
        {
            mCurrentClassId = id;
            mClassList->setIndexSelected(index);
        }
        else if (boost::iequals(id, mCurrentClassId))
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
    const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
    const ESM::Class *klass = store.get<ESM::Class>().search(mCurrentClassId);
    if (!klass)
        return;

    ESM::Class::Specialization specialization = static_cast<ESM::Class::Specialization>(klass->mData.mSpecialization);

    static const char *specIds[3] = {
        "sSpecializationCombat",
        "sSpecializationMagic",
        "sSpecializationStealth"
    };
    std::string specName = mWindowManager.getGameSettingString(specIds[specialization], specIds[specialization]);
    mSpecializationName->setCaption(specName);
    ToolTips::createSpecializationToolTip(mSpecializationName, specName, specialization);

    mFavoriteAttribute[0]->setAttributeId(klass->mData.mAttribute[0]);
    mFavoriteAttribute[1]->setAttributeId(klass->mData.mAttribute[1]);
    ToolTips::createAttributeToolTip(mFavoriteAttribute[0], mFavoriteAttribute[0]->getAttributeId());
    ToolTips::createAttributeToolTip(mFavoriteAttribute[1], mFavoriteAttribute[1]->getAttributeId());

    for (int i = 0; i < 5; ++i)
    {
        mMinorSkill[i]->setSkillNumber(klass->mData.mSkills[i][0]);
        mMajorSkill[i]->setSkillNumber(klass->mData.mSkills[i][1]);
        ToolTips::createSkillToolTip(mMinorSkill[i], klass->mData.mSkills[i][0]);
        ToolTips::createSkillToolTip(mMajorSkill[i], klass->mData.mSkills[i][1]);
    }

    mClassImage->setImageTexture(std::string("textures\\levelup\\") + mCurrentClassId + ".dds");
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
    width += margin*2;
    widget->setSize(width, pos);
}

InfoBoxDialog::InfoBoxDialog(MWBase::WindowManager& parWindowManager)
    : WindowModal("openmw_infobox.layout", parWindowManager)
    , mCurrentButton(-1)
{
    getWidget(mTextBox, "TextBox");
    getWidget(mText, "Text");
    mText->getSubWidgetText()->setWordWrap(true);
    getWidget(mButtonBar, "ButtonBar");

    center();
}

void InfoBoxDialog::setText(const std::string &str)
{
    mText->setCaption(str);
    mTextBox->setVisible(!str.empty());
    fitToText(mText);
}

std::string InfoBoxDialog::getText() const
{
    return mText->getCaption();
}

void InfoBoxDialog::setButtons(ButtonList &buttons)
{
    for (std::vector<MyGUI::Button*>::iterator it = this->mButtons.begin(); it != this->mButtons.end(); ++it)
    {
        MyGUI::Gui::getInstance().destroyWidget(*it);
    }
    this->mButtons.clear();
    mCurrentButton = -1;

    // TODO: The buttons should be generated from a template in the layout file, ie. cloning an existing widget
    MyGUI::Button* button;
    MyGUI::IntCoord coord = MyGUI::IntCoord(0, 0, mButtonBar->getWidth(), 10);
    ButtonList::const_iterator end = buttons.end();
    for (ButtonList::const_iterator it = buttons.begin(); it != end; ++it)
    {
        const std::string &text = *it;
        button = mButtonBar->createWidget<MyGUI::Button>("MW_Button", coord, MyGUI::Align::Top | MyGUI::Align::HCenter, "");
        button->getSubWidgetText()->setWordWrap(true);
        button->setCaption(text);
        fitToText(button);
        button->eventMouseButtonClick += MyGUI::newDelegate(this, &InfoBoxDialog::onButtonClicked);
        coord.top += button->getHeight();
        this->mButtons.push_back(button);
    }
}

void InfoBoxDialog::open()
{
    WindowModal::open();
    // Fix layout
    layoutVertically(mTextBox, 4);
    layoutVertically(mButtonBar, 6);
    layoutVertically(mMainWidget, 4 + 6);

    center();
}

int InfoBoxDialog::getChosenButton() const
{
    return mCurrentButton;
}

void InfoBoxDialog::onButtonClicked(MyGUI::Widget* _sender)
{
    std::vector<MyGUI::Button*>::const_iterator end = mButtons.end();
    int i = 0;
    for (std::vector<MyGUI::Button*>::const_iterator it = mButtons.begin(); it != end; ++it)
    {
        if (*it == _sender)
        {
            mCurrentButton = i;
            eventButtonSelected(i);
            return;
        }
        ++i;
    }
}

/* ClassChoiceDialog */

ClassChoiceDialog::ClassChoiceDialog(MWBase::WindowManager& parWindowManager)
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

CreateClassDialog::CreateClassDialog(MWBase::WindowManager& parWindowManager)
  : WindowModal("openmw_chargen_create_class.layout", parWindowManager)
  , mSpecDialog(NULL)
  , mAttribDialog(NULL)
  , mSkillDialog(NULL)
  , mDescDialog(NULL)
{
    // Centre dialog
    center();

    setText("SpecializationT", mWindowManager.getGameSettingString("sChooseClassMenu1", "Specialization"));
    getWidget(mSpecializationName, "SpecializationName");
    mSpecializationName->eventMouseButtonClick += MyGUI::newDelegate(this, &CreateClassDialog::onSpecializationClicked);

    setText("FavoriteAttributesT", mWindowManager.getGameSettingString("sChooseClassMenu2", "Favorite Attributes:"));
    getWidget(mFavoriteAttribute0, "FavoriteAttribute0");
    getWidget(mFavoriteAttribute1, "FavoriteAttribute1");
    mFavoriteAttribute0->setWindowManager(&mWindowManager);
    mFavoriteAttribute1->setWindowManager(&mWindowManager);
    mFavoriteAttribute0->eventClicked += MyGUI::newDelegate(this, &CreateClassDialog::onAttributeClicked);
    mFavoriteAttribute1->eventClicked += MyGUI::newDelegate(this, &CreateClassDialog::onAttributeClicked);

    setText("MajorSkillT", mWindowManager.getGameSettingString("sSkillClassMajor", ""));
    setText("MinorSkillT", mWindowManager.getGameSettingString("sSkillClassMinor", ""));
    for(int i = 0; i < 5; i++)
    {
        char theIndex = '0'+i;
        getWidget(mMajorSkill[i], std::string("MajorSkill").append(1, theIndex));
        getWidget(mMinorSkill[i], std::string("MinorSkill").append(1, theIndex));
        mSkills.push_back(mMajorSkill[i]);
        mSkills.push_back(mMinorSkill[i]);
    }

    std::vector<Widgets::MWSkillPtr>::const_iterator end = mSkills.end();
    for (std::vector<Widgets::MWSkillPtr>::const_iterator it = mSkills.begin(); it != end; ++it)
    {
        (*it)->setWindowManager(&mWindowManager);
        (*it)->eventClicked += MyGUI::newDelegate(this, &CreateClassDialog::onSkillClicked);
    }

    setText("LabelT", mWindowManager.getGameSettingString("sName", ""));
    getWidget(mEditName, "EditName");

    // Make sure the edit box has focus
    MyGUI::InputManager::getInstance().setKeyFocusWidget(mEditName);

    MyGUI::Button* descriptionButton;
    getWidget(descriptionButton, "DescriptionButton");
    descriptionButton->eventMouseButtonClick += MyGUI::newDelegate(this, &CreateClassDialog::onDescriptionClicked);

    MyGUI::Button* backButton;
    getWidget(backButton, "BackButton");
    backButton->eventMouseButtonClick += MyGUI::newDelegate(this, &CreateClassDialog::onBackClicked);

    MyGUI::Button* okButton;
    getWidget(okButton, "OKButton");
    okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &CreateClassDialog::onOkClicked);

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

CreateClassDialog::~CreateClassDialog()
{
    delete mSpecDialog;
    delete mAttribDialog;
    delete mSkillDialog;
    delete mDescDialog;
}

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
    return mEditName->getOnlyText();
}

std::string CreateClassDialog::getDescription() const
{
    return mDescription;
}

ESM::Class::Specialization CreateClassDialog::getSpecializationId() const
{
    return mSpecializationId;
}

std::vector<int> CreateClassDialog::getFavoriteAttributes() const
{
    std::vector<int> v;
    v.push_back(mFavoriteAttribute0->getAttributeId());
    v.push_back(mFavoriteAttribute1->getAttributeId());
    return v;
}

std::vector<ESM::Skill::SkillEnum> CreateClassDialog::getMajorSkills() const
{
    std::vector<ESM::Skill::SkillEnum> v;
    for(int i = 0; i < 5; i++)
    {
        v.push_back(mMajorSkill[i]->getSkillId());
    }
    return v;
}

std::vector<ESM::Skill::SkillEnum> CreateClassDialog::getMinorSkills() const
{
    std::vector<ESM::Skill::SkillEnum> v;
    for(int i=0; i < 5; i++)
    {
        v.push_back(mMinorSkill[i]->getSkillId());
    }
    return v;
}

void CreateClassDialog::setNextButtonShow(bool shown)
{
    MyGUI::Button* okButton;
    getWidget(okButton, "OKButton");

    if (shown)
        okButton->setCaption(mWindowManager.getGameSettingString("sNext", ""));
    else
        okButton->setCaption(mWindowManager.getGameSettingString("sOK", ""));
}

// widget controls

void CreateClassDialog::onDialogCancel()
{
    mWindowManager.removeDialog(mSpecDialog);
    mSpecDialog = 0;

    mWindowManager.removeDialog(mAttribDialog);
    mAttribDialog = 0;

    mWindowManager.removeDialog(mSkillDialog);
    mSkillDialog = 0;

    mWindowManager.removeDialog(mDescDialog);
    mDescDialog = 0;
}

void CreateClassDialog::onSpecializationClicked(MyGUI::Widget* _sender)
{
    delete mSpecDialog;
    mSpecDialog = new SelectSpecializationDialog(mWindowManager);
    mSpecDialog->eventCancel += MyGUI::newDelegate(this, &CreateClassDialog::onDialogCancel);
    mSpecDialog->eventItemSelected += MyGUI::newDelegate(this, &CreateClassDialog::onSpecializationSelected);
    mSpecDialog->setVisible(true);
}

void CreateClassDialog::onSpecializationSelected()
{
    mSpecializationId = mSpecDialog->getSpecializationId();
    setSpecialization(mSpecializationId);

    mWindowManager.removeDialog(mSpecDialog);
    mSpecDialog = 0;
}

void CreateClassDialog::setSpecialization(int id)
{
    mSpecializationId = (ESM::Class::Specialization) id;
    static const char *specIds[3] = {
        "sSpecializationCombat",
        "sSpecializationMagic",
        "sSpecializationStealth"
    };
    std::string specName = mWindowManager.getGameSettingString(specIds[mSpecializationId], specIds[mSpecializationId]);
    mSpecializationName->setCaption(specName);
    ToolTips::createSpecializationToolTip(mSpecializationName, specName, mSpecializationId);
}

void CreateClassDialog::onAttributeClicked(Widgets::MWAttributePtr _sender)
{
    delete mAttribDialog;
    mAttribDialog = new SelectAttributeDialog(mWindowManager);
    mAffectedAttribute = _sender;
    mAttribDialog->eventCancel += MyGUI::newDelegate(this, &CreateClassDialog::onDialogCancel);
    mAttribDialog->eventItemSelected += MyGUI::newDelegate(this, &CreateClassDialog::onAttributeSelected);
    mAttribDialog->setVisible(true);
}

void CreateClassDialog::onAttributeSelected()
{
    ESM::Attribute::AttributeID id = mAttribDialog->getAttributeId();
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
    mWindowManager.removeDialog(mAttribDialog);
    mAttribDialog = 0;

    update();
}

void CreateClassDialog::onSkillClicked(Widgets::MWSkillPtr _sender)
{
    delete mSkillDialog;
    mSkillDialog = new SelectSkillDialog(mWindowManager);
    mAffectedSkill = _sender;
    mSkillDialog->eventCancel += MyGUI::newDelegate(this, &CreateClassDialog::onDialogCancel);
    mSkillDialog->eventItemSelected += MyGUI::newDelegate(this, &CreateClassDialog::onSkillSelected);
    mSkillDialog->setVisible(true);
}

void CreateClassDialog::onSkillSelected()
{
    ESM::Skill::SkillEnum id = mSkillDialog->getSkillId();

    // Avoid duplicate skills by swapping any skill field that matches the selected one
    std::vector<Widgets::MWSkillPtr>::const_iterator end = mSkills.end();
    for (std::vector<Widgets::MWSkillPtr>::const_iterator it = mSkills.begin(); it != end; ++it)
    {
        if (*it == mAffectedSkill)
            continue;
        if ((*it)->getSkillId() == id)
        {
            (*it)->setSkillId(mAffectedSkill->getSkillId());
            break;
        }
    }

    mAffectedSkill->setSkillId(mSkillDialog->getSkillId());
    mWindowManager.removeDialog(mSkillDialog);
    mSkillDialog = 0;
    update();
}

void CreateClassDialog::onDescriptionClicked(MyGUI::Widget* _sender)
{
    mDescDialog = new DescriptionDialog(mWindowManager);
    mDescDialog->setTextInput(mDescription);
    mDescDialog->eventDone += MyGUI::newDelegate(this, &CreateClassDialog::onDescriptionEntered);
    mDescDialog->setVisible(true);
}

void CreateClassDialog::onDescriptionEntered(WindowBase* parWindow)
{
    mDescription = mDescDialog->getTextInput();
    mWindowManager.removeDialog(mDescDialog);
    mDescDialog = 0;
}

void CreateClassDialog::onOkClicked(MyGUI::Widget* _sender)
{
    if(getName().size() <= 0)
        return;
    eventDone(this);
}

void CreateClassDialog::onBackClicked(MyGUI::Widget* _sender)
{
    eventBack();
}

/* SelectSpecializationDialog */

SelectSpecializationDialog::SelectSpecializationDialog(MWBase::WindowManager& parWindowManager)
  : WindowModal("openmw_chargen_select_specialization.layout", parWindowManager)
{
    // Centre dialog
    center();

    setText("LabelT", mWindowManager.getGameSettingString("sSpecializationMenu1", ""));

    getWidget(mSpecialization0, "Specialization0");
    getWidget(mSpecialization1, "Specialization1");
    getWidget(mSpecialization2, "Specialization2");
    std::string combat = mWindowManager.getGameSettingString(ESM::Class::sGmstSpecializationIds[ESM::Class::Combat], "");
    std::string magic = mWindowManager.getGameSettingString(ESM::Class::sGmstSpecializationIds[ESM::Class::Magic], "");
    std::string stealth = mWindowManager.getGameSettingString(ESM::Class::sGmstSpecializationIds[ESM::Class::Stealth], "");

    mSpecialization0->setCaption(combat);
    mSpecialization0->eventMouseButtonClick += MyGUI::newDelegate(this, &SelectSpecializationDialog::onSpecializationClicked);
    mSpecialization1->setCaption(magic);
    mSpecialization1->eventMouseButtonClick += MyGUI::newDelegate(this, &SelectSpecializationDialog::onSpecializationClicked);
    mSpecialization2->setCaption(stealth);
    mSpecialization2->eventMouseButtonClick += MyGUI::newDelegate(this, &SelectSpecializationDialog::onSpecializationClicked);
    mSpecializationId = ESM::Class::Combat;

    ToolTips::createSpecializationToolTip(mSpecialization0, combat, ESM::Class::Combat);
    ToolTips::createSpecializationToolTip(mSpecialization1, magic, ESM::Class::Magic);
    ToolTips::createSpecializationToolTip(mSpecialization2, stealth, ESM::Class::Stealth);

    MyGUI::Button* cancelButton;
    getWidget(cancelButton, "CancelButton");
    cancelButton->setCaption(mWindowManager.getGameSettingString("sCancel", ""));
    cancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SelectSpecializationDialog::onCancelClicked);
}

SelectSpecializationDialog::~SelectSpecializationDialog()
{
}

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
    eventCancel();
}

/* SelectAttributeDialog */

SelectAttributeDialog::SelectAttributeDialog(MWBase::WindowManager& parWindowManager)
  : WindowModal("openmw_chargen_select_attribute.layout", parWindowManager)
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
        attribute->setAttributeId(ESM::Attribute::sAttributeIds[i]);
        attribute->eventClicked += MyGUI::newDelegate(this, &SelectAttributeDialog::onAttributeClicked);
        ToolTips::createAttributeToolTip(attribute, attribute->getAttributeId());
    }

    MyGUI::Button* cancelButton;
    getWidget(cancelButton, "CancelButton");
    cancelButton->setCaption(mWindowManager.getGameSettingString("sCancel", ""));
    cancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SelectAttributeDialog::onCancelClicked);
}

SelectAttributeDialog::~SelectAttributeDialog()
{
}

// widget controls

void SelectAttributeDialog::onAttributeClicked(Widgets::MWAttributePtr _sender)
{
    // TODO: Change MWAttribute to set and get AttributeID enum instead of int
    mAttributeId = static_cast<ESM::Attribute::AttributeID>(_sender->getAttributeId());
    eventItemSelected();
}

void SelectAttributeDialog::onCancelClicked(MyGUI::Widget* _sender)
{
    eventCancel();
}


/* SelectSkillDialog */

SelectSkillDialog::SelectSkillDialog(MWBase::WindowManager& parWindowManager)
  : WindowModal("openmw_chargen_select_skill.layout", parWindowManager)
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
        getWidget(mCombatSkill[i],  std::string("CombatSkill").append(1, theIndex));
        getWidget(mMagicSkill[i],   std::string("MagicSkill").append(1, theIndex));
        getWidget(mStealthSkill[i], std::string("StealthSkill").append(1, theIndex));
    }

    struct {Widgets::MWSkillPtr widget; ESM::Skill::SkillEnum skillId;} mSkills[3][9] = {
        {
            {mCombatSkill[0], ESM::Skill::Block},
            {mCombatSkill[1], ESM::Skill::Armorer},
            {mCombatSkill[2], ESM::Skill::MediumArmor},
            {mCombatSkill[3], ESM::Skill::HeavyArmor},
            {mCombatSkill[4], ESM::Skill::BluntWeapon},
            {mCombatSkill[5], ESM::Skill::LongBlade},
            {mCombatSkill[6], ESM::Skill::Axe},
            {mCombatSkill[7], ESM::Skill::Spear},
            {mCombatSkill[8], ESM::Skill::Athletics}
        },
        {
            {mMagicSkill[0], ESM::Skill::Enchant},
            {mMagicSkill[1], ESM::Skill::Destruction},
            {mMagicSkill[2], ESM::Skill::Alteration},
            {mMagicSkill[3], ESM::Skill::Illusion},
            {mMagicSkill[4], ESM::Skill::Conjuration},
            {mMagicSkill[5], ESM::Skill::Mysticism},
            {mMagicSkill[6], ESM::Skill::Restoration},
            {mMagicSkill[7], ESM::Skill::Alchemy},
            {mMagicSkill[8], ESM::Skill::Unarmored}
        },
        {
            {mStealthSkill[0], ESM::Skill::Security},
            {mStealthSkill[1], ESM::Skill::Sneak},
            {mStealthSkill[2], ESM::Skill::Acrobatics},
            {mStealthSkill[3], ESM::Skill::LightArmor},
            {mStealthSkill[4], ESM::Skill::ShortBlade},
            {mStealthSkill[5] ,ESM::Skill::Marksman},
            {mStealthSkill[6] ,ESM::Skill::Mercantile},
            {mStealthSkill[7] ,ESM::Skill::Speechcraft},
            {mStealthSkill[8] ,ESM::Skill::HandToHand}
        }
    };

    for (int spec = 0; spec < 3; ++spec)
    {
        for (int i = 0; i < 9; ++i)
        {
            mSkills[spec][i].widget->setWindowManager(&mWindowManager);
            mSkills[spec][i].widget->setSkillId(mSkills[spec][i].skillId);
            mSkills[spec][i].widget->eventClicked += MyGUI::newDelegate(this, &SelectSkillDialog::onSkillClicked);
            ToolTips::createSkillToolTip(mSkills[spec][i].widget, mSkills[spec][i].widget->getSkillId());
        }
    }

    MyGUI::Button* cancelButton;
    getWidget(cancelButton, "CancelButton");
    cancelButton->setCaption(mWindowManager.getGameSettingString("sCancel", ""));
    cancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SelectSkillDialog::onCancelClicked);
}

SelectSkillDialog::~SelectSkillDialog()
{
}

// widget controls

void SelectSkillDialog::onSkillClicked(Widgets::MWSkillPtr _sender)
{
    mSkillId = _sender->getSkillId();
    eventItemSelected();
}

void SelectSkillDialog::onCancelClicked(MyGUI::Widget* _sender)
{
    eventCancel();
}

/* DescriptionDialog */

DescriptionDialog::DescriptionDialog(MWBase::WindowManager& parWindowManager)
  : WindowModal("openmw_chargen_class_description.layout", parWindowManager)
{
    // Centre dialog
    center();

    getWidget(mTextEdit, "TextEdit");

    MyGUI::Button* okButton;
    getWidget(okButton, "OKButton");
    okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &DescriptionDialog::onOkClicked);
    okButton->setCaption(mWindowManager.getGameSettingString("sInputMenu1", ""));

    // Make sure the edit box has focus
    MyGUI::InputManager::getInstance().setKeyFocusWidget(mTextEdit);
}

DescriptionDialog::~DescriptionDialog()
{
}

// widget controls

void DescriptionDialog::onOkClicked(MyGUI::Widget* _sender)
{
    eventDone(this);
}
