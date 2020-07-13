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
        std::stringstream offset(Settings::Manager::getString("view over shoulder offset", "Camera"));
        offset >> mOverShoulderHorizontalOffset >> mOverShoulderVerticalOffset;
        mDefaultShoulderIsRight = mOverShoulderHorizontalOffset >= 0;
        mOverShoulderHorizontalOffset = std::abs(mOverShoulderHorizontalOffset);

        mCamera->enableDynamicCameraDistance(true);
        mCamera->enableCrosshairInThirdPersonMode(true);
        mCamera->setFocalPointTargetOffset({mOverShoulderHorizontalOffset, mOverShoulderVerticalOffset});
    }

    void ViewOverShoulderController::update()
    {
        if (mCamera->isVanityOrPreviewModeEnabled() || mCamera->isFirstPerson())
            return;

        Mode oldMode = mMode;
        auto ptr = mCamera->getTrackingPtr();
        if (ptr.getClass().isActor() && ptr.getClass().getCreatureStats(ptr).getDrawState() != MWMechanics::DrawState_Nothing)
            mMode = Mode::Combat;
        else if (MWBase::Environment::get().getWorld()->isSwimming(ptr))
            mMode = Mode::Swimming;
        else if (oldMode == Mode::Combat || oldMode == Mode::Swimming)
            mMode = mDefaultShoulderIsRight ? Mode::RightShoulder : Mode::LeftShoulder;
        if (mAutoSwitchShoulder && (mMode == Mode::LeftShoulder || mMode == Mode::RightShoulder))
            trySwitchShoulder();
        if (oldMode == mMode) return;

        if (oldMode == Mode::Combat || mMode == Mode::Combat)
            mCamera->setFocalPointTransitionSpeed(5.f);
        else
            mCamera->setFocalPointTransitionSpeed(1.f);

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
        const float limitToSwitch = 120; // switch to other shoulder if wall is closer than this limit
        const float limitToSwitchBack = 300; // switch back to default shoulder if there is no walls at this distance

        auto orient = osg::Quat(mCamera->getYaw(), osg::Vec3d(0,0,1));
        osg::Vec3d playerPos = mCamera->getFocalPoint() - mCamera->getFocalPointOffset();

        MWBase::World* world = MWBase::Environment::get().getWorld();
        osg::Vec3d sideOffset = orient * osg::Vec3d(world->getHalfExtents(mCamera->getTrackingPtr()).x() - 1, 0, 0);
        float rayRight = world->getDistToNearestRayHit(
            playerPos + sideOffset, orient * osg::Vec3d(1, 1, 0), limitToSwitchBack + 1);
        float rayLeft = world->getDistToNearestRayHit(
            playerPos - sideOffset, orient * osg::Vec3d(-1, 1, 0), limitToSwitchBack + 1);
        float rayForward = world->getDistToNearestRayHit(
            playerPos, orient * osg::Vec3d(0, 1, 0), limitToSwitchBack + 1);

        if (rayLeft < limitToSwitch && rayRight > limitToSwitchBack)
            mMode = Mode::RightShoulder;
        else if (rayRight < limitToSwitch && rayLeft > limitToSwitchBack)
            mMode = Mode::LeftShoulder;
        else if (rayLeft > limitToSwitchBack && rayRight > limitToSwitchBack && rayForward > limitToSwitchBack)
            mMode = mDefaultShoulderIsRight ? Mode::RightShoulder : Mode::LeftShoulder;
    }

}