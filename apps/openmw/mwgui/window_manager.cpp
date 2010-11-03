#include "window_manager.hpp"
#include "layouts.hpp"
#include "text_input.hpp"
#include "race.hpp"
#include "class.hpp"
#include "birth.hpp"
#include "review.hpp"
#include "dialogue.hpp"
#include "dialogue_history.hpp"

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
  , dialogueWindow(nullptr)
  , classChoiceDialog(nullptr)
  , generateClassQuestionDialog(nullptr)
  , generateClassResultDialog(nullptr)
  , pickClassDialog(nullptr)
  , createClassDialog(nullptr)
  , birthSignDialog(nullptr)
  , reviewDialog(nullptr)
  , nameChosen(false)
  , raceChosen(false)
  , classChosen(false)
  , birthSignChosen(false)
  , reviewNext(false)
  , gui(_gui)
  , mode(GM_Game)
  , nextMode(GM_Game)
  , needModeChange(false)
  , shown(GW_ALL)
  , allowed(newGame ? GW_None : GW_ALL)
{

    //Register own widgets with MyGUI
    MyGUI::FactoryManager::getInstance().registerFactory<DialogeHistory>("Widget");

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

  // Setup player stats
  for (int i = 0; i < ESM::Attribute::Length; ++i)
  {
      playerAttributes.insert(std::make_pair(ESM::Attribute::attributeIds[i], MWMechanics::Stat<int>()));
  }

  for (int i = 0; i < ESM::Skill::Length; ++i)
  {
      playerSkillValues.insert(std::make_pair(ESM::Skill::skillIds[i], MWMechanics::Stat<float>()));
  }

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
  delete dialogueWindow;
  delete classChoiceDialog;
  delete generateClassQuestionDialog;
  delete generateClassResultDialog;
  delete pickClassDialog;
  delete createClassDialog;
  delete birthSignDialog;
  delete reviewDialog;

  cleanupGarbage();
}

void WindowManager::cleanupGarbage()
{
    // Delete any dialogs which are no longer in use
    if (!garbageDialogs.empty())
    {
        for (std::vector<OEngine::GUI::Layout*>::iterator it = garbageDialogs.begin(); it != garbageDialogs.end(); ++it)
        {
            delete *it;
        }
        garbageDialogs.clear();
    }
}

void WindowManager::update()
{
    cleanupGarbage();
    if (needModeChange)
    {
        needModeChange = false;
        environment.mInputManager->setGuiMode(nextMode);
        nextMode = GM_Game;
    }
}

void WindowManager::setNextMode(GuiMode newMode)
{
    nextMode = newMode;
    needModeChange = true;
}

