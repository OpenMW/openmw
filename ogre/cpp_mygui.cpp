MyGUI::WidgetPtr FPSText;

MyGUI::WindowPtr mwindow;

OIS::MouseState state;

extern "C" void gui_toggleGui()
{
  if(guiMode == 1)
    {
      guiMode = 0;
      mGUI->hidePointer();
      if(mwindow)
        mwindow->setVisible(false);
      state = mMouse->getMouseState();
    }
  else
    {
      // Restore the GUI mouse position. This is a hack because silly
      // OIS doesn't allow us to set the mouse position ourselves.
      *((OIS::MouseState*)&(mMouse->getMouseState())) = state;
      mGUI->injectMouseMove(state.X.abs, state.Y.abs, 0);

      guiMode = 1;
      mGUI->showPointer();
      if(mwindow)
        mwindow->setVisible(true);
    }
}

void turnGuiOff(MyGUI::WidgetPtr sender)
{
  guiMode = 1;
  gui_toggleGui();
}

// Copied from MyGUI demo code, with a few modifications
class Layout
{
public:
  Layout(const std::string & _layout, MyGUI::WidgetPtr _parent = nullptr)
    : mMainWidget(nullptr)
  {
    initialise(_layout, _parent);
  }

  template <typename T>
  void getWidget(T * & _widget, const std::string & _name, bool _throw = true)
  {
    _widget = nullptr;
    for (MyGUI::VectorWidgetPtr::iterator iter=mListWindowRoot.begin();
         iter!=mListWindowRoot.end(); ++iter)
      {
        MyGUI::WidgetPtr find = (*iter)->findWidget(mPrefix + _name);
        if (nullptr != find)
          {
            T * cast = find->castType<T>(false);
            if (nullptr != cast)
              _widget = cast;
            else if (_throw)
              {
                MYGUI_EXCEPT("Error cast : dest type = '" << T::getClassTypeName()
                       << "' source name = '" << find->getName()
                       << "' source type = '" << find->getTypeName() << "' in layout '" << mLayoutName << "'");
              }
            return;
          }
      }
    MYGUI_ASSERT( ! _throw, "widget name '" << _name << "' in layout '" << mLayoutName << "' not found.");
  }

  void initialise(const std::string & _layout,
                  MyGUI::WidgetPtr _parent = nullptr)
  {
    const std::string MAIN_WINDOW = "_Main";
    mLayoutName = _layout;

    if (mLayoutName.empty())
      mMainWidget = _parent;
    else
      {
        mPrefix = MyGUI::utility::toString(this, "_");
        mListWindowRoot = MyGUI::LayoutManager::getInstance().loadLayout(mLayoutName, mPrefix, _parent);

      const std::string main_name = mPrefix + MAIN_WINDOW;
      for (MyGUI::VectorWidgetPtr::iterator iter=mListWindowRoot.begin(); iter!=mListWindowRoot.end(); ++iter) {
        if ((*iter)->getName() == main_name)
          {
            mMainWidget = (*iter);
            break;
          }
      }
      MYGUI_ASSERT(mMainWidget, "root widget name '" << MAIN_WINDOW << "' in layout '" << mLayoutName << "' not found.");
    }
  }

  void shutdown()
  {
    for (VectorBasePtr::iterator iter=mListBase.begin(); iter!=mListBase.end(); ++iter) {
      delete (*iter);
    }
    mListBase.clear();

    MyGUI::LayoutManager::getInstance().unloadLayout(mListWindowRoot);
    mListWindowRoot.clear();
  }

  void setCoord(int x, int y, int w, int h)
  {
    mMainWidget->setCoord(x,y,w,h);
  }

  virtual ~Layout()
  {
    shutdown();
  }

protected:

  MyGUI::WidgetPtr mMainWidget;

  std::string mPrefix;
  std::string mLayoutName;
  MyGUI::VectorWidgetPtr mListWindowRoot;
  typedef std::vector<Layout*> VectorBasePtr;
  VectorBasePtr mListBase;
};

