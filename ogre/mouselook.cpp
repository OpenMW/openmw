#include "mouselook.hpp"

#include <OIS/OIS.h>
#include <OgreCamera.h>

using namespace OIS;
using namespace Ogre;
using namespace OEngine::Render;

void MouseLookEvent::event(Type type, int index, const void *p)
{
  if(type != EV_MouseMove || camera == NULL) return;

  MouseEvent *arg = (MouseEvent*)(p);

  float x = arg->state.X.rel * sensX;
  float y = arg->state.Y.rel * sensY;

  camera->yaw(Degree(-x));

  if(flipProt)
    {
      // The camera before pitching
      Quaternion nopitch = camera->getOrientation();

      camera->pitch(Degree(-y));

      // Apply some failsafe measures against the camera flipping
      // upside down. Is the camera close to pointing straight up or
      // down?
      if(camera->getUp()[1] <= 0.1)
        // If so, undo the last pitch
        camera->setOrientation(nopitch);
    }
  else
    camera->pitch(Degree(-y));
}
