#ifndef MWGUI_LAYOUTS_H
#define MWGUI_LAYOUTS_H

#include <components/esm_store/store.hpp>

#include <boost/array.hpp>
#include <boost/lexical_cast.hpp>

#include <sstream>
#include <set>
#include <string>
#include <utility>

#include "../mwmechanics/stat.hpp"
#include "window_base.hpp"

#include <cmath>

/*
  This file contains classes corresponding to window layouts
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
  class LocalMapBase
  {
  public:
    LocalMapBase();
    void init(MyGUI::ScrollView* widget, OEngine::GUI::Layout* layout);

    void setCellPrefix(const std::string& prefix);
    void setActiveCell(const int x, const int y, bool interior=false);

  protected:
    int mCurX, mCurY;
    bool mInterior;
    MyGUI::ScrollView* mLocalMap;
    std::string mPrefix;
    bool mChanged;

    OEngine::GUI::Layout* mLayout;
  };
  
  class HUD : public OEngine::GUI::Layout, public LocalMapBase
  {
  public:
    HUD(int width, int height, int fpsLevel);
    void setStats(int h, int hmax, int m, int mmax, int s, int smax);
    void setWeapIcon(const char *str);
    void setSpellIcon(const char *str);
    void setWeapStatus(int s, int smax);
    void setSpellStatus(int s, int smax);
    void setEffect(const char *img);
    void setValue (const std::string& id, const MWMechanics::DynamicStat<int>& value);
    void setFPS(float fps);
    void setTriangleCount(size_t count);
    void setBatchCount(size_t count);
    void setPlayerDir(const float x, const float y);
    void setPlayerPos(const float x, const float y);

    MyGUI::ProgressPtr health, magicka, stamina;
    MyGUI::ImageBox *weapImage, *spellImage;
    MyGUI::ProgressPtr weapStatus, spellStatus;
    MyGUI::WidgetPtr effectBox;
    MyGUI::ImageBox* effect1;
    MyGUI::ScrollView* minimap;
    MyGUI::ImageBox* compass;
    MyGUI::ImageBox* crosshair;

    MyGUI::WidgetPtr fpsbox;
    MyGUI::TextBox* fpscounter;
    MyGUI::TextBox* trianglecounter;
    MyGUI::TextBox* batchcounter;
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
      MyGUI::TextBox* armor_rating;
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

          button_pt->eventMouseButtonClick += MyGUI::newDelegate(this, &InventoryWindow::onCategorySelected);
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
