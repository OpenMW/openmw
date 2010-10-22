#include "window_manager.hpp"
#include "layouts.hpp"
#include "text_input.hpp"
#include "race.hpp"
#include "class.hpp"
#include "birth.hpp"

#include "../mwmechanics/mechanicsmanager.hpp"
#include "../mwinput/inputmanager.hpp"

#include "console.hpp"

#include <assert.h>
#include <iostream>
#include <iterator>

using namespace MWGui;

WindowManager::WindowManager(MyGUI::Gui *_gui, MWWorld::Environment& environment,
    const Compiler::Extensions& extensions, bool newGame)
  : environment(environment)
  , nameDialog(nullptr)
  , raceDialog(nullptr)
  , classChoiceDialog(nullptr)
  , generateClassQuestionDialog(nullptr)
  , generateClassResultDialog(nullptr)
  , pickClassDialog(nullptr)
  , createClassDialog(nullptr)
  , birthSignDialog(nullptr)
  , nameChosen(false)
  , raceChosen(false)
  , classChosen(false)
  , birthSignChosen(false)
  , reviewNext(false)
  , gui(_gui)
  , mode(GM_Game)
  , shown(GW_ALL)
  , allowed(newGame ? GW_None : GW_ALL)
{
  // Get size info from the Gui object
  assert(gui);
  int w = gui->getViewSize().width;
  int h = gui->getViewSize().height;

  hud = new HUD(w,h);
  menu = new MainMenu(w,h);
  map = new MapWindow();
  stats = new StatsWindow (environment);
#if 0
  inventory = new InventoryWindow ();
#endif
  console = new Console(w,h, environment, extensions);

  // The HUD is always on
  hud->setVisible(true);

  // Set up visibility
  updateVisible();
}

WindowManager::~WindowManager()
{
  delete console;
  delete hud;
  delete map;
  delete menu;
  delete stats;
#if 0
  delete inventory;
#endif

  delete nameDialog;
  delete raceDialog;
  delete classChoiceDialog;
  delete generateClassQuestionDialog;
  delete generateClassResultDialog;
  delete pickClassDialog;
  delete createClassDialog;
  delete birthSignDialog;
}

void WindowManager::updateVisible()
{
  // Start out by hiding everything except the HUD
  map->setVisible(false);
  menu->setVisible(false);
  stats->setVisible(false);
#if 0
  inventory->setVisible(false);
#endif
  console->disable();

  // Mouse is visible whenever we're not in game mode
  gui->setVisiblePointer(isGuiMode());

  // If in game mode, don't show anything.
  if(mode == GM_Game)
    {
      return;
    }

  if(mode == GM_MainMenu)
    {
      // Enable the main menu
      menu->setVisible(true);
      return;
    }

  if(mode == GM_Console)
    {
      console->enable();
      return;
    }

  if (mode == GM_Name)
  {
      if (!nameDialog)
          nameDialog = new TextInputDialog(environment, gui->getViewSize());

      std::string sName = getGameSettingString("sName", "Name");
      nameDialog->setTextLabel(sName);
      nameDialog->setNextButtonShow(nameChosen);
      nameDialog->eventDone = MyGUI::newDelegate(this, &WindowManager::onNameDialogDone);
      nameDialog->open();
      return;
  }

  if (mode == GM_Race)
  {
      if (!raceDialog)
          raceDialog = new RaceDialog(environment, gui->getViewSize());
      raceDialog->setNextButtonShow(raceChosen);
      raceDialog->eventDone = MyGUI::newDelegate(this, &WindowManager::onRaceDialogDone);
      raceDialog->eventBack = MyGUI::newDelegate(this, &WindowManager::onRaceDialogBack);
      raceDialog->open();
      return;
  }

  if (mode == GM_Class)
  {
      if (classChoiceDialog)
          delete classChoiceDialog;
      classChoiceDialog = new ClassChoiceDialog(environment);
      classChoiceDialog->eventButtonSelected = MyGUI::newDelegate(this, &WindowManager::onClassChoice);
      return;
  }

  if (mode == GM_ClassGenerate)
  {
      generateClassStep = 0;
      showClassQuestionDialog();
      return;
  }

  if (mode == GM_ClassPick)
  {
      if (!pickClassDialog)
          pickClassDialog = new PickClassDialog(environment, gui->getViewSize());
      pickClassDialog->setNextButtonShow(classChosen);
      pickClassDialog->eventDone = MyGUI::newDelegate(this, &WindowManager::onPickClassDialogDone);
      pickClassDialog->eventBack = MyGUI::newDelegate(this, &WindowManager::onPickClassDialogBack);
      pickClassDialog->open();
      return;
  }

  if (mode == GM_ClassCreate)
  {
      if (createClassDialog)
          delete createClassDialog;
      createClassDialog = new CreateClassDialog(environment, gui->getViewSize());
      createClassDialog->eventDone = MyGUI::newDelegate(this, &WindowManager::onCreateClassDialogDone);
      createClassDialog->eventBack = MyGUI::newDelegate(this, &WindowManager::onCreateClassDialogBack);
      createClassDialog->open();
      return;
  }

  if (mode == GM_Birth)
  {
      if (!birthSignDialog)
          birthSignDialog = new BirthDialog(environment, gui->getViewSize());
      birthSignDialog->setNextButtonShow(birthSignChosen);
      birthSignDialog->eventDone = MyGUI::newDelegate(this, &WindowManager::onBirthSignDialogDone);
      birthSignDialog->eventBack = MyGUI::newDelegate(this, &WindowManager::onBirthSignDialogBack);
      birthSignDialog->open();
      return;
  }

  if(mode == GM_Inventory)
    {
      // Ah, inventory mode. First, compute the effective set of
      // windows to show. This is controlled both by what windows the
      // user has opened/closed (the 'shown' variable) and by what
      // windows we are allowed to show (the 'allowed' var.)
      int eff = shown & allowed;

      // Show the windows we want
      map   -> setVisible( (eff & GW_Map) != 0 );
      stats -> setVisible( (eff & GW_Stats) != 0 );
#if 0
//      inventory -> setVisible( eff & GW_Inventory );
#endif
      return;
    }

  // Unsupported mode, switch back to game
  // Note: The call will eventually end up this method again but
  // will stop at the check if(mode == GM_Game) above.
  environment.mInputManager->setGuiMode(GM_Game);
}

