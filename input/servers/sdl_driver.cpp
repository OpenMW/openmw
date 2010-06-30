#include "sdl_driver.hpp"

#include <SDL.h>

using namespace Mangle::Input;

void SDLDriver::capture()
{
  // Poll for events
  SDL_Event evt;
  while(SDL_PollEvent(&evt))
    {
      Event::EventType type = Event::EV_Unknown;
      int index = -1;

      switch(evt.type)
        {
          // For key events, send the keysym as the index.
        case SDL_KEYDOWN:
          type = Event::EV_KeyDown;
          index = evt.key.keysym.sym;
          break;
        case SDL_KEYUP:
          type = Event::EV_KeyUp;
          index = evt.key.keysym.sym;
          break;
        case SDL_MOUSEMOTION:
          type = Event::EV_MouseMove;
          break;
          // Add more event types later
        }

      // Pass the event along, using -1 as index for unidentified
      // event types.
      makeEvent(type, index, &evt);
    }
}

bool SDLDriver::isDown(int index)
{
  int num;
  Uint8 *keys = SDL_GetKeyState(&num);
  assert(index >= 0 && index < num);

  // The returned array from GetKeyState is indexed by the
  // SDLK_KEYNAME enums and is just a list of bools. If the indexed
  // value is true, the button is down.
  return keys[index];
}

void SDLDriver::showMouse(bool show)
{
  SDL_ShowCursor(show?SDL_ENABLE:SDL_DISABLE);
}
