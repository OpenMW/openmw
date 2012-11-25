#include "charactercreation.hpp"

#include "text_input.hpp"
#include "race.hpp"
#include "class.hpp"
#include "birth.hpp"
#include "review.hpp"
#include "dialogue.hpp"
#include "mode.hpp"
#include "inventorywindow.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"

namespace
{
    struct Step
    {
        const char* mText;
        const char* mButtons[3];
        const char* mSound;
        ESM::Class::Specialization mSpecializations[3]; // The specialization for each answer
    };

    static boost::array<Step, 10> sGenerateClassSteps = { {
        // Question 1
        {"On a clear day you chance upon a strange animal, its legs trapped in a hunter's clawsnare. Judging from the bleeding, it will not survive long.",
        {"Draw your dagger, mercifully endings its life with a single thrust.",
        "Use herbs from your pack to put it to sleep.",
        "Do not interfere in the natural evolution of events, but rather take the opportunity to learn more about a strange animal that you have never seen before."},
        "vo\\misc\\chargen qa1.wav",
        {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 2
        {"One Summer afternoon your father gives you a choice of chores.",
        {"Work in the forge with him casting iron for a new plow.",
        "Gather herbs for your mother who is preparing dinner.",
        "Go catch fish at the stream using a net and line."},
        "vo\\misc\\chargen qa2.wav",
        {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 3
        {"Your cousin has given you a very embarrassing nickname and, even worse, likes to call you it in front of your friends. You asked him to stop, but he finds it very amusing to watch you blush.",
        {"Beat up your cousin, then tell him that if he ever calls you that nickname again, you will bloody him worse than this time.",
        "Make up a story that makes your nickname a badge of honor instead of something humiliating.",
        "Make up an even more embarrassing nickname for him and use it constantly until he learns his lesson."},
        "vo\\misc\\chargen qa3.wav",
        {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 4
        {"There is a lot of heated discussion at the local tavern over a grouped of people called 'Telepaths'. They have been hired by certain City-State kings. Rumor has it these Telepaths read a person's mind and tell their lord whether a follower is telling the truth or not.",
        {"This is a terrible practice. A person's thoughts are his own and no one, not even a king, has the right to make such an invasion into another human's mind.",
        "Loyal followers to the king have nothing to fear from a Telepath. It is important to have a method of finding assassins and spies before it is too late.",
        "In these times, it is a necessary evil. Although you do not necessarily like the idea, a Telepath could have certain advantages during a time of war or in finding someone innocent of a crime."},
        "vo\\misc\\chargen qa4.wav",
        {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 5
        {"Your mother sends you to the market with a list of goods to buy. After you finish you find that by mistake a shopkeeper has given you too much money back in exchange for one of the items.",
        {"Return to the store and give the shopkeeper his hard-earned money, explaining to him the mistake?",
        "Decide to put the extra money to good use and purchase items that would help your family?",
        "Pocket the extra money, knowing that shopkeepers in general tend to overcharge customers anyway?"},
        "vo\\misc\\chargen qa5.wav",
        {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 6
        {"While in the market place you witness a thief cut a purse from a noble. Even as he does so, the noble notices and calls for the city guards. In his haste to get away, the thief drops the purse near you. Surprisingly no one seems to notice the bag of coins at your feet.",
        {"Pick up the bag and signal to the guard, knowing that the only honorable thing to do is return the money to its rightful owner.",
        "Leave the bag there, knowing that it is better not to get involved.",
        "Pick up the bag and pocket it, knowing that the extra windfall will help your family in times of trouble."},
        "vo\\misc\\chargen qa6.wav",
        {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 7
        {"Your father sends you on a task which you loathe, cleaning the stables. On the way there, pitchfork in hand, you run into your friend from the homestead near your own. He offers to do it for you, in return for a future favor of his choosing.",
        {"Decline his offer, knowing that your father expects you to do the work, and it is better not to be in debt.",
        "Ask him to help you, knowing that two people can do the job faster than one, and agree to help him with one task of his choosing in the future.",
        "Accept his offer, reasoning that as long as the stables are cleaned, it matters not who does the cleaning."},
        "vo\\misc\\chargen qa7.wav",
        {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 8
        {"Your mother asks you to help fix the stove. While you are working, a very hot pipe slips its mooring and falls towards her.",
        {"Position yourself between the pipe and your mother.",
        "Grab the hot pipe and try to push it away.",
        "Push your mother out of the way."},
        "vo\\misc\\chargen qa8.wav",
        {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 9
        {"While in town the baker gives you a sweetroll. Delighted, you take it into an alley to enjoy only to be intercepted by a gang of three other kids your age. The leader demands the sweetroll, or else he and his friends will beat you and take it.",
        {"Drop the sweetroll and step on it, then get ready for the fight.",
        "Give him the sweetroll now without argument, knowing that later this afternoon you will have all your friends with you and can come and take whatever he owes you.",
        "Act like you're going to give him the sweetroll, but at the last minute throw it in the air, hoping that they'll pay attention to it long enough for you to get a shot in on the leader."},
        "vo\\misc\\chargen qa9.wav",
        {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 10
        {"Entering town you find that you are witness to a very well-dressed man running from a crowd. He screams to you for help. The crowd behind him seem very angry.",
        {"Rush to the town's aid immediately, despite your lack of knowledge of the circumstances.",
        "Stand aside and allow the man and the mob to pass, realizing it is probably best not to get involved.",
        "Rush to the man's aid immediately, despite your lack of knowledge of the circumstances."},
        "vo\\misc\\chargen qa10.wav",
        {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        }
    } };

    struct ClassPoint
    {
        const char *id;
        // Specialization points to match, in order: Stealth, Combat, Magic
        // Note: Order is taken from http://www.uesp.net/wiki/Morrowind:Class_Quiz
        unsigned int points[3];
    };
}

using namespace MWGui;

CharacterCreation::CharacterCreation(MWBase::WindowManager* _wm)
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
    , mWM(_wm)
{
    mCreationStage = CSE_NotStarted;
}

void CharacterCreation::setValue (const std::string& id, const MWMechanics::Stat<int>& value)
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

void CharacterCreation::setValue(const ESM::Skill::SkillEnum parSkill, const MWMechanics::Stat<float>& value)
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
            mWM->removeDialog(mNameDialog);
            mNameDialog = 0;
            mNameDialog = new TextInputDialog(*mWM);
            mNameDialog->setTextLabel(mWM->getGameSettingString("sName", "Name"));
            mNameDialog->setTextInput(mPlayerName);
            mNameDialog->setNextButtonShow(mCreationStage >= CSE_NameChosen);
            mNameDialog->eventDone += MyGUI::newDelegate(this, &CharacterCreation::onNameDialogDone);
            mNameDialog->setVisible(true);
            break;

        case GM_Race:
            mWM->removeDialog(mRaceDialog);
            mRaceDialog = 0;
            mRaceDialog = new RaceDialog(*mWM);
            mRaceDialog->setNextButtonShow(mCreationStage >= CSE_RaceChosen);
            mRaceDialog->setRaceId(mPlayerRaceId);
            mRaceDialog->eventDone += MyGUI::newDelegate(this, &CharacterCreation::onRaceDialogDone);
            mRaceDialog->eventBack += MyGUI::newDelegate(this, &CharacterCreation::onRaceDialogBack);
            mRaceDialog->setVisible(true);;
            break;

        case GM_Class:
            mWM->removeDialog(mClassChoiceDialog);
            mClassChoiceDialog = 0;
            mClassChoiceDialog = new ClassChoiceDialog(*mWM);
            mClassChoiceDialog->eventButtonSelected += MyGUI::newDelegate(this, &CharacterCreation::onClassChoice);
            mClassChoiceDialog->setVisible(true);
            break;

        case GM_ClassPick:
            mWM->removeDialog(mPickClassDialog);
            mPickClassDialog = 0;
            mPickClassDialog = new PickClassDialog(*mWM);
            mPickClassDialog->setNextButtonShow(mCreationStage >= CSE_ClassChosen);
            mPickClassDialog->setClassId(mPlayerClass.mName);
            mPickClassDialog->eventDone += MyGUI::newDelegate(this, &CharacterCreation::onPickClassDialogDone);
            mPickClassDialog->eventBack += MyGUI::newDelegate(this, &CharacterCreation::onPickClassDialogBack);
            mPickClassDialog->setVisible(true);
            break;

        case GM_Birth:
            mWM->removeDialog(mBirthSignDialog);
            mBirthSignDialog = 0;
            mBirthSignDialog = new BirthDialog(*mWM);
            mBirthSignDialog->setNextButtonShow(mCreationStage >= CSE_BirthSignChosen);
            mBirthSignDialog->setBirthId(mPlayerBirthSignId);
            mBirthSignDialog->eventDone += MyGUI::newDelegate(this, &CharacterCreation::onBirthSignDialogDone);
            mBirthSignDialog->eventBack += MyGUI::newDelegate(this, &CharacterCreation::onBirthSignDialogBack);
            mBirthSignDialog->setVisible(true);
            break;

        case GM_ClassCreate:
            mWM->removeDialog(mCreateClassDialog);
            mCreateClassDialog = 0;
            mCreateClassDialog = new CreateClassDialog(*mWM);
            mCreateClassDialog->setNextButtonShow(mCreationStage >= CSE_ClassChosen);
            mCreateClassDialog->eventDone += MyGUI::newDelegate(this, &CharacterCreation::onCreateClassDialogDone);
            mCreateClassDialog->eventBack += MyGUI::newDelegate(this, &CharacterCreation::onCreateClassDialogBack);
            mCreateClassDialog->setVisible(true);
            break;
        case GM_ClassGenerate:
            mGenerateClassStep = 0;
            mGenerateClass = "";
            mGenerateClassSpecializations[0] = 0;
            mGenerateClassSpecializations[1] = 0;
            mGenerateClassSpecializations[2] = 0;
            showClassQuestionDialog();
            break;
        case GM_Review:
            mWM->removeDialog(mReviewDialog);
            mReviewDialog = 0;
            mReviewDialog = new ReviewDialog(*mWM);
            mReviewDialog->setPlayerName(mPlayerName);
            mReviewDialog->setRace(mPlayerRaceId);
            mReviewDialog->setClass(mPlayerClass);
            mReviewDialog->setBirthSign(mPlayerBirthSignId);

            mReviewDialog->setHealth(mPlayerHealth);
            mReviewDialog->setMagicka(mPlayerMagicka);
            mReviewDialog->setFatigue(mPlayerFatigue);

            {
                std::map<int, MWMechanics::Stat<int> > attributes = mWM->getPlayerAttributeValues();
                for (std::map<int, MWMechanics::Stat<int> >::iterator it = attributes.begin();
                    it != attributes.end(); ++it)
                {
                    mReviewDialog->setAttribute(static_cast<ESM::Attribute::AttributeID> (it->first), it->second);
                }
            }

            {
                std::map<int, MWMechanics::Stat<float> > skills = mWM->getPlayerSkillValues();
                for (std::map<int, MWMechanics::Stat<float> >::iterator it = skills.begin();
                    it != skills.end(); ++it)
                {
                    mReviewDialog->setSkillValue(static_cast<ESM::Skill::SkillEnum> (it->first), it->second);
                }
                mReviewDialog->configureSkills(mWM->getPlayerMajorSkills(), mWM->getPlayerMinorSkills());
            }

            mReviewDialog->eventDone += MyGUI::newDelegate(this, &CharacterCreation::onReviewDialogDone);
            mReviewDialog->eventBack += MyGUI::newDelegate(this, &CharacterCreation::onReviewDialogBack);
            mReviewDialog->eventActivateDialog += MyGUI::newDelegate(this, &CharacterCreation::onReviewActivateDialog);
            mReviewDialog->setVisible(true);
            break;
    }
}

void CharacterCreation::setPlayerHealth (const MWMechanics::DynamicStat<float>& value)
{
    mPlayerHealth = value;
}

void CharacterCreation::setPlayerMagicka (const MWMechanics::DynamicStat<float>& value)
{
    mPlayerMagicka = value;
}

void CharacterCreation::setPlayerFatigue (const MWMechanics::DynamicStat<float>& value)
{
    mPlayerFatigue = value;
}

void CharacterCreation::onReviewDialogDone(WindowBase* parWindow)
{
    mWM->removeDialog(mReviewDialog);
    mReviewDialog = 0;

    mWM->popGuiMode();
}

void CharacterCreation::onReviewDialogBack()
{
    mWM->removeDialog(mReviewDialog);
    mReviewDialog = 0;

    mWM->pushGuiMode(GM_Birth);
}

void CharacterCreation::onReviewActivateDialog(int parDialog)
{
    mWM->removeDialog(mReviewDialog);
    mReviewDialog = 0;
    mCreationStage = CSE_ReviewNext;

    mWM->popGuiMode();

    switch(parDialog)
    {
        case ReviewDialog::NAME_DIALOG:
            mWM->pushGuiMode(GM_Name);
            break;
        case ReviewDialog::RACE_DIALOG:
            mWM->pushGuiMode(GM_Race);
            break;
        case ReviewDialog::CLASS_DIALOG:
            mWM->pushGuiMode(GM_Class);
            break;
        case ReviewDialog::BIRTHSIGN_DIALOG:
            mWM->pushGuiMode(GM_Birth);
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
            mWM->setPlayerClass(mPlayerClass);
        }
        mWM->removeDialog(mPickClassDialog);
        mPickClassDialog = 0;
    }

    //TODO This bit gets repeated a few times; wrap it in a function
    if (mCreationStage == CSE_ReviewNext)
    {
        mWM->popGuiMode();
        mWM->pushGuiMode(GM_Review);
    }
    else if (mCreationStage >= CSE_ClassChosen)
    {
        mWM->popGuiMode();
        mWM->pushGuiMode(GM_Birth);
    }
    else
    {
        mCreationStage = CSE_ClassChosen;
        mWM->popGuiMode();
    }
}

void CharacterCreation::onPickClassDialogBack()
{
    if (mPickClassDialog)
    {
        const std::string classId = mPickClassDialog->getClassId();
        if (!classId.empty())
            MWBase::Environment::get().getMechanicsManager()->setPlayerClass(classId);
        mWM->removeDialog(mPickClassDialog);
        mPickClassDialog = 0;
    }

    mWM->popGuiMode();
    mWM->pushGuiMode(GM_Class);
}

void CharacterCreation::onClassChoice(int _index)
{
    mWM->removeDialog(mClassChoiceDialog);
    mClassChoiceDialog = 0;

    mWM->popGuiMode();

    switch(_index)
    {
        case ClassChoiceDialog::Class_Generate:
            mWM->pushGuiMode(GM_ClassGenerate);
            break;
        case ClassChoiceDialog::Class_Pick:
            mWM->pushGuiMode(GM_ClassPick);
            break;
        case ClassChoiceDialog::Class_Create:
            mWM->pushGuiMode(GM_ClassCreate);
            break;
        case ClassChoiceDialog::Class_Back:
            mWM->pushGuiMode(GM_Race);
            break;

    };
}

void CharacterCreation::onNameDialogDone(WindowBase* parWindow)
{
    if (mNameDialog)
    {
        mPlayerName = mNameDialog->getTextInput();
        mWM->setValue("name", mPlayerName);
        MWBase::Environment::get().getMechanicsManager()->setPlayerName(mPlayerName);
        mWM->removeDialog(mNameDialog);
        mNameDialog = 0;
    }

    if (mCreationStage == CSE_ReviewNext)
    {
        mWM->popGuiMode();
        mWM->pushGuiMode(GM_Review);
    }
    else if (mCreationStage >= CSE_NameChosen)
    {
        mWM->popGuiMode();
        mWM->pushGuiMode(GM_Race);
    }
    else
    {
        mCreationStage = CSE_NameChosen;
        mWM->popGuiMode();
    }
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
        mWM->removeDialog(mRaceDialog);
        mRaceDialog = 0;
    }

    mWM->popGuiMode();
    mWM->pushGuiMode(GM_Name);
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
        mWM->getInventoryWindow()->rebuildAvatar();

        mWM->removeDialog(mRaceDialog);
        mRaceDialog = 0;
    }

    if (mCreationStage == CSE_ReviewNext)
    {
        mWM->popGuiMode();
        mWM->pushGuiMode(GM_Review);
    }
    else if (mCreationStage >= CSE_RaceChosen)
    {
        mWM->popGuiMode();
        mWM->pushGuiMode(GM_Class);
    }
    else
    {
        mCreationStage = CSE_RaceChosen;
        mWM->popGuiMode();
    }
}

void CharacterCreation::onBirthSignDialogDone(WindowBase* parWindow)
{
    if (mBirthSignDialog)
    {
        mPlayerBirthSignId = mBirthSignDialog->getBirthId();
        if (!mPlayerBirthSignId.empty())
            MWBase::Environment::get().getMechanicsManager()->setPlayerBirthsign(mPlayerBirthSignId);
        mWM->removeDialog(mBirthSignDialog);
        mBirthSignDialog = 0;
    }

    if (mCreationStage >= CSE_BirthSignChosen)
    {
        mWM->popGuiMode();
        mWM->pushGuiMode(GM_Review);
    }
    else
    {
        mCreationStage = CSE_BirthSignChosen;
        mWM->popGuiMode();
    }
}

void CharacterCreation::onBirthSignDialogBack()
{
    if (mBirthSignDialog)
    {
        MWBase::Environment::get().getMechanicsManager()->setPlayerBirthsign(mBirthSignDialog->getBirthId());
        mWM->removeDialog(mBirthSignDialog);
        mBirthSignDialog = 0;
    }

    mWM->popGuiMode();
    mWM->pushGuiMode(GM_Class);
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
        mWM->setPlayerClass(klass);

        mWM->removeDialog(mCreateClassDialog);
        mCreateClassDialog = 0;
    }

    if (mCreationStage == CSE_ReviewNext)
    {
        mWM->popGuiMode();
        mWM->pushGuiMode(GM_Review);
    }
    else if (mCreationStage >= CSE_ClassChosen)
    {
        mWM->popGuiMode();
        mWM->pushGuiMode(GM_Birth);
    }
    else
    {
        mCreationStage = CSE_ClassChosen;
        mWM->popGuiMode();
    }
}

void CharacterCreation::onCreateClassDialogBack()
{
    mWM->removeDialog(mCreateClassDialog);
    mCreateClassDialog = 0;

    mWM->popGuiMode();
    mWM->pushGuiMode(GM_Class);
}

void CharacterCreation::onClassQuestionChosen(int _index)
{
    MWBase::Environment::get().getSoundManager()->stopSay();

    mWM->removeDialog(mGenerateClassQuestionDialog);
    mGenerateClassQuestionDialog = 0;

    if (_index < 0 || _index >= 3)
    {
        mWM->popGuiMode();
        mWM->pushGuiMode(GM_Class);
        return;
    }

    ESM::Class::Specialization specialization = sGenerateClassSteps[mGenerateClassStep].mSpecializations[_index];
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
    if (mGenerateClassStep == sGenerateClassSteps.size())
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

        mWM->removeDialog(mGenerateClassResultDialog);
        mGenerateClassResultDialog = 0;

        mGenerateClassResultDialog = new GenerateClassResultDialog(*mWM);
        mGenerateClassResultDialog->setClassId(mGenerateClass);
        mGenerateClassResultDialog->eventBack += MyGUI::newDelegate(this, &CharacterCreation::onGenerateClassBack);
        mGenerateClassResultDialog->eventDone += MyGUI::newDelegate(this, &CharacterCreation::onGenerateClassDone);
        mGenerateClassResultDialog->setVisible(true);
        return;
    }

    if (mGenerateClassStep > sGenerateClassSteps.size())
    {
        mWM->popGuiMode();
        mWM->pushGuiMode(GM_Class);
        return;
    }

    mWM->removeDialog(mGenerateClassQuestionDialog);
    mGenerateClassQuestionDialog = 0;

    mGenerateClassQuestionDialog = new InfoBoxDialog(*mWM);

    InfoBoxDialog::ButtonList buttons;
    mGenerateClassQuestionDialog->setText(sGenerateClassSteps[mGenerateClassStep].mText);
    buttons.push_back(sGenerateClassSteps[mGenerateClassStep].mButtons[0]);
    buttons.push_back(sGenerateClassSteps[mGenerateClassStep].mButtons[1]);
    buttons.push_back(sGenerateClassSteps[mGenerateClassStep].mButtons[2]);
    mGenerateClassQuestionDialog->setButtons(buttons);
    mGenerateClassQuestionDialog->eventButtonSelected += MyGUI::newDelegate(this, &CharacterCreation::onClassQuestionChosen);
    mGenerateClassQuestionDialog->setVisible(true);

    MWBase::Environment::get().getSoundManager()->say(sGenerateClassSteps[mGenerateClassStep].mSound);
}

void CharacterCreation::onGenerateClassBack()
{
    if(mCreationStage < CSE_ClassChosen)
        mCreationStage = CSE_ClassChosen;

    mWM->removeDialog(mGenerateClassResultDialog);
    mGenerateClassResultDialog = 0;

    MWBase::Environment::get().getMechanicsManager()->setPlayerClass(mGenerateClass);

    mWM->popGuiMode();
    mWM->pushGuiMode(GM_Class);
}

void CharacterCreation::onGenerateClassDone(WindowBase* parWindow)
{
    mWM->removeDialog(mGenerateClassResultDialog);
    mGenerateClassResultDialog = 0;

    MWBase::Environment::get().getMechanicsManager()->setPlayerClass(mGenerateClass);

    const ESM::Class *klass =
        MWBase::Environment::get().getWorld()->getStore().get<ESM::Class>().find(mGenerateClass);

    mPlayerClass = *klass;
    mWM->setPlayerClass(mPlayerClass);

    if (mCreationStage == CSE_ReviewNext)
    {
        mWM->popGuiMode();
        mWM->pushGuiMode(GM_Review);
    }
    else if (mCreationStage >= CSE_ClassChosen)
    {
        mWM->popGuiMode();
        mWM->pushGuiMode(GM_Birth);
    }
    else
    {
        mCreationStage = CSE_ClassChosen;
        mWM->popGuiMode();
    }
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
