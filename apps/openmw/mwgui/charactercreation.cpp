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
    : nameDialog(0)
    , raceDialog(0)
    , dialogueWindow(0)
    , classChoiceDialog(0)
    , generateClassQuestionDialog(0)
    , generateClassResultDialog(0)
    , pickClassDialog(0)
    , createClassDialog(0)
    , birthSignDialog(0)
    , reviewDialog(0)
    , wm(_wm)
    , environment(_environment)
{
    creationStage = NotStarted;
}

void CharacterCreation::spawnDialog(const char id)
{
    switch (id)
    {
        case GM_Name:
            if(nameDialog)
                wm->removeDialog(nameDialog);
            nameDialog = new TextInputDialog(*wm);
            nameDialog->setTextLabel(wm->getGameSettingString("sName", "Name"));
            nameDialog->setTextInput(playerName);
            nameDialog->setNextButtonShow(creationStage >= NameChosen);
            nameDialog->eventDone = MyGUI::newDelegate(this, &CharacterCreation::onNameDialogDone);
            nameDialog->open();
            break;
        
        case GM_Race:
            if (raceDialog)
                wm->removeDialog(raceDialog);
            raceDialog = new RaceDialog(*wm);
            raceDialog->setNextButtonShow(creationStage >= RaceChosen);
            raceDialog->setRaceId(playerRaceId);
            raceDialog->eventDone = MyGUI::newDelegate(this, &CharacterCreation::onRaceDialogDone);
            raceDialog->eventBack = MyGUI::newDelegate(this, &CharacterCreation::onRaceDialogBack);
            raceDialog->open();
            break;
        
        case GM_Class:
            if (classChoiceDialog)
                wm->removeDialog(classChoiceDialog);
            classChoiceDialog = new ClassChoiceDialog(*wm);
            classChoiceDialog->eventButtonSelected = MyGUI::newDelegate(this, &CharacterCreation::onClassChoice);
            classChoiceDialog->open();
            break;
        
        case GM_ClassPick:
            if (pickClassDialog)
                wm->removeDialog(pickClassDialog);
            pickClassDialog = new PickClassDialog(*wm);
            pickClassDialog->setNextButtonShow(creationStage >= ClassChosen);
            pickClassDialog->setClassId(playerClass.name);
            pickClassDialog->eventDone = MyGUI::newDelegate(this, &CharacterCreation::onPickClassDialogDone);
            pickClassDialog->eventBack = MyGUI::newDelegate(this, &CharacterCreation::onPickClassDialogBack);
            pickClassDialog->open();
            break;
        
        case GM_Birth:
            if (birthSignDialog)
                wm->removeDialog(birthSignDialog);
            birthSignDialog = new BirthDialog(*wm);
            birthSignDialog->setNextButtonShow(creationStage >= BirthSignChosen);
            birthSignDialog->setBirthId(playerBirthSignId);
            birthSignDialog->eventDone = MyGUI::newDelegate(this, &CharacterCreation::onBirthSignDialogDone);
            birthSignDialog->eventBack = MyGUI::newDelegate(this, &CharacterCreation::onBirthSignDialogBack);
            birthSignDialog->open();
            break;
        
        case GM_ClassCreate:
            if (createClassDialog)
                wm->removeDialog(createClassDialog);
            createClassDialog = new CreateClassDialog(*wm);
            createClassDialog->eventDone = MyGUI::newDelegate(this, &CharacterCreation::onCreateClassDialogDone);
            createClassDialog->eventBack = MyGUI::newDelegate(this, &CharacterCreation::onCreateClassDialogBack);
            createClassDialog->open();
            break;
        case GM_ClassGenerate:
            generateClassStep = 0;
            generateClass = "";
            generateClassSpecializations[0] = 0;
            generateClassSpecializations[1] = 0;
            generateClassSpecializations[2] = 0;
            showClassQuestionDialog();
            break;
        case GM_Review:
            if (reviewDialog)
                wm->removeDialog(reviewDialog);
            reviewDialog = new ReviewDialog(*wm);
            reviewDialog->setPlayerName(playerName);
            reviewDialog->setRace(playerRaceId);
            reviewDialog->setClass(playerClass);
            reviewDialog->setBirthSign(playerBirthSignId);

            reviewDialog->setHealth(wm->getValue("HBar"));
            reviewDialog->setMagicka(wm->getValue("MBar"));
            reviewDialog->setFatigue(wm->getValue("FBar"));

            {
                std::map<ESM::Attribute::AttributeID, MWMechanics::Stat<int> >::iterator end = playerAttributes.end();
                for (std::map<ESM::Attribute::AttributeID, MWMechanics::Stat<int> >::iterator it = playerAttributes.begin(); it != end; ++it)
                {
                    reviewDialog->setAttribute(it->first, it->second);
                }
            }

            {
                std::map<ESM::Skill::SkillEnum, MWMechanics::Stat<float> >::iterator end = playerSkillValues.end();
                for (std::map<ESM::Skill::SkillEnum, MWMechanics::Stat<float> >::iterator it = playerSkillValues.begin(); it != end; ++it)
                {
                    reviewDialog->setSkillValue(it->first, it->second);
                }
                reviewDialog->configureSkills(playerMajorSkills, playerMinorSkills);
            }

            reviewDialog->eventDone = MyGUI::newDelegate(this, &CharacterCreation::onReviewDialogDone);
            reviewDialog->eventBack = MyGUI::newDelegate(this, &CharacterCreation::onReviewDialogBack);
            reviewDialog->eventActivateDialog = MyGUI::newDelegate(this, &CharacterCreation::onReviewActivateDialog);
            reviewDialog->open();
            break;
    }
}

