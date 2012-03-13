#include <iostream>
#include <fstream>

using namespace std;

#include "../servers/sdl_gl_driver.hpp"

using namespace Mangle::Rend2D;

int main()
{
  SDLGLDriver sdl;

  sdl.setVideoMode(640,480,0,false);
  sdl.setWindowTitle("Testing 123");
  Sprite *screen = sdl.getScreen();
  const char* imgName = "tile1-blue.png";
  Sprite *image = sdl.loadImage(imgName);

  for(int frames=0; frames<170; frames++)
    {
      screen->fill(0);
      for(int j=0; j<10; j++)
        for(int i=0; i<25; i++)
          screen->draw(image, 2*frames+30*j, 20*i);
      sdl.update();
      sdl.sleep(5);
    }

  return 0;
}
