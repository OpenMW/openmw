#ifndef CHARACTER_CREATION_HPP
#define CHARACTER_CREATION_HPP

#include "window_manager.hpp"

#include "../mwmechanics/mechanicsmanager.hpp"
#include "../mwworld/world.hpp"
#include <components/esm_store/store.hpp>

namespace MWGui
{
    class WindowManager;
    class WindowBase;
    
    class TextInputDialog;
    class InfoBoxDialog;
    class RaceDialog;
    class DialogueWindow;
    class ClassChoiceDialog;
    class GenerateClassResultDialog;
    class PickClassDialog;
    class CreateClassDialog;
    class BirthDialog;
    class ReviewDialog;
    class MessageBoxManager;

    class CharacterCreation
    {
    public:
    CharacterCreation(WindowManager* _wm);
    ~CharacterCreation();
    
    //Show a dialog
    void spawnDialog(const char id);
    
    private:
    WindowManager* wm;
    
    //Dialogs
    TextInputDialog *nameDialog;
    RaceDialog *raceDialog;
    DialogueWindow *dialogueWindow;
    ClassChoiceDialog *classChoiceDialog;
    InfoBoxDialog *generateClassQuestionDialog;
    GenerateClassResultDialog *generateClassResultDialog;
    PickClassDialog *pickClassDialog;
    CreateClassDialog *createClassDialog;
    BirthDialog *birthSignDialog;
    ReviewDialog *reviewDialog;
    
    //Player data
    std::string playerName;
    std::string playerRaceId;
    ESM::Class playerClass;
    
    ////Dialog events
    //Name dialog
    void onNameDialogDone(WindowBase* parWindow);
    
    //Race dialog
    void onRaceDialogDone(WindowBase* parWindow);
    void onRaceDialogBack();
    
    //Class dialog(s)
    void onClassChoice(int _index);
    void onPickClassDialogDone(WindowBase* parWindow);
    void onPickClassDialogBack();
    
    enum CreationStageEnum
    {
        NotStarted,
        NameChosen,
        RaceChosen,
        ClassChosen,
        BirthSignChosen,
        ReviewNext
    };

    // Which state the character creating is in, controls back/next/ok buttons
    CreationStageEnum creationStage;
   };
}

#endif
