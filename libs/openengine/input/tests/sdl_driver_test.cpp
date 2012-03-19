#include <iostream>
#include <mangle/input/servers/sdl_driver.hpp>
#include <SDL.h>

using namespace std;
using namespace Mangle::Input;

int main(int argc, char** argv)
{
  SDL_Init(SDL_INIT_VIDEO);
  SDL_SetVideoMode(640, 480, 0, SDL_SWSURFACE);
  SDLDriver input;

  cout << "Hold the Q key to quit:\n";
  //input->setEvent(&mycb);
  while(!input.isDown(SDLK_q))
    {
      input.capture();
      SDL_Delay(20);

      if(argc == 1)
        {
          cout << "You are running in script mode, aborting. Run this test with a parameter (any at all) to test the input loop properly\n";
          break;
        }
    }
  cout << "\nBye bye!\n";

  SDL_Quit();
  return 0;
}
