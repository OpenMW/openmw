#include "race.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "tooltips.hpp"

namespace
{
    int wrap(int index, int max)
    {
        if (index < 0)
            return max - 1;
        else if (index >= max)
            return 0;
        else
            return index;
    }
}

namespace MWGui
{

    RaceDialog::RaceDialog()
      : WindowModal("openmw_chargen_race.layout")
      , mGenderIndex(0)
      , mFaceIndex(0)
      , mHairIndex(0)
      , mCurrentAngle(0)
      , mPreviewDirty(true)
      , mPreview(NULL)
    {
        // Centre dialog
        center();

        setText("AppearanceT", MWBase::Environment::get().getWindowManager()->getGameSettingString("sRaceMenu1", "Appearance"));
        getWidget(mPreviewImage, "PreviewImage");

        getWidget(mHeadRotate, "HeadRotate");

        // Mouse wheel step is hardcoded to 50 in MyGUI 3.2 ("FIXME").
        // Give other steps the same value to accomodate.
        mHeadRotate->setScrollRange(1000);
        mHeadRotate->setScrollPosition(500);
        mHeadRotate->setScrollViewPage(50);
        mHeadRotate->setScrollPage(50);
        mHeadRotate->eventScrollChangePosition += MyGUI::newDelegate(this, &RaceDialog::onHeadRotate);

        // Set up next/previous buttons
        MyGUI::Button *prevButton, *nextButton;

        setText("GenderChoiceT", MWBase::Environment::get().getWindowManager()->getGameSettingString("sRaceMenu2", "Change Sex"));
        getWidget(prevButton, "PrevGenderButton");
        getWidget(nextButton, "NextGenderButton");
        prevButton->eventMouseButtonClick += MyGUI::newDelegate(this, &RaceDialog::onSelectPreviousGender);
        nextButton->eventMouseButtonClick += MyGUI::newDelegate(this, &RaceDialog::onSelectNextGender);

        setText("FaceChoiceT", MWBase::Environment::get().getWindowManager()->getGameSettingString("sRaceMenu3", "Change Face"));
        getWidget(prevButton, "PrevFaceButton");
        getWidget(nextButton, "NextFaceButton");
        prevButton->eventMouseButtonClick += MyGUI::newDelegate(this, &RaceDialog::onSelectPreviousFace);
        nextButton->eventMouseButtonClick += MyGUI::newDelegate(this, &RaceDialog::onSelectNextFace);

        setText("HairChoiceT", MWBase::Environment::get().getWindowManager()->getGameSettingString("sRaceMenu4", "Change Hair"));
        getWidget(prevButton, "PrevHairButton");
        getWidget(nextButton, "NextHairButton");
        prevButton->eventMouseButtonClick += MyGUI::newDelegate(this, &RaceDialog::onSelectPreviousHair);
        nextButton->eventMouseButtonClick += MyGUI::newDelegate(this, &RaceDialog::onSelectNextHair);

        setText("RaceT", MWBase::Environment::get().getWindowManager()->getGameSettingString("sRaceMenu5", "Race"));
        getWidget(mRaceList, "RaceList");
        mRaceList->setScrollVisible(true);
        mRaceList->eventListSelectAccept += MyGUI::newDelegate(this, &RaceDialog::onAccept);
        mRaceList->eventListChangePosition += MyGUI::newDelegate(this, &RaceDialog::onSelectRace);

        setText("SkillsT", MWBase::Environment::get().getWindowManager()->getGameSettingString("sBonusSkillTitle", "Skill Bonus"));
        getWidget(mSkillList, "SkillList");
        setText("SpellPowerT", MWBase::Environment::get().getWindowManager()->getGameSettingString("sRaceMenu7", "Specials"));
        getWidget(mSpellPowerList, "SpellPowerList");

        MyGUI::Button* backButton;
        getWidget(backButton, "BackButton");
        backButton->eventMouseButtonClick += MyGUI::newDelegate(this, &RaceDialog::onBackClicked);

        MyGUI::Button* okButton;
        getWidget(okButton, "OKButton");
        okButton->setCaption(MWBase::Environment::get().getWindowManager()->getGameSettingString("sOK", ""));
        okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &RaceDialog::onOkClicked);