class HUD : public Layout
{
public:
  HUD()
    : Layout("openmw_hud_layout.xml")
  {
    setCoord(0,0,
             mWindow->getWidth(),
             mWindow->getHeight());

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

    compass->setImageTexture("compass.dds");
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

  MyGUI::ProgressPtr health, magicka, stamina;

  MyGUI::StaticImagePtr weapImage, spellImage;
  MyGUI::ProgressPtr weapStatus, spellStatus;

  MyGUI::WidgetPtr effectBox;
  MyGUI::StaticImagePtr effect1;

  MyGUI::StaticImagePtr minimap;
  MyGUI::StaticImagePtr compass;
};

HUD *hud;

extern "C" void gui_setupGUI()
{
  ResourceGroupManager::getSingleton().
    addResourceLocation("media_mygui", "FileSystem", "General");

  mGUI = new MyGUI::Gui();
  mGUI->initialise(mWindow);

  int mWidth = mWindow->getWidth();
  int mHeight = mWindow->getHeight();
  int width = 120;
  int height = 30;

  // FPS Ticker
  FPSText = mGUI->createWidget<MyGUI::Widget>
    ("StaticText",
     mWidth - width -10, 10, // Position
     width, height, // Size
     MyGUI::ALIGN_RIGHT | MyGUI::ALIGN_TOP,
     "Statistic");
  FPSText->setTextAlign(MyGUI::ALIGN_RIGHT);
  FPSText->setNeedMouseFocus(false);
  FPSText->setTextColour(Ogre::ColourValue::White);

  // Window with Morrowind skin
  mwindow = mGUI->createWidget<MyGUI::Window>
    ("MW_Window",
     (mWidth-width)/4, (mHeight-height)/4, // Position
     300, 190, // Size
     MyGUI::ALIGN_DEFAULT, "Windows");
  mwindow->setCaption("Skin test");
  mwindow->setMinSize(120, 140);
  mwindow->getClientWidget()->setAlpha(0.6);

  MyGUI::WidgetPtr tmp;
  tmp = mwindow->createWidget<MyGUI::Button>
    ("MW_Button",
     10, 32, // Position
     45, 24, // Size
     MyGUI::ALIGN_LEFT | MyGUI::ALIGN_TOP,
     "MWButton1");
  tmp->setCaption("Close");
  tmp->eventMouseButtonClick = MyGUI::newDelegate(&turnGuiOff);
  tmp->setInheritsAlpha(false);

  tmp = mwindow->createWidget<MyGUI::StaticText>
    ("DaedricText_orig",
     10,70,
     300, 20,
     MyGUI::ALIGN_LEFT | MyGUI::ALIGN_TOP,
     "Daed1");
  tmp->setCaption("ABCDEFGHIJKLMNOPQRSTUVWXYZ");

  tmp = mwindow->createWidget<MyGUI::StaticText>
    ("DaedricText",
     10,100,
     300, 20,
     MyGUI::ALIGN_LEFT | MyGUI::ALIGN_TOP,
     "Daed2");
  tmp->setCaption("ABCDEFGHIJKLMNOPQRSTUVWXYZ");

  // Turn the GUI off at startup
  turnGuiOff(NULL);
  // Start the mouse in the middle of the screen
  state.X.abs = mWidth / 2;
  state.Y.abs = mHeight / 2;

  MyGUI::ProgressPtr prog;

  // Set up the HUD
  hud = new HUD();

  hud->setStats(60, 100,
                30, 100,
                80, 100);

  hud->setWeapIcon("icons\\w\\tx_knife_iron.dds");
  hud->setWeapStatus(90, 100);
  hud->setSpellIcon("icons\\s\\b_tx_s_rstor_health.dds");
  hud->setSpellStatus(65, 100);

  hud->setEffect("icons\\s\\tx_s_chameleon.dds");
}

extern "C" void gui_setFpsText(char *str)
{
  if(FPSText != NULL)
    FPSText->setCaption(str);
}