void CharacterCreation::onReviewDialogDone(WindowBase* parWindow)
{
    if (reviewDialog)
        wm->removeDialog(reviewDialog);

    wm->setGuiMode(GM_Game);
}

void CharacterCreation::onReviewDialogBack()
{
    if (reviewDialog)
        wm->removeDialog(reviewDialog);

    wm->setGuiMode(GM_Birth);
}

void CharacterCreation::onReviewActivateDialog(int parDialog)
{
    if (reviewDialog)
        wm->removeDialog(reviewDialog);
    creationStage = ReviewNext;

    switch(parDialog)
    {
        case ReviewDialog::NAME_DIALOG:
            wm->setGuiMode(GM_Name);
            break;
        case ReviewDialog::RACE_DIALOG:
            wm->setGuiMode(GM_Race);
            break;
        case ReviewDialog::CLASS_DIALOG:
            wm->setGuiMode(GM_Class);
            break;
        case ReviewDialog::BIRTHSIGN_DIALOG:
            wm->setGuiMode(GM_Birth);
    };
}

void CharacterCreation::onPickClassDialogDone(WindowBase* parWindow)
{
    if (pickClassDialog)
    {
        const std::string &classId = pickClassDialog->getClassId();
        if (!classId.empty())
            environment->mMechanicsManager->setPlayerClass(classId);
        const ESM::Class *klass = environment->mWorld->getStore().classes.find(classId);
        if (klass)
        {
            playerClass = *klass;
            wm->setPlayerClass(playerClass);
        }
        wm->removeDialog(pickClassDialog);
    }

    //TODO This bit gets repeated a few times; wrap it in a function
    if (creationStage == ReviewNext)
        wm->setGuiMode(GM_Review);
    else if (creationStage >= ClassChosen)
        wm->setGuiMode(GM_Birth);
    else
    {
        creationStage = ClassChosen;
        wm->setGuiMode(GM_Game);
    }
}

void CharacterCreation::onPickClassDialogBack()
{
    if (pickClassDialog)
    {
        const std::string classId = pickClassDialog->getClassId();
        if (!classId.empty())
            environment->mMechanicsManager->setPlayerClass(classId);
        wm->removeDialog(pickClassDialog);
    }

    wm->setGuiMode(GM_Class);
}

void CharacterCreation::onClassChoice(int _index)
{
    if (classChoiceDialog)
    {
        wm->removeDialog(classChoiceDialog);
    }

    switch(_index)
    {
        case ClassChoiceDialog::Class_Generate:
            wm->setGuiMode(GM_ClassGenerate);
            break;
        case ClassChoiceDialog::Class_Pick:
            wm->setGuiMode(GM_ClassPick);
            break;
        case ClassChoiceDialog::Class_Create:
            wm->setGuiMode(GM_ClassCreate);
            break;
        case ClassChoiceDialog::Class_Back:
            wm->setGuiMode(GM_Race);
            break;

    };
}

