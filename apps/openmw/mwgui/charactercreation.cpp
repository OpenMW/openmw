#include "charactercreation.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/npcstats.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/fallback.hpp"
#include "../mwworld/esmstore.hpp"

#include "textinput.hpp"
#include "race.hpp"
#include "class.hpp"
#include "birth.hpp"
#include "review.hpp"
#include "inventorywindow.hpp"

namespace
{
    struct Step
    {
        const std::string mText;
        const std::string mButtons[3];
        const std::string mSound;
    };

    const ESM::Class::Specialization mSpecializations[3]={ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}; // The specialization for each answer
    Step sGenerateClassSteps(int number) {
        number++;
        const MWWorld::Fallback* fallback=MWBase::Environment::get().getWorld()->getFallback();
        Step step = {fallback->getFallbackString("Question_"+MyGUI::utility::toString(number)+"_Question"),
        {fallback->getFallbackString("Question_"+MyGUI::utility::toString(number)+"_AnswerOne"),
        fallback->getFallbackString("Question_"+MyGUI::utility::toString(number)+"_AnswerTwo"),
        fallback->getFallbackString("Question_"+MyGUI::utility::toString(number)+"_AnswerThree")},
        "vo\\misc\\chargen qa"+MyGUI::utility::toString(number)+".wav"
        };
        return step;
    }

    struct ClassPoint
    {
        const char *id;
        // Specialization points to match, in order: Stealth, Combat, Magic
        // Note: Order is taken from http://www.uesp.net/wiki/Morrowind:Class_Quiz
        unsigned int points[3];
    };

    void updatePlayerHealth()
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        MWMechanics::NpcStats& npcStats = player.getClass().getNpcStats(player);
        npcStats.updateHealth();
    }
}

namespace MWGui
{

    CharacterCreation::CharacterCreation()
        : mNameDialog(0)
        , mRaceDialog(0)
        , mClassChoiceDialog(0)
        , mGenerateClassQuestionDialog(0)
        , mGenerateClassResultDialog(0)
        , mPickClassDialog(0)
        , mCreateClassDialog(0)
        , mBirthSignDialog(0)
        , mReviewDialog(0)
        , mGenerateClassStep(0)
    {
        mCreationStage = CSE_NotStarted;
        mGenerateClassSpecializations[0] = 0;
        mGenerateClassSpecializations[1] = 0;
        mGenerateClassSpecializations[2] = 0;
    }

    void CharacterCreation::setValue (const std::string& id, const MWMechanics::AttributeValue& value)
    {
        if (mReviewDialog)
        {
           static const char *ids[] =
            {
                "AttribVal1", "AttribVal2", "AttribVal3", "AttribVal4", "AttribVal5",
                "AttribVal6", "AttribVal7", "AttribVal8",
                0
            };

            for (int i=0; ids[i]; ++i)
            {
                if (ids[i]==id)
                    mReviewDialog->setAttribute(ESM::Attribute::AttributeID(i), value);
            }
        }
    }

    void CharacterCreation::setValue (const std::string& id, const MWMechanics::DynamicStat<float>& value)
    {
        if (mReviewDialog)
        {
            if (id == "HBar")
            {
                mReviewDialog->setHealth (value);
            }
            else if (id == "MBar")
            {
                mReviewDialog->setMagicka (value);
            }
            else if (id == "FBar")
            {
                mReviewDialog->setFatigue (value);
            }
        }
    }

    void CharacterCreation::setValue(const ESM::Skill::SkillEnum parSkill, const MWMechanics::SkillValue& value)
    {
        if (mReviewDialog)
            mReviewDialog->setSkillValue(parSkill, value);
    }

    void CharacterCreation::configureSkills (const SkillList& major, const SkillList& minor)
    {
        if (mReviewDialog)
            mReviewDialog->configureSkills(major, minor);
    }

