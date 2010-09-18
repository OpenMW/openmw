#ifndef MWGUI_LAYOUTS_H
#define MWGUI_LAYOUTS_H

#include <components/esm_store/store.hpp>

#include <openengine/gui/layout.hpp>

#include <boost/array.hpp>

#include <sstream>
#include <set>
#include <string>
#include <utility>

#include "../mwmechanics/stat.hpp"
#include "../mwworld/environment.hpp"
#include "../mwworld/world.hpp"

/*
  This file contains classes corresponding to all the window layouts
  defined in resources/mygui/ *.xml.

  Each class inherites GUI::Layout and loads the XML file, and
  provides some helper functions to manipulate the elements of the
  window.

  The windows are never created or destroyed (except at startup and
  shutdown), they are only hid. You can control visibility with
  setVisible().
 */

namespace MWGui
{
  class HUD : public OEngine::GUI::Layout
  {
  public:
    HUD(int width, int height)
      : Layout("openmw_hud_layout.xml")
    {
      setCoord(0,0, width, height);

      // Energy bars
      getWidget(health, "Health");
      getWidget(magicka, "Magicka");
      getWidget(stamina, "Stamina");

      // Item and spell images and status bars
      getWidget(weapImage, "WeapImage");
      getWidget(weapStatus, "WeapStatus");
      getWidget(spellImage, "SpellImage");
      getWidget(spellStatus, "SpellStatus");

      getWidget(effectBox, "EffectBox");
      getWidget(effect1, "Effect1");

      getWidget(minimap, "MiniMap");
      getWidget(compass, "Compass");

      getWidget(crosshair, "Crosshair");

      compass->setImageTexture("textures\\compass.dds");
      crosshair->setImageTexture("textures\\target.dds");

      // These are just demo values, you should replace these with
      // real calls from outside the class later.
      setWeapIcon("icons\\w\\tx_knife_iron.dds");
      setWeapStatus(90, 100);
      setSpellIcon("icons\\s\\b_tx_s_rstor_health.dds");
      setSpellStatus(65, 100);
      setEffect("icons\\s\\tx_s_chameleon.dds");
    }

    void setStats(int h, int hmax, int m, int mmax, int s, int smax)
    {
      health->setProgressRange(hmax);
      health->setProgressPosition(h);
      magicka->setProgressRange(mmax);
      magicka->setProgressPosition(m);
      stamina->setProgressRange(smax);
      stamina->setProgressPosition(s);
    }

    void setWeapIcon(const char *str)
    { weapImage->setImageTexture(str); }
    void setSpellIcon(const char *str)
    { spellImage->setImageTexture(str); }

    void setWeapStatus(int s, int smax)
    {
      weapStatus->setProgressRange(smax);
      weapStatus->setProgressPosition(s);
    }
    void setSpellStatus(int s, int smax)
    {
      spellStatus->setProgressRange(smax);
      spellStatus->setProgressPosition(s);
    }

    void setEffect(const char *img)
    { effect1->setImageTexture(img); }

    void setValue (const std::string& id, const MWMechanics::DynamicStat<int>& value)
    {
        static const char *ids[] =
        {
            "HBar", "MBar", "FBar",
            0
        };

        for (int i=0; ids[i]; ++i)
            if (ids[i]==id)
            {
                switch (i)
                {
                    case 0:

                      health->setProgressRange (value.getModified());
                      health->setProgressPosition (value.getCurrent());
                      break;

                    case 1:

                      magicka->setProgressRange (value.getModified());
                      magicka->setProgressPosition (value.getCurrent());
                      break;

                    case 2:

                      stamina->setProgressRange (value.getModified());
                      stamina->setProgressPosition (value.getCurrent());
                      break;
                }
            }
    }

    MyGUI::ProgressPtr health, magicka, stamina;

    MyGUI::StaticImagePtr weapImage, spellImage;
    MyGUI::ProgressPtr weapStatus, spellStatus;

    MyGUI::WidgetPtr effectBox;
    MyGUI::StaticImagePtr effect1;

    MyGUI::StaticImagePtr minimap;
    MyGUI::StaticImagePtr compass;

    MyGUI::StaticImagePtr crosshair;
  };

  class MapWindow : public OEngine::GUI::Layout
  {
  public:
    MapWindow()
      : Layout("openmw_map_window_layout.xml")
    {
      setCoord(500,0,320,300);
      setText("WorldButton", "World");
      setImage("Compass", "compass.dds");

      // Obviously you should override this later on
      setCellName("No Cell Loaded");
    }

    void setCellName(const std::string& cellName)
    {
      mMainWidget->setCaption(cellName);
    }
  };