void WindowManager::setValue (const std::string& id, const MWMechanics::Stat<int>& value)
{
    stats->setValue (id, value);
}

void WindowManager::setValue (const std::string& id, const MWMechanics::Stat<float>& value)
{
    stats->setValue (id, value);
}

void WindowManager::setValue (const std::string& id, const MWMechanics::DynamicStat<int>& value)
{
    stats->setValue (id, value);
    hud->setValue (id, value);
}

void WindowManager::setValue (const std::string& id, const std::string& value)
{
    stats->setValue (id, value);
}

void WindowManager::setValue (const std::string& id, int value)
{
    stats->setValue (id, value);
}

void WindowManager::configureSkills (const SkillList& major, const SkillList& minor)
{
    stats->configureSkills (major, minor);
}

void WindowManager::setFactions (const FactionList& factions)
{
    stats->setFactions (factions);
}

void WindowManager::setBirthSign (const std::string &signId)
{
    stats->setBirthSign (signId);
}

void WindowManager::setReputation (int reputation)
{
    stats->setReputation (reputation);
}

void WindowManager::setBounty (int bounty)
{
    stats->setBounty (bounty);
}

void WindowManager::updateSkillArea()
{
    stats->updateSkillArea();
}

void WindowManager::messageBox (const std::string& message, const std::vector<std::string>& buttons)
{
    std::cout << "message box: " << message << std::endl;

    if (!buttons.empty())
    {
        std::cout << "buttons: ";
        std::copy (buttons.begin(), buttons.end(), std::ostream_iterator<std::string> (std::cout, ", "));
        std::cout << std::endl;
    }
}

const std::string &WindowManager::getGameSettingString(const std::string &id, const std::string &default_)
{
    const ESM::GameSetting *setting = environment.mWorld->getStore().gameSettings.search(id);
    if (setting && setting->type == ESM::VT_String)
        return setting->str;
    return default_;
}

void WindowManager::updateCharacterGeneration()
{
    if (raceDialog)
    {
        // TOOD: Uncomment when methods in mechanics manager is implemented
        //raceDialog->setRace(environment.mMechanicsManager->getPlayerRace());
        //raceDialog->setGender(environment.mMechanicsManager->getPlayerMale() ? RaceDialog::GM_Male : RaceDialog::GM_Female);
        // TODO: Face/Hair
    }
}

void WindowManager::onNameDialogDone()
{
    nameDialog->eventDone = MWGui::TextInputDialog::EventHandle_Void();

    bool goNext = nameChosen; // Go to next dialog if name was previously chosen
    nameChosen = true;
    if (nameDialog)
    {
        nameDialog->setVisible(false);
        environment.mMechanicsManager->setPlayerName(nameDialog->getTextInput());
    }

    updateCharacterGeneration();

    if (reviewNext)
        environment.mInputManager->setGuiMode(GM_Review);
    else if (goNext)
        environment.mInputManager->setGuiMode(GM_Race);
    else
        environment.mInputManager->setGuiMode(GM_Game);
}

