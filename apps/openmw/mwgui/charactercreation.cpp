#include "charactercreation.hpp"

#include "text_input.hpp"
#include "race.hpp"
#include "class.hpp"
#include "birth.hpp"
#include "review.hpp"
#include "dialogue.hpp"
#include "mode.hpp"

using namespace MWGui;

CharacterCreation::CharacterCreation(WindowManager* _wm, MWWorld::Environment* _environment)
    : mNameDialog(0)
    , mRaceDialog(0)
    , mDialogueWindow(0)
    , mClassChoiceDialog(0)
    , mGenerateClassQuestionDialog(0)
    , mGenerateClassResultDialog(0)
    , mPickClassDialog(0)
    , mCreateClassDialog(0)
    , mBirthSignDialog(0)
    , mReviewDialog(0)
    , mWM(_wm)
    , mEnvironment(_environment)
{
    mCreationStage = CSE_NotStarted;
}

void CharacterCreation::spawnDialog(const char id)
{
    switch (id)
    {
        case GM_Name:
            if(mNameDialog)
                mWM->removeDialog(mNameDialog);
            mNameDialog = new TextInputDialog(*mWM);
            mNameDialog->setTextLabel(mWM->getGameSettingString("sName", "Name"));
            mNameDialog->setTextInput(mPlayerName);
            mNameDialog->setNextButtonShow(mCreationStage >= CSE_NameChosen);
            mNameDialog->eventDone = MyGUI::newDelegate(this, &CharacterCreation::onNameDialogDone);
            mNameDialog->open();
            break;
        
        case GM_Race:
            if (mRaceDialog)
                mWM->removeDialog(mRaceDialog);
            mRaceDialog = new RaceDialog(*mWM);
            mRaceDialog->setNextButtonShow(mCreationStage >= CSE_RaceChosen);
            mRaceDialog->setRaceId(mPlayerRaceId);
            mRaceDialog->eventDone = MyGUI::newDelegate(this, &CharacterCreation::onRaceDialogDone);
            mRaceDialog->eventBack = MyGUI::newDelegate(this, &CharacterCreation::onRaceDialogBack);
            mRaceDialog->open();
            break;
        
        case GM_Class:
            if (mClassChoiceDialog)
                mWM->removeDialog(mClassChoiceDialog);
            mClassChoiceDialog = new ClassChoiceDialog(*mWM);
            mClassChoiceDialog->eventButtonSelected = MyGUI::newDelegate(this, &CharacterCreation::onClassChoice);
            mClassChoiceDialog->open();
            break;
        
        case GM_ClassPick:
            if (mPickClassDialog)
                mWM->removeDialog(mPickClassDialog);
            mPickClassDialog = new PickClassDialog(*mWM);
            mPickClassDialog->setNextButtonShow(mCreationStage >= CSE_ClassChosen);
            mPickClassDialog->setClassId(mPlayerClass.name);
            mPickClassDialog->eventDone = MyGUI::newDelegate(this, &CharacterCreation::onPickClassDialogDone);
            mPickClassDialog->eventBack = MyGUI::newDelegate(this, &CharacterCreation::onPickClassDialogBack);
            mPickClassDialog->open();
            break;
        
        case GM_Birth:
            if (mBirthSignDialog)
                mWM->removeDialog(mBirthSignDialog);
            mBirthSignDialog = new BirthDialog(*mWM);
            mBirthSignDialog->setNextButtonShow(mCreationStage >= CSE_BirthSignChosen);
            mBirthSignDialog->setBirthId(mPlayerBirthSignId);
            mBirthSignDialog->eventDone = MyGUI::newDelegate(this, &CharacterCreation::onBirthSignDialogDone);
            mBirthSignDialog->eventBack = MyGUI::newDelegate(this, &CharacterCreation::onBirthSignDialogBack);
            mBirthSignDialog->open();
            break;
        
        case GM_ClassCreate:
            if (mCreateClassDialog)
                mWM->removeDialog(mCreateClassDialog);
            mCreateClassDialog = new CreateClassDialog(*mWM);
            mCreateClassDialog->eventDone = MyGUI::newDelegate(this, &CharacterCreation::onCreateClassDialogDone);
            mCreateClassDialog->eventBack = MyGUI::newDelegate(this, &CharacterCreation::onCreateClassDialogBack);
            mCreateClassDialog->open();
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
            if (mReviewDialog)
                mWM->removeDialog(mReviewDialog);
            mReviewDialog = new ReviewDialog(*mWM);
            mReviewDialog->setPlayerName(mPlayerName);
            mReviewDialog->setRace(mPlayerRaceId);
            mReviewDialog->setClass(mPlayerClass);
            mReviewDialog->setBirthSign(mPlayerBirthSignId);

            mReviewDialog->setHealth(mWM->getValue("HBar"));
            mReviewDialog->setMagicka(mWM->getValue("MBar"));
            mReviewDialog->setFatigue(mWM->getValue("FBar"));

            {
                std::map<ESM::Attribute::AttributeID, MWMechanics::Stat<int> >::iterator end = mPlayerAttributes.end();
                for (std::map<ESM::Attribute::AttributeID, MWMechanics::Stat<int> >::iterator it = mPlayerAttributes.begin(); it != end; ++it)
                {
                    mReviewDialog->setAttribute(it->first, it->second);
                }
            }

            {
                std::map<ESM::Skill::SkillEnum, MWMechanics::Stat<float> >::iterator end = mPlayerSkillValues.end();
                for (std::map<ESM::Skill::SkillEnum, MWMechanics::Stat<float> >::iterator it = mPlayerSkillValues.begin(); it != end; ++it)
                {
                    mReviewDialog->setSkillValue(it->first, it->second);
                }
                mReviewDialog->configureSkills(mPlayerMajorSkills, mPlayerMinorSkills);
            }

            mReviewDialog->eventDone = MyGUI::newDelegate(this, &CharacterCreation::onReviewDialogDone);
            mReviewDialog->eventBack = MyGUI::newDelegate(this, &CharacterCreation::onReviewDialogBack);
            mReviewDialog->eventActivateDialog = MyGUI::newDelegate(this, &CharacterCreation::onReviewActivateDialog);
            mReviewDialog->open();
            break;
    }
}

