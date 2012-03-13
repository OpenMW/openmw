#include <iostream>
#include <fstream>

using namespace std;

#include "../servers/sdl_driver.hpp"

using namespace Mangle::Rend2D;

int main()
{
  cout << "Loading SDL driver.\n";
  SDLDriver sdl;

  cout << "Creating window.\n";
  sdl.setVideoMode(640,480);
  cout << "Current mode: " << sdl.width() << "x" << sdl.height() << endl;

  cout << "Setting fancy title, cause we like fancy titles.\n";
  sdl.setWindowTitle("Chief executive window");

  // Display surface
  Sprite *screen = sdl.getScreen();

  const char* imgName = "tile1-blue.png";
  cout << "Loading " << imgName << " from file.\n";
  Sprite *image = sdl.loadImage(imgName);

  const char* imgName2 = "tile1-yellow.png";
  cout << "Loading " << imgName2 << " from memory.\n";
  Sprite *image2;
  {
    // This is hard-coded for file sizes below 500 bytes, so obviously
    // you shouldn't mess with the image files.
    ifstream file(imgName2, ios::binary);
    char buf[500];
    file.read(buf, 500);
    int size = file.gcount();
    image2 = sdl.loadImage(buf, size);
  }

  cout << "Going bananas.\n";
  for(int i=1; i<20; i++)
    screen->draw(image, 30*i, 20*i);

  cout << "Taking a breather.\n";
  sdl.update();
  for(int i=1; i<20; i++)
    screen->draw(image2, 30*(20-i), 20*i);
  sdl.sleep(800);
  sdl.update();
  cout << "WOW DID YOU SEE THAT!?\n";
  sdl.sleep(800);

  cout << "Mucking about with the gamma settings\n";
  sdl.setGamma(2.0, 0.1, 0.8);
  sdl.sleep(100);
  sdl.setGamma(0.6, 2.1, 2.1);
  sdl.sleep(100);
  sdl.setGamma(1.6);
  sdl.sleep(100);

  cout << "Done.\n";
  return 0;
}
