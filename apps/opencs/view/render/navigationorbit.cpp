
#include "navigationorbit.hpp"

#include <OgreCamera.h>

#include <QPoint>

void CSVRender::NavigationOrbit::rotateCamera (const Ogre::Vector3& diff)
{
    Ogre::Vector3 pos = mCamera->getPosition();

    float distance = (pos-mCentre).length();

    Ogre::Vector3 direction = (pos+diff)-mCentre;
    direction.normalise();

    mCamera->setPosition (mCentre + direction*distance);
    mCamera->lookAt (mCentre);
}

CSVRender::NavigationOrbit::NavigationOrbit() : mCamera (0), mCentre (0, 0, 0), mDistance (100)
{}

bool CSVRender::NavigationOrbit::activate (Ogre::Camera *camera)
{
    mCamera = camera;
    mCamera->setFixedYawAxis (false);

    if ((mCamera->getPosition()-mCentre).length()<mDistance)
    {
        // move camera out of the centre area
        Ogre::Vector3 direction = mCentre-mCamera->getPosition();
        direction.normalise();

        if (direction.length()==0)
            direction = Ogre::Vector3 (1, 0, 0);

        mCamera->setPosition (mCentre - direction * mDistance);
    }

    mCamera->lookAt (mCentre);

    return true;
}

bool CSVRender::NavigationOrbit::wheelMoved (int delta)
{
    Ogre::Vector3 diff = getFactor (true) * mCamera->getDirection() * delta;

    Ogre::Vector3 pos = mCamera->getPosition();

    if (delta>0 && diff.length()>=(pos-mCentre).length()-mDistance)
    {
        pos = mCentre-(mCamera->getDirection() * mDistance);
    }
    else
    {
        pos += diff;
    }

    mCamera->setPosition (pos);

    return true;
}

bool CSVRender::NavigationOrbit::mouseMoved (const QPoint& delta, int mode)
{
    Ogre::Vector3 diff =
        getFactor (true) * -mCamera->getDerivedRight() * delta.x()
        + getFactor (true) * mCamera->getDerivedUp() * delta.y();

    if (mode==0)
    {
        rotateCamera (diff);
        return true;
    }
    else if (mode==1)
    {
        mCamera->move (diff);
        mCentre += diff;
        return true;
    }

    return false;
}

bool CSVRender::NavigationOrbit::handleMovementKeys (int vertical, int horizontal)
{
    rotateCamera (
        - getFactor (false) * -mCamera->getDerivedRight() * horizontal
        + getFactor (false) * mCamera->getDerivedUp() * vertical);

    return true;
}

bool CSVRender::NavigationOrbit::handleRollKeys (int delta)
{
    mCamera->roll (Ogre::Degree (getFactor (false) * delta));
    return true;
}
