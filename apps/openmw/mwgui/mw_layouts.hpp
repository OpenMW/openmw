#ifndef MWGUI_LAYOUTS_H
#define MWGUI_LAYOUTS_H

#include <openengine/gui/layout.hpp>

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

      compass->setImageTexture("compass.dds");
      crosshair->setImageTexture("target.dds");

      // These are just demo values, you should replace these with
      // real calls from outside the class later.
      setStats(60, 100,
               30, 100,
               80, 100);
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

    StatsWindow()
      : Layout("openmw_stats_window_layout.xml")
    {
      setCoord(0,0,498, 342);

      setText("Health_str", "Health");
      setText("Magicka_str", "Magicka");
      setText("Fatigue_str", "Fatigue");

      setText("Level_str", "Level");
      setText("Race_str", "Race");
      setText("Class_str", "Class");

      // These are just demo values, you should replace these with
      // real calls from outside the class later.
      setPlayerName("ThePlayer");

      setText("LevelText", "5");
      setText("RaceText", "Wood Elf");
      setText("ClassText", "Pilgrim");

      setText("AttribVal1", "30");
      setText("AttribVal2", "40");
      setText("AttribVal3", "30");
      setText("AttribVal4", "75");
      setText("AttribVal5", "50");
      setText("AttribVal6", "40");
      setText("AttribVal7", "50");
      setText("AttribVal8", "40");
    }

    void setPlayerName(const std::string& playerName)
    {
      mMainWidget->setCaption(playerName);
    }

    /// Set label text for the value with the given ID.
    void setLabel (const std::string& id, const std::string& label)
    {
        static const char *ids[] =
        {
            "Attrib1", "Attrib2", "Attrib3", "Attrib4", "Attrib5", "Attrib6",
            "Attrib7", "Attrib8",
            0
        };

        for (int i=0; ids[i]; ++i)
            if (ids[i]==id)
            {
                setText (id, label);
                break;
            }
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
  };
}
#endif
