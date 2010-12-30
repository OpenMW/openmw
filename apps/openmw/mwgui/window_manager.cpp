#include "window_manager.hpp"
#include "layouts.hpp"
#include "text_input.hpp"
#include "race.hpp"
#include "class.hpp"
#include "birth.hpp"
#include "review.hpp"

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
      generateClass = "";
      generateClassSpecializations[0] = 0;
      generateClassSpecializations[1] = 0;
      generateClassSpecializations[2] = 0;
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
    for (int i = 0; i < sizeof(ids)/sizeof(ids[0]); ++i)
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
    for (int i = 0; i < sizeof(skillMap)/sizeof(skillMap[0]); ++i)
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
        // The specialization for each answer
        ESM::Class::Specialization specializations[3];
    };

    static boost::array<Step, 10> generateClassSteps = { {
        // Question 1
        {"On a clear day you chance upon a strange animal, its legs trapped in a hunter's clawsnare. Judging from the bleeding, it will not survive long.",
         {"Draw your dagger, mercifully endings its life with a single thrust.",
          "Use herbs from your pack to put it to sleep.",
          "Do not interfere in the natural evolution of events, but rather take the opportunity to learn more about a strange animal that you have never seen before."},
         {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 2
        {"One Summer afternoon your father gives you a choice of chores.",
         {"Work in the forge with him casting iron for a new plow.",
          "Gather herbs for your mother who is preparing dinner.",
          "Go catch fish at the stream using a net and line."},
         {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 3
        {"Your cousin has given you a very embarrassing nickname and, even worse, likes to call you it in front of your friends. You asked him to stop, but he finds it very amusing to watch you blush.",
         {"Beat up your cousin, then tell him that if he ever calls you that nickname again, you will bloody him worse than this time.",
          "Make up a story that makes your nickname a badge of honor instead of something humiliating.",
          "Make up an even more embarrassing nickname for him and use it constantly until he learns his lesson."},
         {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 4
        {"There is a lot of heated discussion at the local tavern over a grouped of people called 'Telepaths'. They have been hired by certain City-State kings. Rumor has it these Telepaths read a person's mind and tell their lord whether a follower is telling the truth or not.",
         {"This is a terrible practice. A person's thoughts are his own and no one, not even a king, has the right to make such an invasion into another human's mind.",
          "Loyal followers to the king have nothing to fear from a Telepath. It is important to have a method of finding assassins and spies before it is too late.",
          "In these times, it is a necessary evil. Although you do not necessarily like the idea, a Telepath could have certain advantages during a time of war or in finding someone innocent of a crime."},
         {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 5
        {"Your mother sends you to the market with a list of goods to buy. After you finish you find that by mistake a shopkeeper has given you too much money back in exchange for one of the items.",
         {"Return to the store and give the shopkeeper his hard-earned money, explaining to him the mistake?",
          "Decide to put the extra money to good use and purchase items that would help your family?",
          "Pocket the extra money, knowing that shopkeepers in general tend to overcharge customers anyway?"},
         {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 6
        {"While in the market place you witness a thief cut a purse from a noble. Even as he does so, the noble notices and calls for the city guards. In his haste to get away, the thief drops the purse near you. Surprisingly no one seems to notice the bag of coins at your feet.",
         {"Pick up the bag and signal to the guard, knowing that the only honorable thing to do is return the money to its rightful owner.",
          "Leave the bag there, knowing that it is better not to get involved.",
          "Pick up the bag and pocket it, knowing that the extra windfall will help your family in times of trouble."},
         {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 7
        {"Your father sends you on a task which you loathe, cleaning the stables. On the way there, pitchfork in hand, you run into your friend from the homestead near your own. He offers to do it for you, in return for a future favor of his choosing.",
         {"Decline his offer, knowing that your father expects you to do the work, and it is better not to be in debt.",
          "Ask him to help you, knowing that two people can do the job faster than one, and agree to help him with one task of his choosing in the future.",
          "Accept his offer, reasoning that as long as the stables are cleaned, it matters not who does the cleaning."},
         {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 8
        {"Your mother asks you to help fix the stove. While you are working, a very hot pipe slips its mooring and falls towards her.",
         {"Position yourself between the pipe and your mother.",
          "Grab the hot pipe and try to push it away.",
          "Push your mother out of the way."},
         {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 9
        {"While in town the baker gives you a sweetroll. Delighted, you take it into an alley to enjoy only to be intercepted by a gang of three other kids your age. The leader demands the sweetroll, or else he and his friends will beat you and take it.",
         {"Drop the sweetroll and step on it, then get ready for the fight.",
          "Give him the sweetroll now without argument, knowing that later this afternoon you will have all your friends with you and can come and take whatever he owes you.",
          "Act like you're going to give him the sweetroll, but at the last minute throw it in the air, hoping that they'll pay attention to it long enough for you to get a shot in on the leader."},
         {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        },
        // Question 10
        {"Entering town you find that you are witness to a very well-dressed man running from a crowd. He screams to you for help. The crowd behind him seem very angry.",
         {"Rush to the town's aid immediately, despite your lack of knowledge of the circumstances.",
          "Stand aside and allow the man and the mob to pass, realizing it is probably best not to get involved.",
          "Rush to the man's aid immediately, despite your lack of knowledge of the circumstances."},
         {ESM::Class::Combat, ESM::Class::Magic, ESM::Class::Stealth}
        }
    } };
}

namespace MWGui
{

    struct ClassPoint
    {
        const char *id;
        // Specialization points to match, in order: Stealth, Combat, Magic
        // Note: Order is taken from http://www.uesp.net/wiki/Morrowind:Class_Quiz
        int points[3];
    };
}

void WindowManager::showClassQuestionDialog()
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
                std::cerr
                    << "Failed to deduce class from chosen answers in generate class dialog"
                    << std::endl;
                generateClass = "Thief";
            }
        }

        if (generateClassResultDialog)
            removeDialog(generateClassResultDialog);
        generateClassResultDialog = new GenerateClassResultDialog(environment);
        generateClassResultDialog->setClassId(generateClass);
        generateClassResultDialog->eventBack = MyGUI::newDelegate(this, &WindowManager::onGenerateClassBack);
        generateClassResultDialog->eventDone = MyGUI::newDelegate(this, &WindowManager::onGenerateClassDone);
        generateClassResultDialog->open();
        return;
    }

    if (generateClassStep > generateClassSteps.size())
    {
        setGuiMode(GM_Class);
        return;
    }

    if (generateClassQuestionDialog)
        removeDialog(generateClassQuestionDialog);
    generateClassQuestionDialog = new InfoBoxDialog(environment);

    InfoBoxDialog::ButtonList buttons;
    generateClassQuestionDialog->setText(generateClassSteps[generateClassStep].text);
    buttons.push_back(generateClassSteps[generateClassStep].buttons[0]);
    buttons.push_back(generateClassSteps[generateClassStep].buttons[1]);
    buttons.push_back(generateClassSteps[generateClassStep].buttons[2]);
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

void WindowManager::onGenerateClassBack()
{
    bool goNext = classChosen; // Go to next dialog if class was previously chosen
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