  class MainMenu : public OEngine::GUI::Layout
  {
  public:
    MainMenu(int w, int h)
      : Layout("openmw_mainmenu_layout.xml")
    {
      setCoord(0,0,w,h);
    }
  };

  class StatsWindow : public OEngine::GUI::Layout
  {
  public:
    typedef std::pair<std::string, int> Faction;
    typedef std::vector<Faction> FactionList;

    typedef std::vector<int> SkillList;

    void setBar(const std::string& name, const std::string& tname, int val, int max)
    {
      MyGUI::ProgressPtr pt;
      getWidget(pt, name);
      pt->setProgressRange(max);
      pt->setProgressPosition(val);

      std::stringstream out;
      out << val << "/" << max;
      setText(tname, out.str().c_str());
    }

    StatsWindow (MWWorld::Environment& environment)
      : Layout("openmw_stats_window_layout.xml")
      , environment(environment)
    {
      setCoord(0,0,498, 342);

        const char *names[][2] =
        {
            { "Attrib1", "sAttributeStrength" },
            { "Attrib2", "sAttributeIntelligence" },
            { "Attrib3", "sAttributeWillpower" },
            { "Attrib4", "sAttributeAgility" },
            { "Attrib5", "sAttributeSpeed" },
            { "Attrib6", "sAttributeEndurance" },
            { "Attrib7", "sAttributePersonality" },
            { "Attrib8", "sAttributeLuck" },
            { "Health_str", "sHealth" },
            { "Magicka_str", "sMagic" },
            { "Fatigue_str", "sFatigue" },
            { "Level_str", "sLevel" },
            { "Race_str", "sRace" },
            { "Class_str", "sClass" },
            { 0, 0 }
        };

        const ESMS::ESMStore &store = environment.mWorld->getStore();
        for (int i=0; names[i][0]; ++i)
        {
            setText (names[i][0], store.gameSettings.find (names[i][1])->str);
        }

        getWidget(skillAreaWidget, "Skills");

        for (int i = 0; i < ESM::Skill::Length; ++i)
        {
            skillValues.insert(std::pair<int, MWMechanics::Stat<float> >(i, MWMechanics::Stat<float>()));
            skillWidgetMap.insert(std::pair<int, MyGUI::WidgetPtr>(i, nullptr));
        }
    }

    void setPlayerName(const std::string& playerName)
    {
      mMainWidget->setCaption(playerName);
    }

    /// Set value for the given ID.
    void setValue (const std::string& id, const MWMechanics::Stat<int>& value)
    {
        static const char *ids[] =
        {
            "AttribVal1", "AttribVal2", "AttribVal3", "AttribVal4", "AttribVal5",
            "AttribVal6", "AttribVal7", "AttribVal8",
            0
        };

        for (int i=0; ids[i]; ++i)
            if (ids[i]==id)
            {
                std::ostringstream valueString;
                valueString << value.getModified();
                setText (id, valueString.str());

                if (value.getModified()>value.getBase())
                    setTextColor (id, 0, 1, 0);
                else if (value.getModified()<value.getBase())
                    setTextColor (id, 1, 0, 0);
                else
                    setTextColor (id, 1, 1, 1);

                break;
            }
    }

    void setValue (const std::string& id, const MWMechanics::DynamicStat<int>& value)
    {
        static const char *ids[] =
        {
            "HBar", "MBar", "FBar",
            0
        };

        for (int i=0; ids[i]; ++i)
            if (ids[i]==id)
            {
                std::string id (ids[i]);
                setBar (id, id + "T", value.getCurrent(), value.getModified());
            }
    }

    void setValue (const std::string& id, const std::string& value)
    {
        if (id=="name")
            setPlayerName (value);
        else if (id=="race")
            setText ("RaceText", value);
        else if (id=="class")
            setText ("ClassText", value);
    }

    void setValue (const std::string& id, int value)
    {
        if (id=="level")
        {
            std::ostringstream text;
            text << value;
            setText("LevelText", text.str());
        }
    }

    void setValue (const std::string& id, const MWMechanics::Stat<float>& value);

      void configureSkills (const SkillList& major, const SkillList& minor);
      void setFactions (const std::vector<Faction>& factions);
      void setBirthSign (const std::string &signId);
      void setReputation (int reputation) { this->reputation = reputation; }
      void setBounty (int bounty) { this->bounty = bounty; }
      void updateSkillArea();

  private:
      enum ColorStyle
      {
          CS_Sub,
          CS_Normal,
          CS_Super
      };
      void setStyledText(MyGUI::WidgetPtr widget, ColorStyle style, const std::string &value);
      void addSkills(const SkillList &skills, const std::string &titleId, const std::string &titleDefault, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2);
      void addSeparator(MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2);
      void addGroup(const std::string &label, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2);
      MyGUI::WidgetPtr addValueItem(const std::string text, const std::string &value, ColorStyle style, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2);
      void addItem(const std::string text, MyGUI::IntCoord &coord1, MyGUI::IntCoord &coord2);