    void CharacterCreation::spawnDialog(const char id)
    {
        switch (id)
        {
            case GM_Name:
                MWBase::Environment::get().getWindowManager()->removeDialog(mNameDialog);
                mNameDialog = 0;
                mNameDialog = new TextInputDialog();
                mNameDialog->setTextLabel(MWBase::Environment::get().getWindowManager()->getGameSettingString("sName", "Name"));
                mNameDialog->setTextInput(mPlayerName);
                mNameDialog->setNextButtonShow(mCreationStage >= CSE_NameChosen);
                mNameDialog->eventDone += MyGUI::newDelegate(this, &CharacterCreation::onNameDialogDone);
                mNameDialog->setVisible(true);
                break;

            case GM_Race:
                MWBase::Environment::get().getWindowManager()->removeDialog(mRaceDialog);
                mRaceDialog = 0;
                mRaceDialog = new RaceDialog();
                mRaceDialog->setNextButtonShow(mCreationStage >= CSE_RaceChosen);
                mRaceDialog->setRaceId(mPlayerRaceId);
                mRaceDialog->eventDone += MyGUI::newDelegate(this, &CharacterCreation::onRaceDialogDone);
                mRaceDialog->eventBack += MyGUI::newDelegate(this, &CharacterCreation::onRaceDialogBack);
                mRaceDialog->setVisible(true);
                if (mCreationStage < CSE_NameChosen)
                    mCreationStage = CSE_NameChosen;
                break;

            case GM_Class:
                MWBase::Environment::get().getWindowManager()->removeDialog(mClassChoiceDialog);
                mClassChoiceDialog = 0;
                mClassChoiceDialog = new ClassChoiceDialog();
                mClassChoiceDialog->eventButtonSelected += MyGUI::newDelegate(this, &CharacterCreation::onClassChoice);
                mClassChoiceDialog->setVisible(true);
                if (mCreationStage < CSE_RaceChosen)
                    mCreationStage = CSE_RaceChosen;
                break;

            case GM_ClassPick:
                MWBase::Environment::get().getWindowManager()->removeDialog(mPickClassDialog);
                mPickClassDialog = 0;
                mPickClassDialog = new PickClassDialog();
                mPickClassDialog->setNextButtonShow(mCreationStage >= CSE_ClassChosen);
                mPickClassDialog->setClassId(mPlayerClass.mName);
                mPickClassDialog->eventDone += MyGUI::newDelegate(this, &CharacterCreation::onPickClassDialogDone);
                mPickClassDialog->eventBack += MyGUI::newDelegate(this, &CharacterCreation::onPickClassDialogBack);
                mPickClassDialog->setVisible(true);
                if (mCreationStage < CSE_RaceChosen)
                    mCreationStage = CSE_RaceChosen;
                break;

            case GM_Birth:
                MWBase::Environment::get().getWindowManager()->removeDialog(mBirthSignDialog);
                mBirthSignDialog = 0;
                mBirthSignDialog = new BirthDialog();
                mBirthSignDialog->setNextButtonShow(mCreationStage >= CSE_BirthSignChosen);
                mBirthSignDialog->setBirthId(mPlayerBirthSignId);
                mBirthSignDialog->eventDone += MyGUI::newDelegate(this, &CharacterCreation::onBirthSignDialogDone);
                mBirthSignDialog->eventBack += MyGUI::newDelegate(this, &CharacterCreation::onBirthSignDialogBack);
                mBirthSignDialog->setVisible(true);
                if (mCreationStage < CSE_ClassChosen)
                    mCreationStage = CSE_ClassChosen;
                break;

            case GM_ClassCreate:
                if (!mCreateClassDialog)
                {
                    mCreateClassDialog = new CreateClassDialog();
                    mCreateClassDialog->eventDone += MyGUI::newDelegate(this, &CharacterCreation::onCreateClassDialogDone);
                    mCreateClassDialog->eventBack += MyGUI::newDelegate(this, &CharacterCreation::onCreateClassDialogBack);
                }
                mCreateClassDialog->setNextButtonShow(mCreationStage >= CSE_ClassChosen);
                mCreateClassDialog->setVisible(true);
                if (mCreationStage < CSE_RaceChosen)
                    mCreationStage = CSE_RaceChosen;
                break;
            case GM_ClassGenerate:
                mGenerateClassStep = 0;
                mGenerateClass = "";
                mGenerateClassSpecializations[0] = 0;
                mGenerateClassSpecializations[1] = 0;
                mGenerateClassSpecializations[2] = 0;
                showClassQuestionDialog();
                if (mCreationStage < CSE_RaceChosen)
                    mCreationStage = CSE_RaceChosen;
                break;
            case GM_Review:
                MWBase::Environment::get().getWindowManager()->removeDialog(mReviewDialog);
                mReviewDialog = 0;
                mReviewDialog = new ReviewDialog();
                mReviewDialog->setPlayerName(mPlayerName);
                mReviewDialog->setRace(mPlayerRaceId);
                mReviewDialog->setClass(mPlayerClass);
                mReviewDialog->setBirthSign(mPlayerBirthSignId);

                {
                    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
                    const MWMechanics::CreatureStats& stats = player.getClass().getCreatureStats(player);

                    mReviewDialog->setHealth ( stats.getHealth()  );
                    mReviewDialog->setMagicka( stats.getMagicka() );
                    mReviewDialog->setFatigue( stats.getFatigue() );
                }

                {
                    std::map<int, MWMechanics::AttributeValue > attributes = MWBase::Environment::get().getWindowManager()->getPlayerAttributeValues();
                    for (std::map<int, MWMechanics::AttributeValue >::iterator it = attributes.begin();
                        it != attributes.end(); ++it)
                    {
                        mReviewDialog->setAttribute(static_cast<ESM::Attribute::AttributeID> (it->first), it->second);
                    }
                }

                {
                    std::map<int, MWMechanics::SkillValue > skills = MWBase::Environment::get().getWindowManager()->getPlayerSkillValues();
                    for (std::map<int, MWMechanics::SkillValue >::iterator it = skills.begin();
                        it != skills.end(); ++it)
                    {
                        mReviewDialog->setSkillValue(static_cast<ESM::Skill::SkillEnum> (it->first), it->second);
                    }
                    mReviewDialog->configureSkills(MWBase::Environment::get().getWindowManager()->getPlayerMajorSkills(), MWBase::Environment::get().getWindowManager()->getPlayerMinorSkills());
                }

                mReviewDialog->eventDone += MyGUI::newDelegate(this, &CharacterCreation::onReviewDialogDone);
                mReviewDialog->eventBack += MyGUI::newDelegate(this, &CharacterCreation::onReviewDialogBack);
                mReviewDialog->eventActivateDialog += MyGUI::newDelegate(this, &CharacterCreation::onReviewActivateDialog);
                mReviewDialog->setVisible(true);
                if (mCreationStage < CSE_BirthSignChosen)
                    mCreationStage = CSE_BirthSignChosen;
                break;
        }
    }

