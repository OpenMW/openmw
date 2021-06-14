#include "luabindings.hpp"

#include "../mwrender/camera.hpp"

namespace MWLua
{

    using CameraMode = MWRender::Camera::Mode;

    sol::table initCameraPackage(const Context& context)
    {
        MWRender::Camera* camera = MWBase::Environment::get().getWorld()->getCamera();

        sol::table api(context.mLua->sol(), sol::create);
        api["MODE"] = LuaUtil::makeReadOnly(context.mLua->sol().create_table_with(
            "Static", CameraMode::Static,
            "FirstPerson", CameraMode::FirstPerson,
            "ThirdPerson", CameraMode::ThirdPerson,
            "Vanity", CameraMode::Vanity,
            "Preview", CameraMode::Preview
        ));

        api["getMode"] = [camera]() -> int { return static_cast<int>(camera->getMode()); };
        api["getQueuedMode"] = [camera]() -> sol::optional<int>
        {
            std::optional<CameraMode> mode = camera->getQueuedMode();
            if (mode)
                return static_cast<int>(*mode);
            else
                return sol::nullopt;
        };
        api["setMode"] = [camera](int mode, sol::optional<bool> force)
        {
            camera->setMode(static_cast<CameraMode>(mode), force ? *force : false);
        };

        api["allowCharacterDeferredRotation"] = [camera](bool v) { camera->allowCharacterDeferredRotation(v); };
        api["showCrosshair"] = [camera](bool v) { camera->showCrosshair(v); };

        api["getTrackedPosition"] = [camera]() -> osg::Vec3f { return camera->getTrackedPosition(); };
        api["getPosition"] = [camera]() -> osg::Vec3f { return camera->getPosition(); };

        // All angles are negated in order to make camera rotation consistent with objects rotation.
        // TODO: Fix the inconsistency of rotation direction in camera.cpp.
        api["getPitch"] = [camera]() { return -camera->getPitch(); };
        api["getYaw"] = [camera]() { return -camera->getYaw(); };
        api["getRoll"] = [camera]() { return -camera->getRoll(); };

        api["setStaticPosition"] = [camera](const osg::Vec3f& pos) { camera->setStaticPosition(pos); };
        api["setPitch"] = [camera](float v)
        {
            camera->setPitch(-v, true);
            if (camera->getMode() == CameraMode::ThirdPerson)
                camera->calculateDeferredRotation();
        };
        api["setYaw"] = [camera](float v)
        {
            camera->setYaw(-v, true);
            if (camera->getMode() == CameraMode::ThirdPerson)
                camera->calculateDeferredRotation();
        };
        api["setRoll"] = [camera](float v) { camera->setRoll(-v); };
        api["setExtraPitch"] = [camera](float v) { camera->setExtraPitch(-v); };
        api["setExtraYaw"] = [camera](float v) { camera->setExtraYaw(-v); };
        api["getExtraPitch"] = [camera]() { return -camera->getExtraPitch(); };
        api["getExtraYaw"] = [camera]() { return -camera->getExtraYaw(); };

        api["getThirdPersonDistance"] = [camera]() { return camera->getCameraDistance(); };
        api["setPreferredThirdPersonDistance"] = [camera](float v) { camera->setPreferredCameraDistance(v); };

        api["getFirstPersonOffset"] = [camera]() { return camera->getFirstPersonOffset(); };
        api["setFirstPersonOffset"] = [camera](const osg::Vec3f& v) { camera->setFirstPersonOffset(v); };

        api["getFocalPreferredOffset"] = [camera]() -> osg::Vec2f { return camera->getFocalPointTargetOffset(); };
        api["setFocalPreferredOffset"] = [camera](const osg::Vec2f& v) { camera->setFocalPointTargetOffset(v); };
        api["getFocalTransitionSpeed"] = [camera]() { return camera->getFocalPointTransitionSpeed(); };
        api["setFocalTransitionSpeed"] = [camera](float v) { camera->setFocalPointTransitionSpeed(v); };
        api["instantTransition"] = [camera]() { camera->instantTransition(); };

        return LuaUtil::makeReadOnly(api);
    }

}
