
#include "navigation1st.hpp"

#include <OgreCamera.h>

#include <QPoint>

CSVRender::Navigation1st::Navigation1st() : mCamera (0) {}

bool CSVRender::Navigation1st::activate (Ogre::Camera *camera)
{
    mCamera = camera;
    mCamera->setFixedYawAxis (true, Ogre::Vector3::UNIT_Z);

    Ogre::Radian pitch = mCamera->getOrientation().getPitch();

    Ogre::Radian limit (Ogre::Math::PI/2-0.5);

    if (pitch>limit)
        mCamera->pitch (-(pitch-limit));
    else if (pitch<-limit)
        mCamera->pitch (pitch-limit);

    return true;
}

bool CSVRender::Navigation1st::wheelMoved (int delta)
{
    mCamera->move (getFactor (true) * mCamera->getDirection() * delta);
    return true;
}

bool CSVRender::Navigation1st::mouseMoved (const QPoint& delta, int mode)
{
    if (mode==0)
    {
        // turn camera
        if (delta.x())
            mCamera->yaw (Ogre::Degree (getFactor (true) * delta.x()));

        if (delta.y())
        {
            Ogre::Radian oldPitch = mCamera->getOrientation().getPitch();
            float deltaPitch = getFactor (true) * delta.y();
            Ogre::Radian newPitch = oldPitch + Ogre::Degree (deltaPitch);

            if ((deltaPitch>0 && newPitch<Ogre::Radian(Ogre::Math::PI-0.5)) ||
                (deltaPitch<0 && newPitch>Ogre::Radian(0.5)))
            {
                mCamera->pitch (Ogre::Degree (deltaPitch));
            }
        }

        return true;
    }
    else if (mode==1)
    {
        // pan camera
        if (delta.x())
            mCamera->move (getFactor (true) * mCamera->getDerivedRight() * delta.x());

        if (delta.y())
            mCamera->move (getFactor (true) * -mCamera->getDerivedUp() * delta.y());

        return true;
    }

    return false;
}

bool CSVRender::Navigation1st::handleMovementKeys (int vertical, int horizontal)
{
    if (vertical)
         mCamera->move (getFactor (false) * mCamera->getDirection() * vertical);

    if (horizontal)
        mCamera->move (getFactor (true) * mCamera->getDerivedRight() * horizontal);

    return true;
}

bool CSVRender::Navigation1st::handleRollKeys (int delta)
{
    // we don't roll this way in 1st person mode
    return false;
}
