// Frame listener, passed to Ogre. The only thing we use this for is
// to capture input and pass control to D code.
class MorroFrameListener: public FrameListener
{

public:
  // Start of frame
  bool frameStarted(const FrameEvent& evt)
  {
    TRACE("frameStarted (Input)");

    if(mWindow->isClosed())
      return false;

    // Capture keyboard and mouse events
    mKeyboard->capture();
    mMouse->capture();

    // Notify the GUI
    mGUI->injectFrameEntered(evt.timeSinceLastFrame);

    // Turn over control to the D code
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
    if(guiMode)
      mGUI->injectKeyPress(arg);

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
    // TODO: this should be handled elsewhere later on, most likely in
    // Monster script.
    if(guiMode)
      mGUI->injectMouseMove(arg);

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
    if(guiMode)
      mGUI->injectMousePress(arg, id);

    d_handleMouseButton(&arg.state, id);
    return true;
  }

  bool mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
  {
    if(guiMode)
      mGUI->injectMouseRelease(arg, id);
    return true;
  }

  bool keyReleased( const OIS::KeyEvent &arg )
  {
    if(guiMode)
      mGUI->injectKeyRelease(arg);
    return true;
  }
};

MorroFrameListener mFrameListener;
InputListener mInput;

// Functions called from D during event handling

// Dump screen contents to file
extern "C" void ogre_screenshot(char* filename)
{
  mWindow->writeContentsToFile(filename);
}

// Get current camera orientation, in the form of 'front' and 'up'
// vectors.
extern "C" void ogre_getCameraOrientation(float *fx, float *fy, float *fz,
                                         float *ux, float *uy, float *uz)
{
  Vector3 front = mCamera->getDirection();
  Vector3 up = mCamera->getUp();
  *fx = front[0];
  *fy = -front[2];
  *fz = front[1];
  *ux = up[0];
  *uy = -up[2];
  *uz = up[1];
}

// Move camera
extern "C" void ogre_moveCamera(float x, float y, float z)
{
  // Transforms Morrowind coordinates to OGRE coordinates. The camera
  // is not affected by the rotation of the root node, so we must
  // transform this manually.
  mCamera->setPosition(Vector3(x,z+90,-y));
}

// Rotate camera using Morrowind rotation specifiers
extern "C" void ogre_setCameraRotation(float r1, float r2, float r3)
{
  // TODO: This translation is probably not correct, but for now I
  // have no reference point. Fix it later when we teleport from one
  // cell to another, so we have something to compare against.

  // Rotate around X axis
  Quaternion xr(Radian(-r1), Vector3::UNIT_X);
  // Rotate around Y axis
  Quaternion yr(Radian(r3+3.14), Vector3::UNIT_Y);
  // Rotate around Z axis
  Quaternion zr(Radian(-r2), Vector3::UNIT_Z);

  // Rotates first around z, then y, then x
  mCamera->setOrientation(xr*yr*zr);
}
