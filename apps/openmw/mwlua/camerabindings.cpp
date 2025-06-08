#include "camerabindings.hpp"

#include <components/lua/luastate.hpp>
#include <components/lua/utilpackage.hpp>
#include <components/misc/finitenumbers.hpp>
#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwrender/camera.hpp"
#include "../mwrender/renderingmanager.hpp"

namespace MWLua
{
    using CameraMode = MWRender::Camera::Mode;

    sol::table initCameraPackage(sol::state_view lua)
    {
        using Misc::FiniteFloat;

        MWRender::Camera* camera = MWBase::Environment::get().getWorld()->getCamera();
        MWRender::RenderingManager* renderingManager = MWBase::Environment::get().getWorld()->getRenderingManager();

        sol::table api(lua, sol::create);
        api["MODE"] = LuaUtil::makeStrictReadOnly(
            lua.create_table_with("Static", CameraMode::Static, "FirstPerson", CameraMode::FirstPerson, "ThirdPerson",
                CameraMode::ThirdPerson, "Vanity", CameraMode::Vanity, "Preview", CameraMode::Preview));

        api["getMode"] = [camera]() -> int { return static_cast<int>(camera->getMode()); };
        api["getQueuedMode"] = [camera]() -> sol::optional<int> {
            std::optional<CameraMode> mode = camera->getQueuedMode();
            if (mode)
                return static_cast<int>(*mode);
            else
                return sol::nullopt;
        };
        api["setMode"] = [camera](int mode, sol::optional<bool> force) {
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
        api["setPitch"] = [camera](const FiniteFloat v) {
            camera->setPitch(-v, true);
            if (camera->getMode() == CameraMode::ThirdPerson)
                camera->calculateDeferredRotation();
        };
        api["setYaw"] = [camera](const FiniteFloat v) {
            camera->setYaw(-v, true);
            if (camera->getMode() == CameraMode::ThirdPerson)
                camera->calculateDeferredRotation();
        };
        api["setRoll"] = [camera](const FiniteFloat v) { camera->setRoll(-v); };
        api["setExtraPitch"] = [camera](const FiniteFloat v) { camera->setExtraPitch(-v); };
        api["setExtraYaw"] = [camera](const FiniteFloat v) { camera->setExtraYaw(-v); };
        api["setExtraRoll"] = [camera](const FiniteFloat v) { camera->setExtraRoll(-v); };
        api["getExtraPitch"] = [camera]() { return -camera->getExtraPitch(); };
        api["getExtraYaw"] = [camera]() { return -camera->getExtraYaw(); };
        api["getExtraRoll"] = [camera]() { return -camera->getExtraRoll(); };

        api["getThirdPersonDistance"] = [camera]() { return camera->getCameraDistance(); };
        api["setPreferredThirdPersonDistance"]
            = [camera](const FiniteFloat v) { camera->setPreferredCameraDistance(v); };

        api["getFirstPersonOffset"] = [camera]() { return camera->getFirstPersonOffset(); };
        api["setFirstPersonOffset"] = [camera](const osg::Vec3f& v) { camera->setFirstPersonOffset(v); };

        api["getFocalPreferredOffset"] = [camera]() -> osg::Vec2f { return camera->getFocalPointTargetOffset(); };
        api["setFocalPreferredOffset"] = [camera](const osg::Vec2f& v) { camera->setFocalPointTargetOffset(v); };
        api["getFocalTransitionSpeed"] = [camera]() { return camera->getFocalPointTransitionSpeed(); };
        api["setFocalTransitionSpeed"] = [camera](const FiniteFloat v) { camera->setFocalPointTransitionSpeed(v); };
        api["instantTransition"] = [camera]() { camera->instantTransition(); };

        api["getCollisionType"] = [camera]() { return camera->getCollisionType(); };
        api["setCollisionType"] = [camera](int collisionType) { camera->setCollisionType(collisionType); };

        api["getBaseFieldOfView"] = [] { return osg::DegreesToRadians(Settings::camera().mFieldOfView); };
        api["getFieldOfView"]
            = [renderingManager]() { return osg::DegreesToRadians(renderingManager->getFieldOfView()); };
        api["setFieldOfView"]
            = [renderingManager](const FiniteFloat v) { renderingManager->setFieldOfView(osg::RadiansToDegrees(v)); };

        api["getBaseViewDistance"] = [] { return Settings::camera().mViewingDistance.get(); };
        api["getViewDistance"] = [renderingManager]() { return renderingManager->getViewDistance(); };
        api["setViewDistance"]
            = [renderingManager](const FiniteFloat d) { renderingManager->setViewDistance(d, true); };

        api["getViewTransform"] = [camera]() { return LuaUtil::TransformM{ camera->getViewMatrix() }; };

        api["viewportToWorldVector"] = [camera, renderingManager](osg::Vec2f pos) -> osg::Vec3f {
            const double width = Settings::video().mResolutionX;
            const double height = Settings::video().mResolutionY;
            double aspect = (height == 0.0) ? 1.0 : width / height;
            double fovTan = std::tan(osg::DegreesToRadians(renderingManager->getFieldOfView()) / 2);
            osg::Matrixf invertedViewMatrix;
            invertedViewMatrix.invert(camera->getViewMatrix());
            float x = (pos.x() * 2 - 1) * aspect * fovTan;
            float y = (1 - pos.y() * 2) * fovTan;
            return invertedViewMatrix.preMult(osg::Vec3f(x, y, -1)) - camera->getPosition();
        };

        api["worldToViewportVector"] = [camera](osg::Vec3f pos) {
            const double width = Settings::video().mResolutionX;
            const double height = Settings::video().mResolutionY;

            osg::Matrix windowMatrix
                = osg::Matrix::translate(1.0, 1.0, 1.0) * osg::Matrix::scale(0.5 * width, 0.5 * height, 0.5);
            osg::Vec3f vpCoords = pos * (camera->getViewMatrix() * camera->getProjectionMatrix() * windowMatrix);

            // Move 0,0 to top left to match viewportToWorldVector
            vpCoords.y() = height - vpCoords.y();

            // Set the z component to be distance from camera, in world space units
            vpCoords.z() = (pos - camera->getPosition()).length();

            return vpCoords;
        };

        return LuaUtil::makeReadOnly(api);
    }

}