void CharacterCreation::onNameDialogDone(WindowBase* parWindow)
{
    if (nameDialog)
    {
        playerName = nameDialog->getTextInput();
        wm->setValue("name", playerName);
        environment->mMechanicsManager->setPlayerName(playerName);
        wm->removeDialog(nameDialog);
    }

    if (creationStage == ReviewNext)
        wm->setGuiMode(GM_Review);
    else if (creationStage >= NameChosen)
        wm->setGuiMode(GM_Race);
    else
    {
        creationStage = NameChosen;
        wm->setGuiMode(GM_Game);
    }
}

void CharacterCreation::onRaceDialogBack()
{
    if (raceDialog)
    {
        playerRaceId = raceDialog->getRaceId();
        if (!playerRaceId.empty())
            environment->mMechanicsManager->setPlayerRace(playerRaceId, raceDialog->getGender() == RaceDialog::GM_Male);
        wm->removeDialog(raceDialog);
    }

    wm->setGuiMode(GM_Name);
}

void CharacterCreation::onRaceDialogDone(WindowBase* parWindow)
{
    if (raceDialog)
    {
        playerRaceId = raceDialog->getRaceId();
        wm->setValue("race", playerRaceId);
        if (!playerRaceId.empty())
            environment->mMechanicsManager->setPlayerRace(playerRaceId, raceDialog->getGender() == RaceDialog::GM_Male);
        wm->removeDialog(raceDialog);
    }

    if (creationStage == ReviewNext)
        wm->setGuiMode(GM_Review);
    else if(creationStage >= RaceChosen)
        wm->setGuiMode(GM_Class);
    else
    {
        creationStage = RaceChosen;
        wm->setGuiMode(GM_Game);
    }
}

void CharacterCreation::onBirthSignDialogDone(WindowBase* parWindow)
{
    if (birthSignDialog)
    {
        playerBirthSignId = birthSignDialog->getBirthId();
        wm->setBirthSign(playerBirthSignId);
        if (!playerBirthSignId.empty())
            environment->mMechanicsManager->setPlayerBirthsign(playerBirthSignId);
        wm->removeDialog(birthSignDialog);
    }

    if (creationStage >= BirthSignChosen)
        wm->setGuiMode(GM_Review);
    else
    {
        creationStage = BirthSignChosen;
        wm->setGuiMode(GM_Game);
    }
}

void CharacterCreation::onBirthSignDialogBack()
{
    if (birthSignDialog)
    {
        environment->mMechanicsManager->setPlayerBirthsign(birthSignDialog->getBirthId());
        wm->removeDialog(birthSignDialog);
    }

    wm->setGuiMode(GM_Class);
}

void CharacterCreation::onCreateClassDialogDone(WindowBase* parWindow)
{
    if (createClassDialog)
    {
        ESM::Class klass;
        klass.name = createClassDialog->getName();
        klass.description = createClassDialog->getDescription();
        klass.data.specialization = createClassDialog->getSpecializationId();
        klass.data.isPlayable = 0x1;

        std::vector<int> attributes = createClassDialog->getFavoriteAttributes();
        assert(attributes.size() == 2);
        klass.data.attribute[0] = attributes[0];
        klass.data.attribute[1] = attributes[1];

        std::vector<ESM::Skill::SkillEnum> majorSkills = createClassDialog->getMajorSkills();
        std::vector<ESM::Skill::SkillEnum> minorSkills = createClassDialog->getMinorSkills();
        assert(majorSkills.size() >= sizeof(klass.data.skills)/sizeof(klass.data.skills[0]));
        assert(minorSkills.size() >= sizeof(klass.data.skills)/sizeof(klass.data.skills[0]));
        for (size_t i = 0; i < sizeof(klass.data.skills)/sizeof(klass.data.skills[0]); ++i)
        {
            klass.data.skills[i][1] = majorSkills[i];
            klass.data.skills[i][0] = minorSkills[i];
        }
        environment->mMechanicsManager->setPlayerClass(klass);
        playerClass = klass;
        wm->setPlayerClass(klass);

        wm->removeDialog(createClassDialog);
    }

    if (creationStage == ReviewNext)
        wm->setGuiMode(GM_Review);
    else if (creationStage >= ClassChosen)
        wm->setGuiMode(GM_Birth);
    else
    {
        creationStage = ClassChosen;
        wm->setGuiMode(GM_Game);
    }
}

