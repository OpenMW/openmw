#include "mouselook.hpp"

#include <OIS/OIS.h>
#include <OgreCamera.h>
#include <OgreSceneNode.h>

using namespace OIS;
using namespace Ogre;
using namespace OEngine::Render;

void MouseLookEvent::event(Type type, int index, const void *p)
{
	if(type != EV_MouseMove || camera == NULL) return;

	MouseEvent *arg = (MouseEvent*)(p);

	float x = arg->state.X.rel * sensX;
	float y = arg->state.Y.rel * sensY;

	camera->getParentSceneNode()->getParentSceneNode()->yaw(Degree(-x));
	camera->getParentSceneNode()->pitch(Degree(-y));
	if(flipProt)
	{
		// The camera before pitching
		/*Quaternion nopitch = camera->getParentSceneNode()->getOrientation();

		camera->getParentSceneNode()->pitch(Degree(-y));

		// Apply some failsafe measures against the camera flipping
		// upside down. Is the camera close to pointing straight up or
		// down?
		if(Ogre::Vector3(camera->getParentSceneNode()->getOrientation()*Ogre::Vector3::UNIT_Y)[1] <= 0.1)
		// If so, undo the last pitch
		camera->getParentSceneNode()->setOrientation(nopitch);*/
		//camera->getU

		// Angle of rotation around the X-axis.
		float pitchAngle = (2 * Ogre::Degree(Ogre::Math::ACos(camera->getParentSceneNode()->getOrientation().w)).valueDegrees());

		// Just to determine the sign of the angle we pick up above, the
		// value itself does not interest us.
		float pitchAngleSign = camera->getParentSceneNode()->getOrientation().x;

		// Limit the pitch between -90 degress and +90 degrees, Quake3-style.
		if (pitchAngle > 90.0f)
		{
			if (pitchAngleSign > 0)
				// Set orientation to 90 degrees on X-axis.
				camera->getParentSceneNode()->setOrientation(Ogre::Quaternion(Ogre::Math::Sqrt(0.5f),
				Ogre::Math::Sqrt(0.5f), 0, 0));
			else if (pitchAngleSign < 0)
				// Sets orientation to -90 degrees on X-axis.
				camera->getParentSceneNode()->setOrientation(Ogre::Quaternion(Ogre::Math::Sqrt(0.5f),
				-Ogre::Math::Sqrt(0.5f), 0, 0));
		}
	}
}
