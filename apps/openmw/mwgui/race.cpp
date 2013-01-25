#include "race.hpp"

#include <iostream>
#include <iterator>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "widgets.hpp"
#include "tooltips.hpp"

using namespace MWGui;
using namespace Widgets;

RaceDialog::RaceDialog(MWBase::WindowManager& parWindowManager)
  : WindowBase("openmw_chargen_race.layout", parWindowManager)
  , mGenderIndex(0)
  , mFaceIndex(0)
  , mHairIndex(0)
  , mFaceCount(10)
  , mHairCount(14)
  , mCurrentAngle(0)
{
    // Centre dialog
    center();

    setText("AppearanceT", mWindowManager.getGameSettingString("sRaceMenu1", "Appearance"));
    getWidget(mPreviewImage, "PreviewImage");

    getWidget(mHeadRotate, "HeadRotate");
    mHeadRotate->setScrollRange(50);
    mHeadRotate->setScrollPosition(25);
    mHeadRotate->setScrollViewPage(10);
    mHeadRotate->eventScrollChangePosition += MyGUI::newDelegate(this, &RaceDialog::onHeadRotate);

    // Set up next/previous buttons
    MyGUI::ButtonPtr prevButton, nextButton;

    setText("GenderChoiceT", mWindowManager.getGameSettingString("sRaceMenu2", "Change Sex"));
    getWidget(prevButton, "PrevGenderButton");
    getWidget(nextButton, "NextGenderButton");
    prevButton->eventMouseButtonClick += MyGUI::newDelegate(this, &RaceDialog::onSelectPreviousGender);
    nextButton->eventMouseButtonClick += MyGUI::newDelegate(this, &RaceDialog::onSelectNextGender);

    setText("FaceChoiceT", mWindowManager.getGameSettingString("sRaceMenu3", "Change Face"));
    getWidget(prevButton, "PrevFaceButton");
    getWidget(nextButton, "NextFaceButton");
    prevButton->eventMouseButtonClick += MyGUI::newDelegate(this, &RaceDialog::onSelectPreviousFace);
    nextButton->eventMouseButtonClick += MyGUI::newDelegate(this, &RaceDialog::onSelectNextFace);

    setText("HairChoiceT", mWindowManager.getGameSettingString("sRaceMenu4", "Change Hair"));
    getWidget(prevButton, "PrevHairButton");
    getWidget(nextButton, "NextHairButton");
    prevButton->eventMouseButtonClick += MyGUI::newDelegate(this, &RaceDialog::onSelectPreviousHair);
    nextButton->eventMouseButtonClick += MyGUI::newDelegate(this, &RaceDialog::onSelectNextHair);

    setText("RaceT", mWindowManager.getGameSettingString("sRaceMenu4", "Race"));
    getWidget(mRaceList, "RaceList");
    mRaceList->setScrollVisible(true);
    mRaceList->eventListSelectAccept += MyGUI::newDelegate(this, &RaceDialog::onSelectRace);
    mRaceList->eventListMouseItemActivate += MyGUI::newDelegate(this, &RaceDialog::onSelectRace);
    mRaceList->eventListChangePosition += MyGUI::newDelegate(this, &RaceDialog::onSelectRace);

    setText("SkillsT", mWindowManager.getGameSettingString("sBonusSkillTitle", "Skill Bonus"));
    getWidget(mSkillList, "SkillList");
    setText("SpellPowerT", mWindowManager.getGameSettingString("sRaceMenu7", "Specials"));
    getWidget(mSpellPowerList, "SpellPowerList");

    MyGUI::ButtonPtr backButton;
    getWidget(backButton, "BackButton");
    backButton->eventMouseButtonClick += MyGUI::newDelegate(this, &RaceDialog::onBackClicked);

    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");
    okButton->setCaption(mWindowManager.getGameSettingString("sOK", ""));
    okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &RaceDialog::onOkClicked);
    okButton->setEnabled(false);

    updateRaces();
    updateSkills();
    updateSpellPowers();
}

void RaceDialog::setNextButtonShow(bool shown)
{
    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");

    if (shown)
        okButton->setCaption(mWindowManager.getGameSettingString("sNext", ""));
    else
        okButton->setCaption(mWindowManager.getGameSettingString("sOK", ""));
}

void RaceDialog::open()
{
    updateRaces();
    updateSkills();
    updateSpellPowers();

    mPreview = new MWRender::RaceSelectionPreview();
    mPreview->setup();
    mPreview->update (0);

    const ESM::NPC proto = mPreview->getPrototype();
    setRaceId(proto.mRace);
    recountParts();
    
    std::string index = proto.mHead.substr(proto.mHead.size() - 2, 2);
    mFaceIndex = boost::lexical_cast<int>(index) - 1;

    index = proto.mHair.substr(proto.mHair.size() - 2, 2);
    mHairIndex = boost::lexical_cast<int>(index) - 1;

    mPreviewImage->setImageTexture ("CharacterHeadPreview");
}


