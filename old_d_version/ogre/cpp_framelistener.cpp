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