void WindowManager::onRaceDialogDone()
{
    raceDialog->eventDone = MWGui::RaceDialog::EventHandle_Void();

    bool goNext = raceChosen; // Go to next dialog if race was previously chosen
    raceChosen = true;
    if (raceDialog)
    {
        raceDialog->setVisible(false);
        environment.mMechanicsManager->setPlayerRace(raceDialog->getRaceId(), raceDialog->getGender() == RaceDialog::GM_Male);
    }

    updateCharacterGeneration();

    if (reviewNext)
        environment.mInputManager->setGuiMode(GM_Review);
    else if (goNext)
        environment.mInputManager->setGuiMode(GM_Class);
    else
        environment.mInputManager->setGuiMode(GM_Game);
}

void WindowManager::onRaceDialogBack()
{
    if (raceDialog)
    {
        raceDialog->setVisible(false);
        environment.mMechanicsManager->setPlayerRace(raceDialog->getRaceId(), raceDialog->getGender() == RaceDialog::GM_Male);
    }

    updateCharacterGeneration();

    environment.mInputManager->setGuiMode(GM_Name);
}

void WindowManager::onClassChoice(MyGUI::WidgetPtr, int _index)
{
    classChoiceDialog->setVisible(false);
//    classChoiceDialog = nullptr;

    if (_index == ClassChoiceDialog::Class_Generate)
    {
        environment.mInputManager->setGuiMode(GM_ClassGenerate);
    }
    else if (_index == ClassChoiceDialog::Class_Pick)
    {
        environment.mInputManager->setGuiMode(GM_ClassPick);
    }
    else if (_index == ClassChoiceDialog::Class_Create)
    {
        environment.mInputManager->setGuiMode(GM_ClassCreate);
    }
    else if (_index == ClassChoiceDialog::Class_Back)
    {
        environment.mInputManager->setGuiMode(GM_Race);
    }
}

void WindowManager::showClassQuestionDialog()
{
    struct Step
    {
        const char* text;
        const char* buttons[3];
    };
    static boost::array<Step, 2> steps = { {
        {"On a clear day you chance upon a strange animal, its legs trapped in a hunter's clawsnare. Judging from the bleeding, it will not survive long.",
         {"Use herbs from your pack to put it to sleep?",
          "Do not interfere in the natural evolution of events, but rather take the opportunity to learn more about a strange animal that you have never seen before?",
          "Draw your dagger, mercifully endings its life with a single thrust?"}
        },
        {"Your mother sends you to the market with a list of goods to buy. After you finish you find that by mistake a shopkeeper has given you too much money back in exchange for one of the items.",
         {"Return to the store and give the shopkeeper his hard-earned money, explaining to him the mistake?",
          "Pocket the extra money, knowing that shopkeepers in general tend to overcharge customers anyway?",
          "Decide to put the extra money to good use and purchase items that would help your family?"}
        },
    } };

    if (generateClassStep == steps.size())
    {
        // TODO: Calculate this in mechanics manager
        generateClass = "acrobat";

        if (generateClassResultDialog)
            delete generateClassResultDialog;
        generateClassResultDialog = new GenerateClassResultDialog(environment);
        generateClassResultDialog->setClassId(generateClass);
        generateClassResultDialog->eventBack = MyGUI::newDelegate(this, &WindowManager::onGenerateClassBack);
        generateClassResultDialog->eventDone = MyGUI::newDelegate(this, &WindowManager::onGenerateClassDone);
        generateClassResultDialog->setVisible(true);
        return;
    }

    if (generateClassStep > steps.size())
    {
        environment.mInputManager->setGuiMode(GM_Class);
        return;
    }

    if (!generateClassQuestionDialog)
        generateClassQuestionDialog = new InfoBoxDialog(environment);

    InfoBoxDialog::ButtonList buttons;
    generateClassQuestionDialog->setText(steps[generateClassStep].text);
    buttons.push_back(steps[generateClassStep].buttons[0]);
    buttons.push_back(steps[generateClassStep].buttons[1]);
    buttons.push_back(steps[generateClassStep].buttons[2]);
    generateClassQuestionDialog->setButtons(buttons);
    generateClassQuestionDialog->update();
    generateClassQuestionDialog->eventButtonSelected = MyGUI::newDelegate(this, &WindowManager::onClassQuestionChosen);
    generateClassQuestionDialog->setVisible(true);
}