void WindowManager::setGuiMode(GuiMode newMode)
{
    environment.mInputManager->setGuiMode(newMode);
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
      if (nameDialog)
          removeDialog(nameDialog);
      nameDialog = new TextInputDialog(environment);
      std::string sName = getGameSettingString("sName", "Name");
      nameDialog->setTextLabel(sName);
      nameDialog->setTextInput(playerName);
      nameDialog->setNextButtonShow(nameChosen || reviewNext);
      nameDialog->eventDone = MyGUI::newDelegate(this, &WindowManager::onNameDialogDone);
      nameDialog->open();
      return;
  }

  if (mode == GM_Race)
  {
      if (raceDialog)
          removeDialog(raceDialog);
      raceDialog = new RaceDialog(environment);
      raceDialog->setNextButtonShow(raceChosen || reviewNext);
      raceDialog->setRaceId(playerRaceId);
      raceDialog->eventDone = MyGUI::newDelegate(this, &WindowManager::onRaceDialogDone);
      raceDialog->eventBack = MyGUI::newDelegate(this, &WindowManager::onRaceDialogBack);
      raceDialog->open();
      return;
  }

  if (mode == GM_Class)
  {
      if (classChoiceDialog)
          removeDialog(classChoiceDialog);
      classChoiceDialog = new ClassChoiceDialog(environment);
      classChoiceDialog->eventButtonSelected = MyGUI::newDelegate(this, &WindowManager::onClassChoice);
      classChoiceDialog->open();
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
      if (pickClassDialog)
          removeDialog(pickClassDialog);
      pickClassDialog = new PickClassDialog(environment);
      pickClassDialog->setNextButtonShow(classChosen || reviewNext);
      pickClassDialog->setClassId(playerClass.name);
      pickClassDialog->eventDone = MyGUI::newDelegate(this, &WindowManager::onPickClassDialogDone);
      pickClassDialog->eventBack = MyGUI::newDelegate(this, &WindowManager::onPickClassDialogBack);
      pickClassDialog->open();
      return;
  }

  if (mode == GM_ClassCreate)
  {
      if (createClassDialog)
          removeDialog(createClassDialog);
      createClassDialog = new CreateClassDialog(environment);
      createClassDialog->eventDone = MyGUI::newDelegate(this, &WindowManager::onCreateClassDialogDone);
      createClassDialog->eventBack = MyGUI::newDelegate(this, &WindowManager::onCreateClassDialogBack);
      createClassDialog->open();
      return;
  }

  if (mode == GM_Birth)
  {
      if (birthSignDialog)
          removeDialog(birthSignDialog);
      birthSignDialog = new BirthDialog(environment);
      birthSignDialog->setNextButtonShow(birthSignChosen || reviewNext);
      birthSignDialog->setBirthId(playerBirthSignId);
      birthSignDialog->eventDone = MyGUI::newDelegate(this, &WindowManager::onBirthSignDialogDone);
      birthSignDialog->eventBack = MyGUI::newDelegate(this, &WindowManager::onBirthSignDialogBack);
      birthSignDialog->open();
      return;
  }

  if (mode == GM_Review)
  {
      reviewNext = false;
      if (reviewDialog)
          removeDialog(reviewDialog);
      reviewDialog = new ReviewDialog(environment);
      reviewDialog->setPlayerName(playerName);
      reviewDialog->setRace(playerRaceId);
      reviewDialog->setClass(playerClass);
      reviewDialog->setBirthSign(playerBirthSignId);

      reviewDialog->setHealth(playerHealth);
      reviewDialog->setMagicka(playerMagicka);
      reviewDialog->setFatigue(playerFatigue);

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

      reviewDialog->eventDone = MyGUI::newDelegate(this, &WindowManager::onReviewDialogDone);
      reviewDialog->eventBack = MyGUI::newDelegate(this, &WindowManager::onReviewDialogBack);

      reviewDialog->eventNameActivated = MyGUI::newDelegate(this, &WindowManager::onNameDialogActivate);
      reviewDialog->eventRaceActivated = MyGUI::newDelegate(this, &WindowManager::onRaceDialogActivate);
      reviewDialog->eventClassActivated = MyGUI::newDelegate(this, &WindowManager::onClassDialogActivate);
      reviewDialog->eventBirthSignActivated = MyGUI::newDelegate(this, &WindowManager::onBirthSignDialogActivate);

      reviewDialog->open();
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

  if (mode == GM_Dialogue)
  {
      if (dialogueWindow)
          removeDialog(dialogueWindow);
      dialogueWindow = new DialogueWindow(environment);
      dialogueWindow->eventBye = MyGUI::newDelegate(this, &WindowManager::onDialogueWindowBye);
      dialogueWindow->open();
      return;
  }


  // Unsupported mode, switch back to game
  // Note: The call will eventually end up this method again but
  // will stop at the check if(mode == GM_Game) above.
  setGuiMode(GM_Game);
}

void WindowManager::setValue (const std::string& id, const MWMechanics::Stat<int>& value)
{
    stats->setValue (id, value);

    static const char *ids[] =
    {
        "AttribVal1", "AttribVal2", "AttribVal3", "AttribVal4", "AttribVal5",
        "AttribVal6", "AttribVal7", "AttribVal8"
    };
    static ESM::Attribute::AttributeID attributes[] =
    {
        ESM::Attribute::Strength,
        ESM::Attribute::Intelligence,
        ESM::Attribute::Willpower,
        ESM::Attribute::Agility,
        ESM::Attribute::Speed,
        ESM::Attribute::Endurance,
        ESM::Attribute::Personality,
        ESM::Attribute::Luck
    };
    for (size_t i = 0; i < sizeof(ids)/sizeof(ids[0]); ++i)
    {
        if (id != ids[i])
            continue;
        playerAttributes[attributes[i]] = value;
        break;
    }
}