void RaceDialog::setRaceId(const std::string &raceId)
{
    mCurrentRaceId = raceId;
    mRaceList->setIndexSelected(MyGUI::ITEM_NONE);
    size_t count = mRaceList->getItemCount();
    for (size_t i = 0; i < count; ++i)
    {
        if (boost::iequals(*mRaceList->getItemDataAt<std::string>(i), raceId))
        {
            mRaceList->setIndexSelected(i);
            MyGUI::ButtonPtr okButton;
            getWidget(okButton, "OKButton");
            okButton->setEnabled(true);
            break;
        }
    }

    updateSkills();
    updateSpellPowers();
}

int wrap(int index, int max)
{
    if (index < 0)
        return max - 1;
    else if (index >= max)
        return 0;
    else
        return index;
}

int countParts(const std::string &part, const std::string &race, bool male)
{
    const MWWorld::Store<ESM::BodyPart> &store =
        MWBase::Environment::get().getWorld()->getStore().get<ESM::BodyPart>();

    std::string prefix =
        "b_n_" + race + ((male) ? "_m_" : "_f_") + part;

    std::string suffix;
    suffix.reserve(prefix.size() + 3);
    
    int count = -1;
    do {
        ++count;
        suffix = "_" + (boost::format("%02d") % (count + 1)).str();
    }
    while (store.search(prefix + suffix) != 0);

    if (count == 0 && part == "hair") {
        count = -1;
        do {
            ++count;
            suffix = (boost::format("%02d") % (count + 1)).str();
        }
        while (store.search(prefix + suffix) != 0);
    }
    return count;
}

void RaceDialog::close()
{
    delete mPreview;
    mPreview = 0;
}

// widget controls

void RaceDialog::onOkClicked(MyGUI::Widget* _sender)
{
    if(mRaceList->getIndexSelected() == MyGUI::ITEM_NONE)
        return;
    eventDone(this);
}

void RaceDialog::onBackClicked(MyGUI::Widget* _sender)
{
    eventBack();
}

void RaceDialog::onHeadRotate(MyGUI::ScrollBar*, size_t _position)
{
    float angle = (float(_position) / 49.f - 0.5) * 3.14 * 2;
    float diff = angle - mCurrentAngle;
    mPreview->update (diff);
    mCurrentAngle += diff;
}

void RaceDialog::onSelectPreviousGender(MyGUI::Widget*)
{
    mGenderIndex = wrap(mGenderIndex - 1, 2);

    recountParts();
    updatePreview();
}

void RaceDialog::onSelectNextGender(MyGUI::Widget*)
{
    mGenderIndex = wrap(mGenderIndex + 1, 2);

    recountParts();
    updatePreview();
}

void RaceDialog::onSelectPreviousFace(MyGUI::Widget*)
{
    mFaceIndex = wrap(mFaceIndex - 1, mFaceCount);
    updatePreview();
}

void RaceDialog::onSelectNextFace(MyGUI::Widget*)
{
    mFaceIndex = wrap(mFaceIndex + 1, mFaceCount);
    updatePreview();
}

void RaceDialog::onSelectPreviousHair(MyGUI::Widget*)
{
    mHairIndex = wrap(mHairIndex - 1, mHairCount);
    updatePreview();
}

void RaceDialog::onSelectNextHair(MyGUI::Widget*)
{
    mHairIndex = wrap(mHairIndex + 1, mHairCount);
    updatePreview();
}

void RaceDialog::onSelectRace(MyGUI::ListBox* _sender, size_t _index)
{
    if (_index == MyGUI::ITEM_NONE)
        return;

    MyGUI::ButtonPtr okButton;
    getWidget(okButton, "OKButton");
    okButton->setEnabled(true);
    const std::string *raceId = mRaceList->getItemDataAt<std::string>(_index);
    if (boost::iequals(mCurrentRaceId, *raceId))
        return;

    mCurrentRaceId = *raceId;

    recountParts();

    updatePreview();
    updateSkills();
    updateSpellPowers();
}

void RaceDialog::recountParts()
{
    mFaceIndex = 0;
    mHairIndex = 0;

    mFaceCount = countParts("head", mCurrentRaceId, mGenderIndex == 0);
    mHairCount = countParts("hair", mCurrentRaceId, mGenderIndex == 0);
}

// update widget content

