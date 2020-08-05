#include "viewovershoulder.hpp"

#include <osg/Quat>

#include <components/settings/settings.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/refdata.hpp"

#include "../mwmechanics/drawstate.hpp"

namespace MWRender
{

    ViewOverShoulderController::ViewOverShoulderController(Camera* camera) :
        mCamera(camera), mMode(Mode::RightShoulder),
        mAutoSwitchShoulder(Settings::Manager::getBool("auto switch shoulder", "Camera")),
        mOverShoulderHorizontalOffset(30.f), mOverShoulderVerticalOffset(-10.f)
    {
        osg::Vec2f offset = Settings::Manager::getVector2("view over shoulder offset", "Camera");
        mOverShoulderHorizontalOffset = std::abs(offset.x());
        mOverShoulderVerticalOffset = offset.y();
        mDefaultShoulderIsRight = offset.x() >= 0;

        mCamera->enableDynamicCameraDistance(true);
        mCamera->enableCrosshairInThirdPersonMode(true);
        mCamera->setFocalPointTargetOffset(offset);
    }

    void ViewOverShoulderController::update()
    {
        if (mCamera->isFirstPerson())
            return;

        Mode oldMode = mMode;
        auto ptr = mCamera->getTrackingPtr();
        bool combat = ptr.getClass().isActor() && ptr.getClass().getCreatureStats(ptr).getDrawState() != MWMechanics::DrawState_Nothing;
        if (combat && !mCamera->isVanityOrPreviewModeEnabled())
            mMode = Mode::Combat;
        else if (MWBase::Environment::get().getWorld()->isSwimming(ptr))
            mMode = Mode::Swimming;
        else if (oldMode == Mode::Combat || oldMode == Mode::Swimming)
            mMode = mDefaultShoulderIsRight ? Mode::RightShoulder : Mode::LeftShoulder;
        if (mAutoSwitchShoulder && (mMode == Mode::LeftShoulder || mMode == Mode::RightShoulder))
            trySwitchShoulder();

        if (oldMode == mMode)
            return;

        if (mCamera->getMode() == Camera::Mode::Vanity)
            // Player doesn't touch controls for a long time. Transition should be very slow.
            mCamera->setFocalPointTransitionSpeed(0.2f);
        else if ((oldMode == Mode::Combat || mMode == Mode::Combat) && mCamera->getMode() == Camera::Mode::Normal)
            // Transition to/from combat mode and we are not it preview mode. Should be fast.
            mCamera->setFocalPointTransitionSpeed(5.f);
        else
            mCamera->setFocalPointTransitionSpeed(1.f);  // Default transition speed.

        switch (mMode)
        {
        case Mode::RightShoulder:
            mCamera->setFocalPointTargetOffset({mOverShoulderHorizontalOffset, mOverShoulderVerticalOffset});
            break;
        case Mode::LeftShoulder:
            mCamera->setFocalPointTargetOffset({-mOverShoulderHorizontalOffset, mOverShoulderVerticalOffset});
            break;
        case Mode::Combat:
        case Mode::Swimming:
        default:
            mCamera->setFocalPointTargetOffset({0, 15});
        }
    }

    void ViewOverShoulderController::trySwitchShoulder()
    {
        if (mCamera->getMode() != Camera::Mode::Normal)
            return;

        const float limitToSwitch = 120; // switch to other shoulder if wall is closer than this limit
        const float limitToSwitchBack = 300; // switch back to default shoulder if there is no walls at this distance

        auto orient = osg::Quat(mCamera->getYaw(), osg::Vec3d(0,0,1));
        osg::Vec3d playerPos = mCamera->getFocalPoint() - mCamera->getFocalPointOffset();

        MWBase::World* world = MWBase::Environment::get().getWorld();
        osg::Vec3d sideOffset = orient * osg::Vec3d(world->getHalfExtents(mCamera->getTrackingPtr()).x() - 1, 0, 0);
        float rayRight = world->getDistToNearestRayHit(
            playerPos + sideOffset, orient * osg::Vec3d(1, 0, 0), limitToSwitchBack + 1);
        float rayLeft = world->getDistToNearestRayHit(
            playerPos - sideOffset, orient * osg::Vec3d(-1, 0, 0), limitToSwitchBack + 1);
        float rayRightForward = world->getDistToNearestRayHit(
            playerPos + sideOffset, orient * osg::Vec3d(1, 3, 0), limitToSwitchBack + 1);
        float rayLeftForward = world->getDistToNearestRayHit(
            playerPos - sideOffset, orient * osg::Vec3d(-1, 3, 0), limitToSwitchBack + 1);
        float distRight = std::min(rayRight, rayRightForward);
        float distLeft = std::min(rayLeft, rayLeftForward);

        if (distLeft < limitToSwitch && distRight > limitToSwitchBack)
            mMode = Mode::RightShoulder;
        else if (distRight < limitToSwitch && distLeft > limitToSwitchBack)
            mMode = Mode::LeftShoulder;
        else if (distRight > limitToSwitchBack && distLeft > limitToSwitchBack)
            mMode = mDefaultShoulderIsRight ? Mode::RightShoulder : Mode::LeftShoulder;
    }

}
