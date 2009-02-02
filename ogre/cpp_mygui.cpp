MyGUI::WidgetPtr FPSText;

MyGUI::WindowPtr window;
MyGUI::WindowPtr mwindow;

void turnGuiOff(MyGUI::WidgetPtr sender)
{
  guiMode = 0;
  mGUI->hidePointer();
  if(window)
    {
      window->destroySmooth();
      window = NULL;
    }
  if(mwindow)
    {
      mwindow->destroySmooth();
      mwindow = NULL;
    }
}

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

  guiMode = 1;
  MyGUI::WidgetPtr tmp;
  /*
  // TESTING WINDOW WITH BUTTON
  width = 300;
  height = 200;
  window = mGUI->createWidget<MyGUI::Window>
    ("WindowCS",
     (mWidth-width)/4, (mHeight-height)/4, // Position
     width, height, // Size
     MyGUI::ALIGN_DEFAULT, "Overlapped");
  //window->setFontName("ManualFont");
  window->setCaption("GUI Demo");
  window->setAlpha(0.7);

  width = 150;
  height = 30;
  tmp = window->createWidget<MyGUI::Button>
    ("ButtonSmall",
     40, 100, // Position
     width, height, // Size
     MyGUI::ALIGN_LEFT | MyGUI::ALIGN_TOP,
     "QuitButton");
  tmp->setCaption("Press this button");
  tmp->eventMouseButtonClick = MyGUI::newDelegate(&turnGuiOff);
  */

  // TESTING MORROWIND SKIN
  width = 300;
  height = 190;
  mwindow = mGUI->createWidget<MyGUI::Window>
    ("MW_Window",
     (mWidth-width)/4, (mHeight-height)/4, // Position
     width, height, // Size
     MyGUI::ALIGN_DEFAULT, "Overlapped");
  mwindow->setCaption("Skin test");
  mwindow->setMinMax(100, 140, 1000, 1000);
  mwindow->setAlpha(1);

  width = 45;
  height = 24;
  tmp = mwindow->createWidget<MyGUI::Button>
    ("MW_Button",
     10, 32, // Position
     width, height, // Size
     MyGUI::ALIGN_LEFT | MyGUI::ALIGN_TOP,
     "MWButton1");
  tmp->setCaption("Close");
  tmp->eventMouseButtonClick = MyGUI::newDelegate(&turnGuiOff);

  // TESTING BITMAP FONT
  /*
  tmp = mGUI->createWidget<MyGUI::Widget>
    ("StaticText",
     10, mHeight - height, // Position
     width, height, // Size
     MyGUI::ALIGN_LEFT | MyGUI::ALIGN_BOTTOM,
     "Statistic");
  tmp->setTextAlign(MyGUI::ALIGN_LEFT);
  tmp->setFontName("ManualFont");
  tmp->setCaption("ABC");
  //*/
}

extern "C" void gui_setFpsText(char *str)
{
  if(FPSText != NULL)
    FPSText->setCaption(str);
}
