bool consoleMode = false;
bool inventoryMode = false;

void enterGui()
{
  guiMode++;

  if(guiMode == 1)
    {
      // If we just entered GUI mode, enable the pointer
      mGUI->showPointer();

      // Restore the GUI mouse position. This is a hack because silly
      // OIS doesn't allow us to set the mouse position ourselves.
      *((OIS::MouseState*)&(mMouse->getMouseState())) = state;
      mGUI->injectMouseMove(state.X.abs, state.Y.abs, 0);
    }
}

void leaveGui()
{
  guiMode--;

  if(guiMode < 0)
    {
      std::cout << "WARNING: guiMode is " << guiMode << "\n";
      guiMode = 0;
    }

  // Are we done with all GUI windows?
  if(guiMode == 0)
    {
      // If so, hide the pointer and store the mouse state for later.
      mGUI->hidePointer();
      state = mMouse->getMouseState();
    }
}

extern "C" void gui_toggleGui()
{
  if(inventoryMode)
    {
      leaveGui();
      if(stats)
        stats->setVisible(false);
      if(map)
        map->setVisible(false);
    }
  else
    {
      enterGui();
      if(stats)
        stats->setVisible(true);
      if(map)
        map->setVisible(true);
    }

  inventoryMode = !inventoryMode;
}

extern "C" void gui_setupGUI(int32_t debugOut)
{
  guiMode = 1;
  leaveGui();
}