void CharacterCreation::onReviewDialogDone(WindowBase* parWindow)
{
    if (mReviewDialog)
        mWM->removeDialog(mReviewDialog);

    mWM->setGuiMode(GM_Game);
}

void CharacterCreation::onReviewDialogBack()
{
    if (mReviewDialog)
        mWM->removeDialog(mReviewDialog);

    mWM->setGuiMode(GM_Birth);
}

void CharacterCreation::onReviewActivateDialog(int parDialog)
{
    if (mReviewDialog)
        mWM->removeDialog(mReviewDialog);
    mCreationStage = CSE_ReviewNext;

    switch(parDialog)
    {
        case ReviewDialog::NAME_DIALOG:
            mWM->setGuiMode(GM_Name);
            break;
        case ReviewDialog::RACE_DIALOG:
            mWM->setGuiMode(GM_Race);
            break;
        case ReviewDialog::CLASS_DIALOG:
            mWM->setGuiMode(GM_Class);
            break;
        case ReviewDialog::BIRTHSIGN_DIALOG:
            mWM->setGuiMode(GM_Birth);
    };
}

void CharacterCreation::onPickClassDialogDone(WindowBase* parWindow)
{
    if (mPickClassDialog)
    {
        const std::string &classId = mPickClassDialog->getClassId();
        if (!classId.empty())
            mEnvironment->mMechanicsManager->setPlayerClass(classId);
        const ESM::Class *klass = mEnvironment->mWorld->getStore().classes.find(classId);
        if (klass)
        {
            mPlayerClass = *klass;
            mWM->setPlayerClass(mPlayerClass);
        }
        mWM->removeDialog(mPickClassDialog);
    }

    //TODO This bit gets repeated a few times; wrap it in a function
    if (mCreationStage == CSE_ReviewNext)
        mWM->setGuiMode(GM_Review);
    else if (mCreationStage >= CSE_ClassChosen)
        mWM->setGuiMode(GM_Birth);
    else
    {
        mCreationStage = CSE_ClassChosen;
        mWM->setGuiMode(GM_Game);
    }
}

void CharacterCreation::onPickClassDialogBack()
{
    if (mPickClassDialog)
    {
        const std::string classId = mPickClassDialog->getClassId();
        if (!classId.empty())
            mEnvironment->mMechanicsManager->setPlayerClass(classId);
        mWM->removeDialog(mPickClassDialog);
    }

    mWM->setGuiMode(GM_Class);
}