        updateRaces();
        updateSkills();
        updateSpellPowers();
    }

    void RaceDialog::setNextButtonShow(bool shown)
    {
        MyGUI::Button* okButton;
        getWidget(okButton, "OKButton");

        if (shown)
            okButton->setCaption(MWBase::Environment::get().getWindowManager()->getGameSettingString("sNext", ""));
        else
            okButton->setCaption(MWBase::Environment::get().getWindowManager()->getGameSettingString("sOK", ""));
    }

    void RaceDialog::open()
    {
        WindowModal::open();

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

        mPreviewDirty = true;
    }


    void RaceDialog::setRaceId(const std::string &raceId)
    {
        mCurrentRaceId = raceId;
        mRaceList->setIndexSelected(MyGUI::ITEM_NONE);
        size_t count = mRaceList->getItemCount();
        for (size_t i = 0; i < count; ++i)
        {
            if (Misc::StringUtils::ciEqual(*mRaceList->getItemDataAt<std::string>(i), raceId))
            {
                mRaceList->setIndexSelected(i);
                MyGUI::Button* okButton;
                getWidget(okButton, "OKButton");
                break;
            }
        }

        updateSkills();
        updateSpellPowers();
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

    void RaceDialog::onHeadRotate(MyGUI::ScrollBar* scroll, size_t _position)
    {
        float angle = (float(_position) / (scroll->getScrollRange()-1) - 0.5) * 3.14 * 2;
        float diff = angle - mCurrentAngle;
        mPreview->update (diff);
        mPreviewDirty = true;
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
        mFaceIndex = wrap(mFaceIndex - 1, mAvailableHeads.size());
        updatePreview();
    }

    void RaceDialog::onSelectNextFace(MyGUI::Widget*)
    {
        mFaceIndex = wrap(mFaceIndex + 1, mAvailableHeads.size());
        updatePreview();
    }

    void RaceDialog::onSelectPreviousHair(MyGUI::Widget*)
    {
        mHairIndex = wrap(mHairIndex - 1, mAvailableHairs.size());
        updatePreview();
    }

    void RaceDialog::onSelectNextHair(MyGUI::Widget*)
    {
        mHairIndex = wrap(mHairIndex + 1, mAvailableHairs.size());
        updatePreview();
    }

    void RaceDialog::onSelectRace(MyGUI::ListBox* _sender, size_t _index)
    {
        if (_index == MyGUI::ITEM_NONE)
            return;

        MyGUI::Button* okButton;
        getWidget(okButton, "OKButton");
        const std::string *raceId = mRaceList->getItemDataAt<std::string>(_index);
        if (Misc::StringUtils::ciEqual(mCurrentRaceId, *raceId))
            return;

        mCurrentRaceId = *raceId;

        recountParts();

        updatePreview();
        updateSkills();
        updateSpellPowers();
    }

    void RaceDialog::onAccept(MyGUI::ListBox *_sender, size_t _index)
    {
        onSelectRace(_sender, _index);
        if(mRaceList->getIndexSelected() == MyGUI::ITEM_NONE)
            return;
        eventDone(this);
    }

    void RaceDialog::getBodyParts (int part, std::vector<std::string>& out)
    {
        out.clear();
        const MWWorld::Store<ESM::BodyPart> &store =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::BodyPart>();

        for (MWWorld::Store<ESM::BodyPart>::iterator it = store.begin(); it != store.end(); ++it)
        {
            const ESM::BodyPart& bodypart = *it;
            if (bodypart.mData.mFlags & ESM::BodyPart::BPF_NotPlayable)
                continue;
            if (bodypart.mData.mType != ESM::BodyPart::MT_Skin)
                continue;
            if (bodypart.mData.mPart != static_cast<ESM::BodyPart::MeshPart>(part))
                continue;
            if (mGenderIndex != (bodypart.mData.mFlags & ESM::BodyPart::BPF_Female))
                continue;
            bool firstPerson = (bodypart.mId.size() >= 3)
                    && bodypart.mId[bodypart.mId.size()-3] == '1'
                    && bodypart.mId[bodypart.mId.size()-2] == 's'
                    && bodypart.mId[bodypart.mId.size()-1] == 't';
            if (firstPerson)
                continue;
            if (Misc::StringUtils::ciEqual(bodypart.mRace, mCurrentRaceId))
                out.push_back(bodypart.mId);
        }
    }

    void RaceDialog::recountParts()
    {
        getBodyParts(ESM::BodyPart::MP_Hair, mAvailableHairs);
        getBodyParts(ESM::BodyPart::MP_Head, mAvailableHeads);

        mFaceIndex = 0;
        mHairIndex = 0;
    }

    // update widget content

    void RaceDialog::updatePreview()
    {
        ESM::NPC record = mPreview->getPrototype();
        record.mRace = mCurrentRaceId;
        record.setIsMale(mGenderIndex == 0);

        record.mHead = mAvailableHeads[mFaceIndex];
        record.mHair = mAvailableHairs[mHairIndex];

        mPreview->setPrototype(record);
        mPreviewDirty = true;
    }

    void RaceDialog::doRenderUpdate()
    {
        if (mPreviewDirty)
        {
            mPreview->render();
            mPreviewDirty = false;
        }
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
            if (Misc::StringUtils::ciEqual(it->mId, mCurrentRaceId))
                mRaceList->setIndexSelected(index);
            ++index;
        }
    }

    void RaceDialog::updateSkills()
    {
        for (std::vector<MyGUI::Widget*>::iterator it = mSkillItems.begin(); it != mSkillItems.end(); ++it)
        {
            MyGUI::Gui::getInstance().destroyWidget(*it);
        }
        mSkillItems.clear();

        if (mCurrentRaceId.empty())
            return;

        Widgets::MWSkillPtr skillWidget;
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

            skillWidget = mSkillList->createWidget<Widgets::MWSkill>("MW_StatNameValue", coord1, MyGUI::Align::Default,
                                                           std::string("Skill") + boost::lexical_cast<std::string>(i));
            skillWidget->setSkillNumber(skillId);
            skillWidget->setSkillValue(Widgets::MWSkill::SkillValue(race->mData.mBonus[i].mBonus));
            ToolTips::createSkillToolTip(skillWidget, skillId);


            mSkillItems.push_back(skillWidget);

            coord1.top += lineHeight;
        }
    }

    void RaceDialog::updateSpellPowers()
    {
        for (std::vector<MyGUI::Widget*>::iterator it = mSpellPowerItems.begin(); it != mSpellPowerItems.end(); ++it)
        {
            MyGUI::Gui::getInstance().destroyWidget(*it);
        }
        mSpellPowerItems.clear();

        if (mCurrentRaceId.empty())
            return;

        Widgets::MWSpellPtr spellPowerWidget;
        const int lineHeight = 18;
        MyGUI::IntCoord coord(0, 0, mSpellPowerList->getWidth(), 18);

        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
        const ESM::Race *race = store.get<ESM::Race>().find(mCurrentRaceId);

        std::vector<std::string>::const_iterator it = race->mPowers.mList.begin();
        std::vector<std::string>::const_iterator end = race->mPowers.mList.end();
        for (int i = 0; it != end; ++it)
        {
            const std::string &spellpower = *it;
            spellPowerWidget = mSpellPowerList->createWidget<Widgets::MWSpell>("MW_StatName", coord, MyGUI::Align::Default, std::string("SpellPower") + boost::lexical_cast<std::string>(i));
            spellPowerWidget->setSpellId(spellpower);
            spellPowerWidget->setUserString("ToolTipType", "Spell");
            spellPowerWidget->setUserString("Spell", spellpower);

            mSpellPowerItems.push_back(spellPowerWidget);

            coord.top += lineHeight;
            ++i;
        }
    }
}
