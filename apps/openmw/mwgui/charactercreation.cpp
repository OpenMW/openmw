#include "charactercreation.hpp"

#include "text_input.hpp"
#include "race.hpp"
#include "class.hpp"
#include "birth.hpp"
#include "review.hpp"
#include "dialogue.hpp"

#include "mode.hpp"

using namespace MWGui;

CharacterCreation::CharacterCreation(WindowManager* _wm)
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
{
    creationStage = NotStarted;
}

void CharacterCreation::spawnDialog(const char id)
{
    //Switch this out with a switch/case structure
    if(id == GM_Name)
    {
        if(nameDialog)
            wm->removeDialog(nameDialog);
        nameDialog = new TextInputDialog(*wm);
        nameDialog->setTextLabel(wm->getGameSettingString("sName", "Name"));
        nameDialog->setTextInput(playerName);
        nameDialog->setNextButtonShow(creationStage >= NameChosen);
        nameDialog->eventDone = MyGUI::newDelegate(this, &CharacterCreation::onNameDialogDone);
        nameDialog->open();
        return;
    }
    
    if(id == GM_Race)
    {
        if (raceDialog)
            wm->removeDialog(raceDialog);
        raceDialog = new RaceDialog(*wm);
        raceDialog->setNextButtonShow(creationStage >= RaceChosen);
        raceDialog->setRaceId(playerRaceId);
        raceDialog->eventDone = MyGUI::newDelegate(this, &CharacterCreation::onRaceDialogDone);
        raceDialog->eventBack = MyGUI::newDelegate(this, &CharacterCreation::onRaceDialogBack);
        raceDialog->open();
        return;
    }
    
    if(id == GM_Class)
    {
        if (classChoiceDialog)
            wm->removeDialog(classChoiceDialog);
        classChoiceDialog = new ClassChoiceDialog(*wm);
        classChoiceDialog->eventButtonSelected = MyGUI::newDelegate(this, &CharacterCreation::onClassChoice);
        classChoiceDialog->open();
        return;
    }
    
    if(id == GM_ClassPick)
    {
        if (pickClassDialog)
            wm->removeDialog(pickClassDialog);
        pickClassDialog = new PickClassDialog(*wm);
        pickClassDialog->setNextButtonShow(creationStage >= ClassChosen);
        pickClassDialog->setClassId(playerClass.name);
        pickClassDialog->eventDone = MyGUI::newDelegate(this, &CharacterCreation::onPickClassDialogDone);
        pickClassDialog->eventBack = MyGUI::newDelegate(this, &CharacterCreation::onPickClassDialogBack);
        pickClassDialog->open();
        return;
    }
}

void CharacterCreation::onPickClassDialogDone(WindowBase* parWindow)
{
    if (pickClassDialog)
    {
        const std::string &classId = pickClassDialog->getClassId();
        if (!classId.empty())
            wm->getMechanicsManager()->setPlayerClass(classId);
        const ESM::Class *klass = wm->getWorld()->getStore().classes.find(classId);
        if (klass)
            playerClass = *klass;
        wm->removeDialog(pickClassDialog);
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

void CharacterCreation::onPickClassDialogBack()
{
    if (pickClassDialog)
    {
        const std::string classId = pickClassDialog->getClassId();
        if (!classId.empty())
            wm->getMechanicsManager()->setPlayerClass(classId);
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
        wm->getMechanicsManager()->setPlayerName(playerName);
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
            wm->getMechanicsManager()->setPlayerRace(playerRaceId, raceDialog->getGender() == RaceDialog::GM_Male);
        wm->removeDialog(raceDialog);
    }

    wm->setGuiMode(GM_Name);
}

void CharacterCreation::onRaceDialogDone(WindowBase* parWindow)
{
    if (raceDialog)
    {
        playerRaceId = raceDialog->getRaceId();
        if (!playerRaceId.empty())
            wm->getMechanicsManager()->setPlayerRace(playerRaceId, raceDialog->getGender() == RaceDialog::GM_Male);
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