void WindowManager::onClassQuestionChosen(MyGUI::Widget* _sender, int _index)
{
    generateClassQuestionDialog->setVisible(false);
    if (_index < 0 || _index >= 3)
    {
        environment.mInputManager->setGuiMode(GM_Class);
        return;
    }

    ++generateClassStep;
    showClassQuestionDialog();
}

void WindowManager::onGenerateClassBack()
{
    bool goNext = classChosen; // Go to next dialog if class was previously chosen
    classChosen = true;

    if (generateClassResultDialog)
    {
        generateClassResultDialog->setVisible(false);
    }
    environment.mMechanicsManager->setPlayerClass(generateClass);

    updateCharacterGeneration();

    environment.mInputManager->setGuiMode(GM_Class);
}

void WindowManager::onGenerateClassDone()
{
    bool goNext = classChosen; // Go to next dialog if class was previously chosen
    classChosen = true;

    if (generateClassResultDialog)
    {
        generateClassResultDialog->setVisible(false);
    }
    environment.mMechanicsManager->setPlayerClass(generateClass);

    updateCharacterGeneration();

    if (reviewNext)
        environment.mInputManager->setGuiMode(GM_Review);
    else if (goNext)
        environment.mInputManager->setGuiMode(GM_Birth);
    else
        environment.mInputManager->setGuiMode(GM_Game);
}


void WindowManager::onPickClassDialogDone()
{
    pickClassDialog->eventDone = MWGui::PickClassDialog::EventHandle_Void();

    bool goNext = classChosen; // Go to next dialog if class was previously chosen
    classChosen = true;
    if (pickClassDialog)
    {
        pickClassDialog->setVisible(false);
        environment.mMechanicsManager->setPlayerClass(pickClassDialog->getClassId());
    }

    updateCharacterGeneration();

    if (reviewNext)
        environment.mInputManager->setGuiMode(GM_Review);
    else if (goNext)
        environment.mInputManager->setGuiMode(GM_Birth);
    else
        environment.mInputManager->setGuiMode(GM_Game);
}

void WindowManager::onPickClassDialogBack()
{
    if (pickClassDialog)
    {
        pickClassDialog->setVisible(false);
        environment.mMechanicsManager->setPlayerClass(pickClassDialog->getClassId());
    }

    updateCharacterGeneration();

    environment.mInputManager->setGuiMode(GM_Class);
}

void WindowManager::onCreateClassDialogDone()
{
    createClassDialog->eventDone = MWGui::CreateClassDialog::EventHandle_Void();

    bool goNext = classChosen; // Go to next dialog if class was previously chosen
    classChosen = true;
    if (createClassDialog)
    {
        createClassDialog->setVisible(false);

        // TODO: The ESM::Class should have methods to set these values to ensure correct data is assigned
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
        environment.mMechanicsManager->setPlayerClass(klass);
    }

    updateCharacterGeneration();

    if (reviewNext)
        environment.mInputManager->setGuiMode(GM_Review);
    else if (goNext)
        environment.mInputManager->setGuiMode(GM_Birth);
    else
        environment.mInputManager->setGuiMode(GM_Game);
}

void WindowManager::onCreateClassDialogBack()
{
    if (pickClassDialog)
    {
        pickClassDialog->setVisible(false);
        environment.mMechanicsManager->setPlayerClass(pickClassDialog->getClassId());
    }

    updateCharacterGeneration();

    environment.mInputManager->setGuiMode(GM_Class);
}

void WindowManager::onBirthSignDialogDone()
{
    birthSignDialog->eventDone = MWGui::BirthDialog::EventHandle_Void();

    bool goNext = birthSignChosen; // Go to next dialog if birth sign was previously chosen
    birthSignChosen = true;
    if (birthSignDialog)
    {
        birthSignDialog->setVisible(false);
        environment.mMechanicsManager->setPlayerBirthsign(birthSignDialog->getBirthId());
    }

    updateCharacterGeneration();

    if (reviewNext || goNext)
        environment.mInputManager->setGuiMode(GM_Review);
    else
        environment.mInputManager->setGuiMode(GM_Game);
}

void WindowManager::onBirthSignDialogBack()
{
    if (birthSignDialog)
    {
        birthSignDialog->setVisible(false);
        environment.mMechanicsManager->setPlayerBirthsign(birthSignDialog->getBirthId());
    }

    updateCharacterGeneration();

    environment.mInputManager->setGuiMode(GM_Class);
}
