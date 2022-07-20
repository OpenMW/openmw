#include "luabindings.hpp"

#include <components/lua/utilpackage.hpp>
#include <components/settings/settings.hpp>

#include "../mwrender/camera.hpp"
#include "../mwrender/renderingmanager.hpp"

namespace MWLua
{

    using CameraMode = MWRender::Camera::Mode;

    sol::table initCameraPackage(const Context& context)
    {
        MWRender::Camera* camera = MWBase::Environment::get().getWorld()->getCamera();
        MWRender::RenderingManager* renderingManager = MWBase::Environment::get().getWorld()->getRenderingManager();

        sol::table api(context.mLua->sol(), sol::create);
        api["MODE"] = LuaUtil::makeStrictReadOnly(context.mLua->sol().create_table_with(
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
        api["setExtraRoll"] = [camera](float v) { camera->setExtraRoll(-v); };
        api["getExtraPitch"] = [camera]() { return -camera->getExtraPitch(); };
        api["getExtraYaw"] = [camera]() { return -camera->getExtraYaw(); };
        api["getExtraRoll"] = [camera]() { return -camera->getExtraRoll(); };

        api["getThirdPersonDistance"] = [camera]() { return camera->getCameraDistance(); };
        api["setPreferredThirdPersonDistance"] = [camera](float v) { camera->setPreferredCameraDistance(v); };

        api["getFirstPersonOffset"] = [camera]() { return camera->getFirstPersonOffset(); };
        api["setFirstPersonOffset"] = [camera](const osg::Vec3f& v) { camera->setFirstPersonOffset(v); };

        api["getFocalPreferredOffset"] = [camera]() -> osg::Vec2f { return camera->getFocalPointTargetOffset(); };
        api["setFocalPreferredOffset"] = [camera](const osg::Vec2f& v) { camera->setFocalPointTargetOffset(v); };
        api["getFocalTransitionSpeed"] = [camera]() { return camera->getFocalPointTransitionSpeed(); };
        api["setFocalTransitionSpeed"] = [camera](float v) { camera->setFocalPointTransitionSpeed(v); };
        api["instantTransition"] = [camera]() { camera->instantTransition(); };

        api["getCollisionType"] = [camera]() { return camera->getCollisionType(); };
        api["setCollisionType"] = [camera](int collisionType) { camera->setCollisionType(collisionType); };

        api["getBaseFieldOfView"] = []()
        {
            return osg::DegreesToRadians(std::clamp(Settings::Manager::getFloat("field of view", "Camera"), 1.f, 179.f));
        };
        api["getFieldOfView"] = [renderingManager]() { return osg::DegreesToRadians(renderingManager->getFieldOfView()); };
        api["setFieldOfView"] = [renderingManager](float v) { renderingManager->setFieldOfView(osg::RadiansToDegrees(v)); };

        api["getBaseViewDistance"] = []()
        {
            return std::max(0.f, Settings::Manager::getFloat("viewing distance", "Camera"));
        };
        api["getViewDistance"] = [renderingManager]() { return renderingManager->getViewDistance(); };
        api["setViewDistance"] = [renderingManager](float d) { renderingManager->setViewDistance(d, true); };

        api["getViewTransform"] = [camera]() { return LuaUtil::TransformM{camera->getViewMatrix()}; };

        api["viewportToWorldVector"] = [camera, renderingManager](osg::Vec2f pos) -> osg::Vec3f
        {
            double width = Settings::Manager::getInt("resolution x", "Video");
            double height = Settings::Manager::getInt("resolution y", "Video");
            double aspect = (height == 0.0) ? 1.0 : width / height;
            double fovTan = std::tan(osg::DegreesToRadians(renderingManager->getFieldOfView()) / 2);
            osg::Matrixf invertedViewMatrix;
            invertedViewMatrix.invert(camera->getViewMatrix());
            float x = (pos.x() * 2 - 1) * aspect * fovTan;
            float y = (1 - pos.y() * 2) * fovTan;
            return invertedViewMatrix.preMult(osg::Vec3f(x, y, -1)) - camera->getPosition();
        };

        return LuaUtil::makeReadOnly(api);
    }

}