    void CharacterCreation::doRenderUpdate()
    {
        if (mRaceDialog)
            mRaceDialog->doRenderUpdate();
    }

    void CharacterCreation::onReviewDialogDone(WindowBase* parWindow)
    {
        MWBase::Environment::get().getWindowManager()->removeDialog(mReviewDialog);
        mReviewDialog = 0;

        MWBase::Environment::get().getWindowManager()->popGuiMode();
    }

    void CharacterCreation::onReviewDialogBack()
    {
        MWBase::Environment::get().getWindowManager()->removeDialog(mReviewDialog);
        mReviewDialog = 0;
        mCreationStage = CSE_ReviewBack;

        MWBase::Environment::get().getWindowManager()->popGuiMode();
        MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Birth);
    }

    void CharacterCreation::onReviewActivateDialog(int parDialog)
    {
        MWBase::Environment::get().getWindowManager()->removeDialog(mReviewDialog);
        mReviewDialog = 0;
        mCreationStage = CSE_ReviewNext;

        MWBase::Environment::get().getWindowManager()->popGuiMode();

        switch(parDialog)
        {
            case ReviewDialog::NAME_DIALOG:
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Name);
                break;
            case ReviewDialog::RACE_DIALOG:
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Race);
                break;
            case ReviewDialog::CLASS_DIALOG:
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Class);
                break;
            case ReviewDialog::BIRTHSIGN_DIALOG:
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Birth);
        };
    }

    void CharacterCreation::onPickClassDialogDone(WindowBase* parWindow)
    {
        if (mPickClassDialog)
        {
            const std::string &classId = mPickClassDialog->getClassId();
            if (!classId.empty())
                MWBase::Environment::get().getMechanicsManager()->setPlayerClass(classId);

            const ESM::Class *klass =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Class>().find(classId);
            if (klass)
            {
                mPlayerClass = *klass;
                MWBase::Environment::get().getWindowManager()->setPlayerClass(mPlayerClass);
            }
            MWBase::Environment::get().getWindowManager()->removeDialog(mPickClassDialog);
            mPickClassDialog = 0;
        }

        updatePlayerHealth();

        handleDialogDone(CSE_ClassChosen, GM_Birth);
    }

    void CharacterCreation::onPickClassDialogBack()
    {
        if (mPickClassDialog)
        {
            const std::string classId = mPickClassDialog->getClassId();
            if (!classId.empty())
                MWBase::Environment::get().getMechanicsManager()->setPlayerClass(classId);
            MWBase::Environment::get().getWindowManager()->removeDialog(mPickClassDialog);
            mPickClassDialog = 0;
        }

        MWBase::Environment::get().getWindowManager()->popGuiMode();
        MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Class);
    }

    void CharacterCreation::onClassChoice(int _index)
    {
        MWBase::Environment::get().getWindowManager()->removeDialog(mClassChoiceDialog);
        mClassChoiceDialog = 0;

        MWBase::Environment::get().getWindowManager()->popGuiMode();

        switch(_index)
        {
            case ClassChoiceDialog::Class_Generate:
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_ClassGenerate);
                break;
            case ClassChoiceDialog::Class_Pick:
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_ClassPick);
                break;
            case ClassChoiceDialog::Class_Create:
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_ClassCreate);
                break;
            case ClassChoiceDialog::Class_Back:
                MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Race);
                break;

        };
    }

    void CharacterCreation::onNameDialogDone(WindowBase* parWindow)
    {
        if (mNameDialog)
        {
            mPlayerName = mNameDialog->getTextInput();
            MWBase::Environment::get().getWindowManager()->setValue("name", mPlayerName);
            MWBase::Environment::get().getMechanicsManager()->setPlayerName(mPlayerName);
            MWBase::Environment::get().getWindowManager()->removeDialog(mNameDialog);
            mNameDialog = 0;
        }

        handleDialogDone(CSE_NameChosen, GM_Race);
    }

    void CharacterCreation::onRaceDialogBack()
    {
        if (mRaceDialog)
        {
            const ESM::NPC &data = mRaceDialog->getResult();
            mPlayerRaceId = data.mRace;
            if (!mPlayerRaceId.empty()) {
                MWBase::Environment::get().getMechanicsManager()->setPlayerRace(
                    data.mRace,
                    data.isMale(),
                    data.mHead,
                    data.mHair
                );
            }
            MWBase::Environment::get().getWindowManager()->removeDialog(mRaceDialog);
            mRaceDialog = 0;
        }

        MWBase::Environment::get().getWindowManager()->popGuiMode();
        MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Name);
    }

    void CharacterCreation::onRaceDialogDone(WindowBase* parWindow)
    {
        if (mRaceDialog)
        {
            const ESM::NPC &data = mRaceDialog->getResult();
            mPlayerRaceId = data.mRace;
            if (!mPlayerRaceId.empty()) {
                MWBase::Environment::get().getMechanicsManager()->setPlayerRace(
                    data.mRace,
                    data.isMale(),
                    data.mHead,
                    data.mHair
                );
            }
            MWBase::Environment::get().getWindowManager()->getInventoryWindow()->rebuildAvatar();

            MWBase::Environment::get().getWindowManager()->removeDialog(mRaceDialog);
            mRaceDialog = 0;
        }

        updatePlayerHealth();

        handleDialogDone(CSE_RaceChosen, GM_Class);
    }

    void CharacterCreation::onBirthSignDialogDone(WindowBase* parWindow)
    {
        if (mBirthSignDialog)
        {
            mPlayerBirthSignId = mBirthSignDialog->getBirthId();
            if (!mPlayerBirthSignId.empty())
                MWBase::Environment::get().getMechanicsManager()->setPlayerBirthsign(mPlayerBirthSignId);
            MWBase::Environment::get().getWindowManager()->removeDialog(mBirthSignDialog);
            mBirthSignDialog = 0;
        }

        updatePlayerHealth();

        handleDialogDone(CSE_BirthSignChosen, GM_Review);
    }

    void CharacterCreation::onBirthSignDialogBack()
    {
        if (mBirthSignDialog)
        {
            MWBase::Environment::get().getMechanicsManager()->setPlayerBirthsign(mBirthSignDialog->getBirthId());
            MWBase::Environment::get().getWindowManager()->removeDialog(mBirthSignDialog);
            mBirthSignDialog = 0;
        }

        MWBase::Environment::get().getWindowManager()->popGuiMode();
        MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Class);
    }

    void CharacterCreation::onCreateClassDialogDone(WindowBase* parWindow)
    {
        if (mCreateClassDialog)
        {
            ESM::Class klass;
            klass.mName = mCreateClassDialog->getName();
            klass.mDescription = mCreateClassDialog->getDescription();
            klass.mData.mSpecialization = mCreateClassDialog->getSpecializationId();
            klass.mData.mIsPlayable = 0x1;

            std::vector<int> attributes = mCreateClassDialog->getFavoriteAttributes();
            assert(attributes.size() == 2);
            klass.mData.mAttribute[0] = attributes[0];
            klass.mData.mAttribute[1] = attributes[1];

            std::vector<ESM::Skill::SkillEnum> majorSkills = mCreateClassDialog->getMajorSkills();
            std::vector<ESM::Skill::SkillEnum> minorSkills = mCreateClassDialog->getMinorSkills();
            assert(majorSkills.size() >= sizeof(klass.mData.mSkills)/sizeof(klass.mData.mSkills[0]));
            assert(minorSkills.size() >= sizeof(klass.mData.mSkills)/sizeof(klass.mData.mSkills[0]));
            for (size_t i = 0; i < sizeof(klass.mData.mSkills)/sizeof(klass.mData.mSkills[0]); ++i)
            {
                klass.mData.mSkills[i][1] = majorSkills[i];
                klass.mData.mSkills[i][0] = minorSkills[i];
            }

            MWBase::Environment::get().getMechanicsManager()->setPlayerClass(klass);
            mPlayerClass = klass;
            MWBase::Environment::get().getWindowManager()->setPlayerClass(klass);

            // Do not delete dialog, so that choices are rembered in case we want to go back and adjust them later
            mCreateClassDialog->setVisible(false);
        }

        updatePlayerHealth();

        handleDialogDone(CSE_ClassChosen, GM_Birth);
    }

    void CharacterCreation::onCreateClassDialogBack()
    {
        // Do not delete dialog, so that choices are rembered in case we want to go back and adjust them later
        mCreateClassDialog->setVisible(false);

        MWBase::Environment::get().getWindowManager()->popGuiMode();
        MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Class);
    }

    void CharacterCreation::onClassQuestionChosen(int _index)
    {
        MWBase::Environment::get().getSoundManager()->stopSay();

        MWBase::Environment::get().getWindowManager()->removeDialog(mGenerateClassQuestionDialog);
        mGenerateClassQuestionDialog = 0;

        if (_index < 0 || _index >= 3)
        {
            MWBase::Environment::get().getWindowManager()->popGuiMode();
            MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Class);
            return;
        }

        ESM::Class::Specialization specialization = mSpecializations[_index];
        if (specialization == ESM::Class::Stealth)
            ++mGenerateClassSpecializations[0];
        else if (specialization == ESM::Class::Combat)
            ++mGenerateClassSpecializations[1];
        else if (specialization == ESM::Class::Magic)
            ++mGenerateClassSpecializations[2];
        ++mGenerateClassStep;
        showClassQuestionDialog();
    }

    void CharacterCreation::showClassQuestionDialog()
    {
        if (mGenerateClassStep == 10)
        {
            static boost::array<ClassPoint, 23> classes = { {
                {"Acrobat",     {6, 2, 2}},
                {"Agent",       {6, 1, 3}},
                {"Archer",      {3, 5, 2}},
                {"Archer",      {5, 5, 0}},
                {"Assassin",    {6, 3, 1}},
                {"Barbarian",   {3, 6, 1}},
                {"Bard",        {3, 3, 3}},
                {"Battlemage",  {1, 3, 6}},
                {"Crusader",    {1, 6, 3}},
                {"Healer",      {3, 1, 6}},
                {"Knight",      {2, 6, 2}},
                {"Monk",        {5, 3, 2}},
                {"Nightblade",  {4, 2, 4}},
                {"Pilgrim",     {5, 2, 3}},
                {"Rogue",       {3, 4, 3}},
                {"Rogue",       {4, 4, 2}},
                {"Rogue",       {5, 4, 1}},
                {"Scout",       {2, 5, 3}},
                {"Sorcerer",    {2, 2, 6}},
                {"Spellsword",  {2, 4, 4}},
                {"Spellsword",  {5, 1, 4}},
                {"Witchhunter", {2, 3, 5}},
                {"Witchhunter", {5, 0, 5}}
            } };

            int match = -1;
            for (unsigned i = 0; i < classes.size(); ++i)
            {
                if (mGenerateClassSpecializations[0] == classes[i].points[0] &&
                    mGenerateClassSpecializations[1] == classes[i].points[1] &&
                    mGenerateClassSpecializations[2] == classes[i].points[2])
                {
                    match = i;
                    mGenerateClass = classes[i].id;
                    break;
                }
            }

            if (match == -1)
            {
                if (mGenerateClassSpecializations[0] >= 7)
                    mGenerateClass = "Thief";
                else if (mGenerateClassSpecializations[1] >= 7)
                    mGenerateClass = "Warrior";
                else if (mGenerateClassSpecializations[2] >= 7)
                    mGenerateClass = "Mage";
                else
                {
                    std::cerr << "Failed to deduce class from chosen answers in generate class dialog" << std::endl;
                    mGenerateClass = "Thief";
                }
            }

            MWBase::Environment::get().getWindowManager()->removeDialog(mGenerateClassResultDialog);
            mGenerateClassResultDialog = 0;

            mGenerateClassResultDialog = new GenerateClassResultDialog();
            mGenerateClassResultDialog->setClassId(mGenerateClass);
            mGenerateClassResultDialog->eventBack += MyGUI::newDelegate(this, &CharacterCreation::onGenerateClassBack);
            mGenerateClassResultDialog->eventDone += MyGUI::newDelegate(this, &CharacterCreation::onGenerateClassDone);
            mGenerateClassResultDialog->setVisible(true);
            return;
        }

        if (mGenerateClassStep > 10)
        {
            MWBase::Environment::get().getWindowManager()->popGuiMode();
            MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Class);
            return;
        }

        MWBase::Environment::get().getWindowManager()->removeDialog(mGenerateClassQuestionDialog);
        mGenerateClassQuestionDialog = 0;

        mGenerateClassQuestionDialog = new InfoBoxDialog();

        InfoBoxDialog::ButtonList buttons;
        mGenerateClassQuestionDialog->setText(sGenerateClassSteps(mGenerateClassStep).mText);
        buttons.push_back(sGenerateClassSteps(mGenerateClassStep).mButtons[0]);
        buttons.push_back(sGenerateClassSteps(mGenerateClassStep).mButtons[1]);
        buttons.push_back(sGenerateClassSteps(mGenerateClassStep).mButtons[2]);
        mGenerateClassQuestionDialog->setButtons(buttons);
        mGenerateClassQuestionDialog->eventButtonSelected += MyGUI::newDelegate(this, &CharacterCreation::onClassQuestionChosen);
        mGenerateClassQuestionDialog->setVisible(true);

        MWBase::Environment::get().getSoundManager()->say(sGenerateClassSteps(mGenerateClassStep).mSound);
    }

    void CharacterCreation::onGenerateClassBack()
    {
        MWBase::Environment::get().getWindowManager()->removeDialog(mGenerateClassResultDialog);
        mGenerateClassResultDialog = 0;

        MWBase::Environment::get().getMechanicsManager()->setPlayerClass(mGenerateClass);

        MWBase::Environment::get().getWindowManager()->popGuiMode();
        MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Class);
    }

    void CharacterCreation::onGenerateClassDone(WindowBase* parWindow)
    {
        MWBase::Environment::get().getWindowManager()->removeDialog(mGenerateClassResultDialog);
        mGenerateClassResultDialog = 0;

        MWBase::Environment::get().getMechanicsManager()->setPlayerClass(mGenerateClass);

        const ESM::Class *klass =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Class>().find(mGenerateClass);

        mPlayerClass = *klass;
        MWBase::Environment::get().getWindowManager()->setPlayerClass(mPlayerClass);

        updatePlayerHealth();

        handleDialogDone(CSE_ClassChosen, GM_Birth);
    }

    CharacterCreation::~CharacterCreation()
    {
        delete mNameDialog;
        delete mRaceDialog;
        delete mClassChoiceDialog;
        delete mGenerateClassQuestionDialog;
        delete mGenerateClassResultDialog;
        delete mPickClassDialog;
        delete mCreateClassDialog;
        delete mBirthSignDialog;
        delete mReviewDialog;
    }

    void CharacterCreation::handleDialogDone(CSE currentStage, int nextMode)
    {
        MWBase::Environment::get().getWindowManager()->popGuiMode();
        if (mCreationStage == CSE_ReviewNext)
        {
            MWBase::Environment::get().getWindowManager()->pushGuiMode(GM_Review);
        }
        else if (mCreationStage >= currentStage)
        {
            MWBase::Environment::get().getWindowManager()->pushGuiMode((GuiMode)nextMode);
        }
        else
        {
            mCreationStage = currentStage;
        }
    }
}