      static const int lineHeight;

      MWWorld::Environment& environment;
      MyGUI::WidgetPtr skillAreaWidget;
      SkillList majorSkills, minorSkills, miscSkills;
      std::map<int, MWMechanics::Stat<float> > skillValues;
      std::map<int, MyGUI::WidgetPtr> skillWidgetMap;
      std::map<std::string, MyGUI::WidgetPtr> factionWidgetMap;
      FactionList factions; ///< Stores a list of factions and the current rank
      std::string birthSignId;
      int reputation, bounty;
      std::vector<MyGUI::WidgetPtr> skillWidgets; //< Skills and other information
  };

#if 0
  class InventoryWindow : public OEngine::GUI::Layout
  {
  public:
    enum CategoryMode
      {
        CM_All = 0,      // All items
        CM_Weapon = 1,   // Only weapons
        CM_Apparel = 2,  // Apparel
        CM_Magic = 3,    // Magic
        CM_Misc = 4      // Misc
      };

    InventoryWindow ()
      : Layout("openmw_inventory_window_layout.xml")
      , categoryMode(CM_All)

      // color should be fetched from skin
      , activeColor(0, 0, 1)
      , inactiveColor(0.7, 0.7, 0.7)
    {
      setCoord(0, 200, 600, 400);

      // These are just demo values, you should replace these with
      // real calls from outside the class later.

      mMainWidget->setCaption("Glass Frostsword");
      setText("EncumbranceBarT", "176/210");

      MyGUI::ProgressPtr pt;
      getWidget(pt, "EncumbranceBar");
      pt->setProgressRange(210);
      pt->setProgressPosition(176);

      MyGUI::WidgetPtr avatar;
      getWidget(avatar, "Avatar");

      // Adjust armor rating text to bottom of avatar widget
      MyGUI::StaticTextPtr armor_rating;
      getWidget(armor_rating, "ArmorRating");
      armor_rating->setCaption("Armor: 11");
      MyGUI::IntCoord coord = armor_rating->getCoord();
      coord.top = avatar->getCoord().height - 4 - coord.height;
      armor_rating->setCoord(coord);

      names[0] = "All";
      names[1] = "Weapon";
      names[2] = "Apparel";
      names[3] = "Magic";
      names[4] = "Misc";

      boost::array<CategoryMode, 5> categories = { {
        CM_All, CM_Weapon, CM_Apparel, CM_Magic, CM_Misc
      } };

      // Initialize buttons with text and adjust sizes, also mark All as active button
      int margin = 2;
      int last_x = 0;
      for (int i = 0; i < categories.size(); ++i)
      {
          CategoryMode mode = categories[i];
          std::string name = names[mode];
          name += "Button";
          setText(name, names[mode]);
          getWidget(buttons[mode], name);

          MyGUI::ButtonPtr &button_pt = buttons[mode];
          if (mode == CM_All)
              button_pt->setTextColour(activeColor);
          else
              button_pt->setTextColour(inactiveColor);
          MyGUI::IntCoord coord = button_pt->getCoord();
          coord.left = last_x;
          last_x += coord.width + margin;
          button_pt->setCoord(coord);

          button_pt->eventMouseButtonClick = MyGUI::newDelegate(this, &InventoryWindow::onCategorySelected);
      }
    }

    void setCategory(CategoryMode mode)
    {
        MyGUI::ButtonPtr pt = getCategoryButton(categoryMode);
        pt->setTextColour(inactiveColor);

        pt = getCategoryButton(mode);
        pt->setTextColour(activeColor);
        categoryMode = mode;
    }

    MyGUI::ButtonPtr getCategoryButton(CategoryMode mode)
    {
        return buttons[mode];
    }

    void onCategorySelected(MyGUI::Widget *widget)
    {
        boost::array<CategoryMode, 5> categories = { {
        CM_All, CM_Weapon, CM_Apparel, CM_Magic, CM_Misc
        } };

        for (int i = 0; i < categories.size(); ++i)
        {
            CategoryMode mode = categories[i];
            if (widget == buttons[mode])
            {
                setCategory(mode);
                return;
            }
        }
    }

    CategoryMode categoryMode;        // Current category filter
    MyGUI::ButtonPtr buttons[5];    // Button pointers
    std::string names[5];            // Names of category buttons

    MyGUI::Colour activeColor;
    MyGUI::Colour inactiveColor;
  };
#endif
}
#endif
