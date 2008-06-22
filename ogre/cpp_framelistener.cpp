/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (cpp_framelistener.cpp) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

 */


// Callbacks to D code.

// Called once each frame
extern "C" int d_frameStarted(float time);

// Handle events
extern "C" void d_handleKey(int keycode, unsigned int text);
extern "C" void d_handleMouseMove(const OIS::MouseState *state);
extern "C" void d_handleMouseButton(const OIS::MouseState *state, int button);

// Frame listener, passed to Ogre. The only thing we use this for is
// to capture input and pass control to D code.
class MorroFrameListener: public FrameListener
{

public:
  // Start of frame
  bool frameStarted(const FrameEvent& evt)
  {
    if(mWindow->isClosed())
      return false;

    // Capture keyboard and mouse events
    mKeyboard->capture();
    mMouse->capture();

    return d_frameStarted(evt.timeSinceLastFrame);
  }
};

// Recieves input events and sends them to the D event handler.
class InputListener : public OIS::KeyListener, public OIS::MouseListener
{
public:
  bool keyPressed( const OIS::KeyEvent &arg )
  {
    /*
    std::cout << "KeyPressed {" << arg.key
              << ", " << ((OIS::Keyboard*)(arg.device))->getAsString(arg.key)
              << "} || Character (" << (char)arg.text << ")\n";
    //*/
    d_handleKey(arg.key, arg.text);
    return true;
  }

  bool mouseMoved( const OIS::MouseEvent &arg )
  {
    /*
    const OIS::MouseState& s = arg.state;
    std::cout << "MouseMoved: Abs("
              << s.X.abs << ", " << s.Y.abs << ", " << s.Z.abs << ") Rel("
              << s.X.rel << ", " << s.Y.rel << ", " << s.Z.rel << ")\n";
    */
    d_handleMouseMove(&arg.state);
    return true;
  }

  bool mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
  {
    /*
    const OIS::MouseState& s = arg.state;
    std::cout << "Mouse button #" << id << " pressed. Abs("
              << s.X.abs << ", " << s.Y.abs << ", " << s.Z.abs << ") Rel("
              << s.X.rel << ", " << s.Y.rel << ", " << s.Z.rel << ")\n";
    */
    d_handleMouseButton(&arg.state, id);
    return true;
  }

  // Unused
  bool keyReleased( const OIS::KeyEvent &arg ) { return true; }
  bool mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id ) { return true; }
};

MorroFrameListener mFrameListener;
InputListener mInput;

// Functions called from D during event handling

extern "C" int cpp_isPressed(int keysym)
{
  return mKeyboard->isKeyDown((OIS::KeyCode)keysym);
}

// Dump screen contents to file
extern "C" void cpp_screenshot(char* filename)
{
  mWindow->writeContentsToFile(filename);

  //This doesn't work, I think I have to set up an overlay or
  //something first and display the text manually.
  //mWindow->setDebugText(String("Wrote ") + filename);
}

// Rotate camera as result of mouse movement
extern "C" void cpp_rotateCamera(float x, float y)
{
  mCamera->yaw(Degree(-x));
  mCamera->pitch(Degree(-y));
}

// Get current camera position
extern "C" void cpp_getCameraPos(float *x, float *y, float *z)
{
  Vector3 pos = mCamera->getPosition();
  *x = pos[0];
  *y = -pos[2];
  *z = pos[1];
}

// Move and rotate camera in place. Transforms Morrowind coordinates
// to OGRE coordinates.
extern "C" void cpp_moveCamera(float x, float y, float z,
			       float r1, float r2, float r3)
{
  mCamera->setPosition(Vector3(x,z,-y));

  // Rotation is probably not correct, but for now I have no reference
  // point. Fix it later when we teleport from one cell to another, so
  // we have something to compare against.

  // Rotate around X axis
  Quaternion xr(Radian(-r1), Vector3::UNIT_X);
  // Rotate around Y axis
  Quaternion yr(Radian(r3+3.14), Vector3::UNIT_Y);
  // Rotate around Z axis
  Quaternion zr(Radian(-r2), Vector3::UNIT_Z);

  // Rotates first around z, then y, then x
  mCamera->setOrientation(xr*yr*zr);
}

// Move camera relative to its own axis set
extern "C" void cpp_moveCameraRel(float x, float y, float z)
{
  mCamera->moveRelative(Vector3(x,y,z));
}
