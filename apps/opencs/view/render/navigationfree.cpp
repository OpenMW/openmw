
#include "navigationfree.hpp"

#include <OgreCamera.h>

#include <QPoint>

CSVRender::NavigationFree::NavigationFree() : mCamera (0) {}

bool CSVRender::NavigationFree::activate (Ogre::Camera *camera)
{
    mCamera = camera;
    mCamera->setFixedYawAxis (false);
    return false;
}

bool CSVRender::NavigationFree::wheelMoved (int delta)
{
    mCamera->move (getFactor (true) * mCamera->getDirection() * delta);
    return true;
}

bool CSVRender::NavigationFree::mouseMoved (const QPoint& delta, int mode)
{
    if (mode==0)
    {
        // turn camera
        if (delta.x())
            mCamera->yaw (Ogre::Degree (getFactor (true) * delta.x()));

        if (delta.y())
            mCamera->pitch (Ogre::Degree (getFactor (true) * delta.y()));

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

bool CSVRender::NavigationFree::handleMovementKeys (int vertical, int horizontal)
{
    if (vertical)
         mCamera->move (getFactor (false) * mCamera->getDerivedUp() * vertical);

    if (horizontal)
        mCamera->move (getFactor (true) * mCamera->getDerivedRight() * horizontal);

    return true;
}

bool CSVRender::NavigationFree::handleRollKeys (int delta)
{
    mCamera->roll (Ogre::Degree (getFactor (false) * delta));
    return true;
}