void CharacterCreation::onClassChoice(int _index)
{
    if (mClassChoiceDialog)
    {
        mWM->removeDialog(mClassChoiceDialog);
    }

    switch(_index)
    {
        case ClassChoiceDialog::Class_Generate:
            mWM->setGuiMode(GM_ClassGenerate);
            break;
        case ClassChoiceDialog::Class_Pick:
            mWM->setGuiMode(GM_ClassPick);
            break;
        case ClassChoiceDialog::Class_Create:
            mWM->setGuiMode(GM_ClassCreate);
            break;
        case ClassChoiceDialog::Class_Back:
            mWM->setGuiMode(GM_Race);
            break;

    };
}

void CharacterCreation::onNameDialogDone(WindowBase* parWindow)
{
    if (mNameDialog)
    {
        mPlayerName = mNameDialog->getTextInput();
        mWM->setValue("name", mPlayerName);
        mEnvironment->mMechanicsManager->setPlayerName(mPlayerName);
        mWM->removeDialog(mNameDialog);
    }

    if (mCreationStage == CSE_ReviewNext)
        mWM->setGuiMode(GM_Review);
    else if (mCreationStage >= CSE_NameChosen)
        mWM->setGuiMode(GM_Race);
    else
    {
        mCreationStage = CSE_NameChosen;
        mWM->setGuiMode(GM_Game);
    }
}

void CharacterCreation::onRaceDialogBack()
{
    if (mRaceDialog)
    {
        mPlayerRaceId = mRaceDialog->getRaceId();
        if (!mPlayerRaceId.empty())
            mEnvironment->mMechanicsManager->setPlayerRace(mPlayerRaceId, mRaceDialog->getGender() == RaceDialog::GM_Male);
        mWM->removeDialog(mRaceDialog);
    }

    mWM->setGuiMode(GM_Name);
}

void CharacterCreation::onRaceDialogDone(WindowBase* parWindow)
{
    if (mRaceDialog)
    {
        mPlayerRaceId = mRaceDialog->getRaceId();
        mWM->setValue("race", mPlayerRaceId);
        if (!mPlayerRaceId.empty())
            mEnvironment->mMechanicsManager->setPlayerRace(mPlayerRaceId, mRaceDialog->getGender() == RaceDialog::GM_Male);
        mWM->removeDialog(mRaceDialog);
    }

    if (mCreationStage == CSE_ReviewNext)
        mWM->setGuiMode(GM_Review);
    else if(mCreationStage >= CSE_RaceChosen)
        mWM->setGuiMode(GM_Class);
    else
    {
        mCreationStage = CSE_RaceChosen;
        mWM->setGuiMode(GM_Game);
    }
}

void CharacterCreation::onBirthSignDialogDone(WindowBase* parWindow)
{
    if (mBirthSignDialog)
    {
        mPlayerBirthSignId = mBirthSignDialog->getBirthId();
        mWM->setBirthSign(mPlayerBirthSignId);
        if (!mPlayerBirthSignId.empty())
            mEnvironment->mMechanicsManager->setPlayerBirthsign(mPlayerBirthSignId);
        mWM->removeDialog(mBirthSignDialog);
    }

    if (mCreationStage >= CSE_BirthSignChosen)
        mWM->setGuiMode(GM_Review);
    else
    {
        mCreationStage = CSE_BirthSignChosen;
        mWM->setGuiMode(GM_Game);
    }
}

void CharacterCreation::onBirthSignDialogBack()
{
    if (mBirthSignDialog)
    {
        mEnvironment->mMechanicsManager->setPlayerBirthsign(mBirthSignDialog->getBirthId());
        mWM->removeDialog(mBirthSignDialog);
    }

    mWM->setGuiMode(GM_Class);
}

void CharacterCreation::onCreateClassDialogDone(WindowBase* parWindow)
{
    if (mCreateClassDialog)
    {
        ESM::Class klass;
        klass.name = mCreateClassDialog->getName();
        klass.description = mCreateClassDialog->getDescription();
        klass.data.specialization = mCreateClassDialog->getSpecializationId();
        klass.data.isPlayable = 0x1;

        std::vector<int> attributes = mCreateClassDialog->getFavoriteAttributes();
        assert(attributes.size() == 2);
        klass.data.attribute[0] = attributes[0];
        klass.data.attribute[1] = attributes[1];

        std::vector<ESM::Skill::SkillEnum> majorSkills = mCreateClassDialog->getMajorSkills();
        std::vector<ESM::Skill::SkillEnum> minorSkills = mCreateClassDialog->getMinorSkills();
        assert(majorSkills.size() >= sizeof(klass.data.skills)/sizeof(klass.data.skills[0]));
        assert(minorSkills.size() >= sizeof(klass.data.skills)/sizeof(klass.data.skills[0]));
        for (size_t i = 0; i < sizeof(klass.data.skills)/sizeof(klass.data.skills[0]); ++i)
        {
            klass.data.skills[i][1] = majorSkills[i];
            klass.data.skills[i][0] = minorSkills[i];
        }
        mEnvironment->mMechanicsManager->setPlayerClass(klass);
        mPlayerClass = klass;
        mWM->setPlayerClass(klass);

        mWM->removeDialog(mCreateClassDialog);
    }

    if (mCreationStage == CSE_ReviewNext)
        mWM->setGuiMode(GM_Review);
    else if (mCreationStage >= CSE_ClassChosen)
        mWM->setGuiMode(GM_Birth);
    else
    {
        mCreationStage = CSE_ClassChosen;
        mWM->setGuiMode(GM_Game);
    }
}

