#include <iostream>
#include "../driver.hpp"
#include <unistd.h>
using namespace std;
using namespace Mangle::Input;

Driver *input;

struct MyCB : Event
{
  void event(Event::EventType type, int i, const void *p)
  {
    cout << "got event: type=" << type << " index=" << i << endl;
  }
} mycb;
void mainLoop(int argc, int quitKey)
{
  cout << "Hold the Q key to quit:\n";
  input->setEvent(&mycb);
  while(!input->isDown(quitKey))
    {
      input->capture();
      usleep(20000);

      if(argc == 1)
        {
          cout << "You are running in script mode, aborting. Run this test with a parameter (any at all) to test the input loop properly\n";
          break;
        }
    }

  delete input;
  cout << "\nBye bye!\n";
}