void RaceDialog::updatePreview()
{
    ESM::NPC record = mPreview->getPrototype();
    record.mRace = mCurrentRaceId;
    record.setIsMale(mGenderIndex == 0);

    std::string prefix =
        "b_n_" + mCurrentRaceId + ((record.isMale()) ? "_m_" : "_f_");

    std::string headIndex = (boost::format("%02d") % (mFaceIndex + 1)).str();
    std::string hairIndex = (boost::format("%02d") % (mHairIndex + 1)).str();

    record.mHead = prefix + "head_" + headIndex;
    record.mHair = prefix + "hair_" + hairIndex;

    const MWWorld::Store<ESM::BodyPart> &parts =
        MWBase::Environment::get().getWorld()->getStore().get<ESM::BodyPart>();

    if (parts.search(record.mHair) == 0) {
        record.mHair = prefix + "hair" + hairIndex;
    }
    mPreview->setPrototype(record);
}

void RaceDialog::updateRaces()
{
    mRaceList->removeAllItems();

    const MWWorld::Store<ESM::Race> &races =
        MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>();

    
    int index = 0;
    MWWorld::Store<ESM::Race>::iterator it = races.begin();
    for (; it != races.end(); ++it)
    {
        bool playable = it->mData.mFlags & ESM::Race::Playable;
        if (!playable) // Only display playable races
            continue;

        mRaceList->addItem(it->mName, it->mId);
        if (boost::iequals(it->mId, mCurrentRaceId))
            mRaceList->setIndexSelected(index);
        ++index;
    }
}

void RaceDialog::updateSkills()
{
    for (std::vector<MyGUI::WidgetPtr>::iterator it = mSkillItems.begin(); it != mSkillItems.end(); ++it)
    {
        MyGUI::Gui::getInstance().destroyWidget(*it);
    }
    mSkillItems.clear();

    if (mCurrentRaceId.empty())
        return;

    MWSkillPtr skillWidget;
    const int lineHeight = 18;
    MyGUI::IntCoord coord1(0, 0, mSkillList->getWidth(), 18);

    const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
    const ESM::Race *race = store.get<ESM::Race>().find(mCurrentRaceId);
    int count = sizeof(race->mData.mBonus)/sizeof(race->mData.mBonus[0]); // TODO: Find a portable macro for this ARRAYSIZE?
    for (int i = 0; i < count; ++i)
    {
        int skillId = race->mData.mBonus[i].mSkill;
        if (skillId < 0 || skillId > ESM::Skill::Length) // Skip unknown skill indexes
            continue;

        skillWidget = mSkillList->createWidget<MWSkill>("MW_StatNameValue", coord1, MyGUI::Align::Default,
                                                       std::string("Skill") + boost::lexical_cast<std::string>(i));
        skillWidget->setWindowManager(&mWindowManager);
        skillWidget->setSkillNumber(skillId);
        skillWidget->setSkillValue(MWSkill::SkillValue(race->mData.mBonus[i].mBonus));
        ToolTips::createSkillToolTip(skillWidget, skillId);


        mSkillItems.push_back(skillWidget);

        coord1.top += lineHeight;
    }
}

void RaceDialog::updateSpellPowers()
{
    for (std::vector<MyGUI::WidgetPtr>::iterator it = mSpellPowerItems.begin(); it != mSpellPowerItems.end(); ++it)
    {
        MyGUI::Gui::getInstance().destroyWidget(*it);
    }
    mSpellPowerItems.clear();

    if (mCurrentRaceId.empty())
        return;

    MWSpellPtr spellPowerWidget;
    const int lineHeight = 18;
    MyGUI::IntCoord coord(0, 0, mSpellPowerList->getWidth(), 18);

    const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
    const ESM::Race *race = store.get<ESM::Race>().find(mCurrentRaceId);

    std::vector<std::string>::const_iterator it = race->mPowers.mList.begin();
    std::vector<std::string>::const_iterator end = race->mPowers.mList.end();
    for (int i = 0; it != end; ++it)
    {
        const std::string &spellpower = *it;
        spellPowerWidget = mSpellPowerList->createWidget<MWSpell>("MW_StatName", coord, MyGUI::Align::Default, std::string("SpellPower") + boost::lexical_cast<std::string>(i));
        spellPowerWidget->setWindowManager(&mWindowManager);
        spellPowerWidget->setSpellId(spellpower);
        spellPowerWidget->setUserString("ToolTipType", "Spell");
        spellPowerWidget->setUserString("Spell", spellpower);

        mSpellPowerItems.push_back(spellPowerWidget);

        coord.top += lineHeight;
        ++i;
    }
}