void CharacterCreation::onCreateClassDialogBack()
{
    if (mCreateClassDialog)
        mWM->removeDialog(mCreateClassDialog);

    mWM->setGuiMode(GM_Class);
}

void CharacterCreation::onClassQuestionChosen(int _index)
{
    if (mGenerateClassQuestionDialog)
        mWM->removeDialog(mGenerateClassQuestionDialog);
    if (_index < 0 || _index >= 3)
    {
        mWM->setGuiMode(GM_Class);
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

        if (mGenerateClassResultDialog)
            mWM->removeDialog(mGenerateClassResultDialog);
        mGenerateClassResultDialog = new GenerateClassResultDialog(*mWM);
        mGenerateClassResultDialog->setClassId(mGenerateClass);
        mGenerateClassResultDialog->eventBack = MyGUI::newDelegate(this, &CharacterCreation::onGenerateClassBack);
        mGenerateClassResultDialog->eventDone = MyGUI::newDelegate(this, &CharacterCreation::onGenerateClassDone);
        mGenerateClassResultDialog->open();
        return;
    }

    if (mGenerateClassStep > sGenerateClassSteps.size())
    {
        mWM->setGuiMode(GM_Class);
        return;
    }

    if (mGenerateClassQuestionDialog)
        mWM->removeDialog(mGenerateClassQuestionDialog);
    mGenerateClassQuestionDialog = new InfoBoxDialog(*mWM);

    InfoBoxDialog::ButtonList buttons;
    mGenerateClassQuestionDialog->setText(sGenerateClassSteps[mGenerateClassStep].mText);
    buttons.push_back(sGenerateClassSteps[mGenerateClassStep].mButtons[0]);
    buttons.push_back(sGenerateClassSteps[mGenerateClassStep].mButtons[1]);
    buttons.push_back(sGenerateClassSteps[mGenerateClassStep].mButtons[2]);
    mGenerateClassQuestionDialog->setButtons(buttons);
    mGenerateClassQuestionDialog->eventButtonSelected = MyGUI::newDelegate(this, &CharacterCreation::onClassQuestionChosen);
    mGenerateClassQuestionDialog->open();
}

void CharacterCreation::onGenerateClassBack()
{
    if(mCreationStage < CSE_ClassChosen)
        mCreationStage = CSE_ClassChosen;

    if (mGenerateClassResultDialog)
        mWM->removeDialog(mGenerateClassResultDialog);
    mEnvironment->mMechanicsManager->setPlayerClass(mGenerateClass);

    mWM->setGuiMode(GM_Class);
}

void CharacterCreation::onGenerateClassDone(WindowBase* parWindow)
{
    if (mGenerateClassResultDialog)
        mWM->removeDialog(mGenerateClassResultDialog);
    mEnvironment->mMechanicsManager->setPlayerClass(mGenerateClass);

    if (mCreationStage == CSE_ReviewNext)
        mWM->setGuiMode(GM_Review);
    else if (mCreationStage >= CSE_ClassChosen)
        mWM->setGuiMode(GM_Birth);
    else
    {
        mCreationStage = CSE_ClassChosen;
        mWM->setGuiMode(GM_Game);
    }
}

CharacterCreation::~CharacterCreation()
{
    delete mNameDialog;
    delete mRaceDialog;
    delete mDialogueWindow;
    delete mClassChoiceDialog;
    delete mGenerateClassQuestionDialog;
    delete mGenerateClassResultDialog;
    delete mPickClassDialog;
    delete mCreateClassDialog;
    delete mBirthSignDialog;
    delete mReviewDialog;
}