void WindowManager::setValue (const std::string& id, const MWMechanics::Stat<float>& value)
{
    stats->setValue (id, value);

    static struct {const char *id; ESM::Skill::SkillEnum skillId; } skillMap[] =
    {
        {"SkillBlock", ESM::Skill::Block},
        {"SkillArmorer", ESM::Skill::Armorer},
        {"SkillMediumArmor", ESM::Skill::MediumArmor},
        {"SkillHeavyArmor", ESM::Skill::HeavyArmor},
        {"SkillBluntWeapon", ESM::Skill::BluntWeapon},
        {"SkillLongBlade", ESM::Skill::LongBlade},
        {"SkillAxe", ESM::Skill::Axe},
        {"SkillSpear", ESM::Skill::Spear},
        {"SkillAthletics", ESM::Skill::Athletics},
        {"SkillEnchant", ESM::Skill::Armorer},
        {"SkillDestruction", ESM::Skill::Destruction},
        {"SkillAlteration", ESM::Skill::Alteration},
        {"SkillIllusion", ESM::Skill::Illusion},
        {"SkillConjuration", ESM::Skill::Conjuration},
        {"SkillMysticism", ESM::Skill::Mysticism},
        {"SkillRestoration", ESM::Skill::Restoration},
        {"SkillAlchemy", ESM::Skill::Alchemy},
        {"SkillUnarmored", ESM::Skill::Unarmored},
        {"SkillSecurity", ESM::Skill::Security},
        {"SkillSneak", ESM::Skill::Sneak},
        {"SkillAcrobatics", ESM::Skill::Acrobatics},
        {"SkillLightArmor", ESM::Skill::LightArmor},
        {"SkillShortBlade", ESM::Skill::ShortBlade},
        {"SkillMarksman", ESM::Skill::Marksman},
        {"SkillMercantile", ESM::Skill::Mercantile},
        {"SkillSpeechcraft", ESM::Skill::Speechcraft},
        {"SkillHandToHand", ESM::Skill::HandToHand},
    };
    for (size_t i = 0; i < sizeof(skillMap)/sizeof(skillMap[0]); ++i)
    {
        if (skillMap[i].id == id)
        {
            ESM::Skill::SkillEnum skillId = skillMap[i].skillId;
            playerSkillValues[skillId] = value;
            break;
        }
    }
}

void WindowManager::setValue (const std::string& id, const MWMechanics::DynamicStat<int>& value)
{
    stats->setValue (id, value);
    hud->setValue (id, value);
    if (id == "HBar")
        playerHealth = value;
    else if (id == "MBar")
        playerMagicka = value;
    else if (id == "FBar")
        playerFatigue = value;
}

void WindowManager::setValue (const std::string& id, const std::string& value)
{
    stats->setValue (id, value);
    if (id=="name")
        playerName = value;
    else if (id=="race")
        playerRaceId = value;
}

void WindowManager::setValue (const std::string& id, int value)
{
    stats->setValue (id, value);
}

void WindowManager::setPlayerClass (const ESM::Class &class_)
{
    playerClass = class_;
    stats->setValue("class", playerClass.name);
}

void WindowManager::configureSkills (const SkillList& major, const SkillList& minor)
{
    stats->configureSkills (major, minor);
    playerMajorSkills = major;
    playerMinorSkills = minor;
}

void WindowManager::setFactions (const FactionList& factions)
{
    stats->setFactions (factions);
}

