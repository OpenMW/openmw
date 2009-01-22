module monster.modules.all;

import monster.modules.io;
import monster.modules.timer;
import monster.modules.frames;
import monster.modules.random;
import monster.modules.threads;

void initAllModules()
{
  initIOModule();
  initTimerModule();
  initFramesModule();
  initThreadModule();
  initRandomModule();
}