void CharacterCreation::onCreateClassDialogBack()
{
    if (createClassDialog)
        wm->removeDialog(createClassDialog);

    wm->setGuiMode(GM_Class);
}

void CharacterCreation::onClassQuestionChosen(int _index)
{
    if (generateClassQuestionDialog)
        wm->removeDialog(generateClassQuestionDialog);
    if (_index < 0 || _index >= 3)
    {
        wm->setGuiMode(GM_Class);
        return;
    }

    ESM::Class::Specialization specialization = generateClassSteps[generateClassStep].specializations[_index];
    if (specialization == ESM::Class::Stealth)
        ++generateClassSpecializations[0];
    else if (specialization == ESM::Class::Combat)
        ++generateClassSpecializations[1];
    else if (specialization == ESM::Class::Magic)
        ++generateClassSpecializations[2];
    ++generateClassStep;
    showClassQuestionDialog();
}

void CharacterCreation::showClassQuestionDialog()
{
    if (generateClassStep == generateClassSteps.size())
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
            if (generateClassSpecializations[0] == classes[i].points[0] &&
                generateClassSpecializations[1] == classes[i].points[1] &&
                generateClassSpecializations[2] == classes[i].points[2])
            {
                match = i;
                generateClass = classes[i].id;
                break;
            }
        }

        if (match == -1)
        {
            if (generateClassSpecializations[0] >= 7)
                generateClass = "Thief";
            else if (generateClassSpecializations[1] >= 7)
                generateClass = "Warrior";
            else if (generateClassSpecializations[2] >= 7)
                generateClass = "Mage";
            else
            {
                std::cerr << "Failed to deduce class from chosen answers in generate class dialog" << std::endl;
                generateClass = "Thief";
            }
        }

        if (generateClassResultDialog)
            wm->removeDialog(generateClassResultDialog);
        generateClassResultDialog = new GenerateClassResultDialog(*wm);
        generateClassResultDialog->setClassId(generateClass);
        generateClassResultDialog->eventBack = MyGUI::newDelegate(this, &CharacterCreation::onGenerateClassBack);
        generateClassResultDialog->eventDone = MyGUI::newDelegate(this, &CharacterCreation::onGenerateClassDone);
        generateClassResultDialog->open();
        return;
    }

    if (generateClassStep > generateClassSteps.size())
    {
        wm->setGuiMode(GM_Class);
        return;
    }

    if (generateClassQuestionDialog)
        wm->removeDialog(generateClassQuestionDialog);
    generateClassQuestionDialog = new InfoBoxDialog(*wm);

    InfoBoxDialog::ButtonList buttons;
    generateClassQuestionDialog->setText(generateClassSteps[generateClassStep].text);
    buttons.push_back(generateClassSteps[generateClassStep].buttons[0]);
    buttons.push_back(generateClassSteps[generateClassStep].buttons[1]);
    buttons.push_back(generateClassSteps[generateClassStep].buttons[2]);
    generateClassQuestionDialog->setButtons(buttons);
    generateClassQuestionDialog->eventButtonSelected = MyGUI::newDelegate(this, &CharacterCreation::onClassQuestionChosen);
    generateClassQuestionDialog->open();
}

void CharacterCreation::onGenerateClassBack()
{
    if(creationStage < ClassChosen)
        creationStage = ClassChosen;

    if (generateClassResultDialog)
        wm->removeDialog(generateClassResultDialog);
    environment->mMechanicsManager->setPlayerClass(generateClass);

    wm->setGuiMode(GM_Class);
}

void CharacterCreation::onGenerateClassDone(WindowBase* parWindow)
{
    if (generateClassResultDialog)
        wm->removeDialog(generateClassResultDialog);
    environment->mMechanicsManager->setPlayerClass(generateClass);

    if (creationStage == ReviewNext)
        wm->setGuiMode(GM_Review);
    else if (creationStage >= ClassChosen)
        wm->setGuiMode(GM_Birth);
    else
    {
        creationStage = ClassChosen;
        wm->setGuiMode(GM_Game);
    }
}

CharacterCreation::~CharacterCreation()
{
    delete nameDialog;
    delete raceDialog;
    delete dialogueWindow;
    delete classChoiceDialog;
    delete generateClassQuestionDialog;
    delete generateClassResultDialog;
    delete pickClassDialog;
    delete createClassDialog;
    delete birthSignDialog;
    delete reviewDialog;
}
