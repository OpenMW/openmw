#include "common.cpp"

#include "../servers/sdl_driver.hpp"
#include <SDL.h>

int main(int argc, char** argv)
{
  SDL_Init(SDL_INIT_VIDEO);
  SDL_SetVideoMode(640, 480, 0, SDL_SWSURFACE);
  input = new SDLDriver();

  mainLoop(argc, SDLK_q);

  SDL_Quit();
  return 0;
}