void WindowManager::setBirthSign (const std::string &signId)
{
    stats->setBirthSign (signId);
    playerBirthSignId = signId;
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

void WindowManager::removeDialog(OEngine::GUI::Layout*dialog)
{
    assert(dialog);
    if (!dialog)
        return;
    dialog->setVisible(false);
    garbageDialogs.push_back(dialog);
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

void WindowManager::onNameDialogDone()
{
    if (nameDialog)
    {
        playerName = nameDialog->getTextInput();
        environment.mMechanicsManager->setPlayerName(playerName);
        removeDialog(nameDialog);
    }

    bool goNext = nameChosen; // Go to next dialog if name was previously chosen
    nameChosen = true;

    if (reviewNext)
        setGuiMode(GM_Review);
    else if (goNext)
        setGuiMode(GM_Race);
    else
        setGuiMode(GM_Game);
}

void WindowManager::onRaceDialogDone()
{
    if (raceDialog)
    {
        playerRaceId = raceDialog->getRaceId();
        if (!playerRaceId.empty())
            environment.mMechanicsManager->setPlayerRace(playerRaceId, raceDialog->getGender() == RaceDialog::GM_Male);
        removeDialog(raceDialog);
    }

    bool goNext = raceChosen; // Go to next dialog if race was previously chosen
    raceChosen = true;

    if (reviewNext)
        setGuiMode(GM_Review);
    else if (goNext)
        setGuiMode(GM_Class);
    else
        setGuiMode(GM_Game);
}

void WindowManager::onDialogueWindowBye()
{
    if (dialogueWindow)
    {
        //FIXME set some state and stuff?
        removeDialog(dialogueWindow);
    }
    setGuiMode(GM_Game);
}

void WindowManager::onRaceDialogBack()
{
    if (raceDialog)
    {
        playerRaceId = raceDialog->getRaceId();
        if (!playerRaceId.empty())
            environment.mMechanicsManager->setPlayerRace(playerRaceId, raceDialog->getGender() == RaceDialog::GM_Male);
        removeDialog(raceDialog);
    }

    setGuiMode(GM_Name);
}

void WindowManager::onClassChoice(MyGUI::WidgetPtr, int _index)
{
    if (classChoiceDialog)
    {
        removeDialog(classChoiceDialog);
    }

    if (_index == ClassChoiceDialog::Class_Generate)
    {
        setGuiMode(GM_ClassGenerate);
    }
    else if (_index == ClassChoiceDialog::Class_Pick)
    {
        setGuiMode(GM_ClassPick);
    }
    else if (_index == ClassChoiceDialog::Class_Create)
    {
        setGuiMode(GM_ClassCreate);
    }
    else if (_index == ClassChoiceDialog::Class_Back)
    {
        setGuiMode(GM_Race);
    }
}

namespace MWGui
{

    struct Step
    {
        const char* text;
        const char* buttons[3];
    };

}

void WindowManager::showClassQuestionDialog()
{
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
            removeDialog(generateClassResultDialog);
        generateClassResultDialog = new GenerateClassResultDialog(environment);
        generateClassResultDialog->setClassId(generateClass);
        generateClassResultDialog->eventBack = MyGUI::newDelegate(this, &WindowManager::onGenerateClassBack);
        generateClassResultDialog->eventDone = MyGUI::newDelegate(this, &WindowManager::onGenerateClassDone);
        generateClassResultDialog->open();
        return;
    }

    if (generateClassStep > steps.size())
    {
        setGuiMode(GM_Class);
        return;
    }

    if (generateClassQuestionDialog)
        removeDialog(generateClassQuestionDialog);
    generateClassQuestionDialog = new InfoBoxDialog(environment);

    InfoBoxDialog::ButtonList buttons;
    generateClassQuestionDialog->setText(steps[generateClassStep].text);
    buttons.push_back(steps[generateClassStep].buttons[0]);
    buttons.push_back(steps[generateClassStep].buttons[1]);
    buttons.push_back(steps[generateClassStep].buttons[2]);
    generateClassQuestionDialog->setButtons(buttons);
    generateClassQuestionDialog->eventButtonSelected = MyGUI::newDelegate(this, &WindowManager::onClassQuestionChosen);
    generateClassQuestionDialog->open();
}

void WindowManager::onClassQuestionChosen(MyGUI::Widget* _sender, int _index)
{
    if (generateClassQuestionDialog)
        removeDialog(generateClassQuestionDialog);
    if (_index < 0 || _index >= 3)
    {
        setGuiMode(GM_Class);
        return;
    }

    ++generateClassStep;
    showClassQuestionDialog();
}

void WindowManager::onGenerateClassBack()
{
    classChosen = true;

    if (generateClassResultDialog)
        removeDialog(generateClassResultDialog);
    environment.mMechanicsManager->setPlayerClass(generateClass);

    setGuiMode(GM_Class);
}

void WindowManager::onGenerateClassDone()
{
    bool goNext = classChosen; // Go to next dialog if class was previously chosen
    classChosen = true;

    if (generateClassResultDialog)
        removeDialog(generateClassResultDialog);
    environment.mMechanicsManager->setPlayerClass(generateClass);

    if (reviewNext)
        setGuiMode(GM_Review);
    else if (goNext)
        setGuiMode(GM_Birth);
    else
        setGuiMode(GM_Game);
}


void WindowManager::onPickClassDialogDone()
{
    if (pickClassDialog)
    {
        const std::string &classId = pickClassDialog->getClassId();
        if (!classId.empty())
            environment.mMechanicsManager->setPlayerClass(classId);
        const ESM::Class *klass = environment.mWorld->getStore().classes.find(classId);
        if (klass)
            playerClass = *klass;
        removeDialog(pickClassDialog);
    }

    bool goNext = classChosen; // Go to next dialog if class was previously chosen
    classChosen = true;

    if (reviewNext)
        setGuiMode(GM_Review);
    else if (goNext)
        setGuiMode(GM_Birth);
    else
        setGuiMode(GM_Game);
}

void WindowManager::onPickClassDialogBack()
{
    if (pickClassDialog)
    {
        const std::string classId = pickClassDialog->getClassId();
        if (!classId.empty())
            environment.mMechanicsManager->setPlayerClass(classId);
        removeDialog(pickClassDialog);
    }

    setGuiMode(GM_Class);
}

void WindowManager::onCreateClassDialogDone()
{
    if (createClassDialog)
    {
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
        playerClass = klass;

        removeDialog(createClassDialog);
    }

    bool goNext = classChosen; // Go to next dialog if class was previously chosen
    classChosen = true;

    if (reviewNext)
        setGuiMode(GM_Review);
    else if (goNext)
        setGuiMode(GM_Birth);
    else
        setGuiMode(GM_Game);
}

void WindowManager::onCreateClassDialogBack()
{
    if (createClassDialog)
        removeDialog(createClassDialog);

    setGuiMode(GM_Class);
}

void WindowManager::onBirthSignDialogDone()
{
    if (birthSignDialog)
    {
        playerBirthSignId = birthSignDialog->getBirthId();
        if (!playerBirthSignId.empty())
            environment.mMechanicsManager->setPlayerBirthsign(playerBirthSignId);
        removeDialog(birthSignDialog);
    }

    bool goNext = birthSignChosen; // Go to next dialog if birth sign was previously chosen
    birthSignChosen = true;

    if (reviewNext || goNext)
        setGuiMode(GM_Review);
    else
        setGuiMode(GM_Game);
}

void WindowManager::onBirthSignDialogBack()
{
    if (birthSignDialog)
    {
        environment.mMechanicsManager->setPlayerBirthsign(birthSignDialog->getBirthId());
        removeDialog(birthSignDialog);
    }

    setGuiMode(GM_Class);
}

void WindowManager::onReviewDialogDone()
{
    if (reviewDialog)
        removeDialog(reviewDialog);

    setGuiMode(GM_Game);
}

void WindowManager::onReviewDialogBack()
{
    if (reviewDialog)
        removeDialog(reviewDialog);

    setGuiMode(GM_Birth);
}

void WindowManager::onNameDialogActivate()
{
    if (reviewDialog)
        removeDialog(reviewDialog);

    reviewNext = true;
    setGuiMode(GM_Name);
}

void WindowManager::onRaceDialogActivate()
{
    if (reviewDialog)
        removeDialog(reviewDialog);

    reviewNext = true;
    setGuiMode(GM_Race);
}

void WindowManager::onClassDialogActivate()
{
    if (reviewDialog)
        removeDialog(reviewDialog);

    reviewNext = true;
    setGuiMode(GM_Class);
}

void WindowManager::onBirthSignDialogActivate()
{
    if (reviewDialog)
        removeDialog(reviewDialog);

    reviewNext = true;
    setGuiMode(GM_Birth);
}
