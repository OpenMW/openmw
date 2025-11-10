#ifndef OPENMW_COMPONENTS_FX_STATEUPDATER_H
#define OPENMW_COMPONENTS_FX_STATEUPDATER_H

#include <osg/BufferTemplate>

#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/statesetupdater.hpp>
#include <components/std140/ubo.hpp>

namespace Fx
{
    class StateUpdater : public SceneUtil::StateSetUpdater
    {
    public:
        StateUpdater(bool useUBO);

        void setProjectionMatrix(const osg::Matrixf& matrix)
        {
            mData.get<ProjectionMatrix>() = matrix;
            mData.get<InvProjectionMatrix>() = osg::Matrixf::inverse(matrix);
        }

        void setViewMatrix(const osg::Matrixf& matrix)
        {
            mData.get<ViewMatrix>() = matrix;
            mData.get<InvViewMatrix>() = osg::Matrixf::inverse(matrix);
        }

        void setPrevViewMatrix(const osg::Matrixf& matrix) { mData.get<PrevViewMatrix>() = matrix; }

        void setEyePos(const osg::Vec3f& pos) { mData.get<EyePos>() = osg::Vec4f(pos, 0.f); }

        void setEyeVec(const osg::Vec3f& vec) { mData.get<EyeVec>() = osg::Vec4f(vec, 0.f); }

        void setFogColor(const osg::Vec4f& color) { mData.get<FogColor>() = color; }

        void setAmbientColor(const osg::Vec4f& color) { mData.get<AmbientColor>() = color; }

        void setSkyColor(const osg::Vec4f& color) { mData.get<SkyColor>() = color; }

        void setSunColor(const osg::Vec4f& color) { mData.get<SunColor>() = color; }

        void setSunPos(const osg::Vec4f& pos, bool night)
        {
            mData.get<SunPos>() = pos;
            mData.get<SunPos>().normalize();

            if (night)
                mData.get<SunPos>().z() *= -1.f;
        }

        void setResolution(const osg::Vec2f& size)
        {
            mData.get<Resolution>() = size;
            mData.get<RcpResolution>() = { 1.f / size.x(), 1.f / size.y() };
        }

        void setSunVis(float vis) { mData.get<SunVis>() = vis; }

        void setFogRange(float near, float far)
        {
            mData.get<FogNear>() = near;
            mData.get<FogFar>() = far;
        }

        void setNearFar(float near, float far)
        {
            mData.get<Near>() = near;
            mData.get<Far>() = far;
        }

        void setIsUnderwater(bool underwater) { mData.get<IsUnderwater>() = underwater; }

        void setIsInterior(bool interior) { mData.get<IsInterior>() = interior; }

        void setFov(float fov) { mData.get<Fov>() = fov; }

        void setGameHour(float hour) { mData.get<GameHour>() = hour; }

        void setWeatherId(int id) { mData.get<WeatherID>() = id; }

        void setNextWeatherId(int id) { mData.get<NextWeatherID>() = id; }

        void setWaterHeight(float height) { mData.get<WaterHeight>() = height; }

        void setIsWaterEnabled(bool enabled) { mData.get<IsWaterEnabled>() = enabled; }

        void setSimulationTime(float time) { mData.get<SimulationTime>() = time; }

        void setDeltaSimulationTime(float time) { mData.get<DeltaSimulationTime>() = time; }

        void setFrameNumber(int frame) { mData.get<FrameNumber>() = frame; }

        void setWindSpeed(float speed) { mData.get<WindSpeed>() = speed; }

        void setWeatherTransition(float transition)
        {
            mData.get<WeatherTransition>() = transition > 0 ? 1 - transition : 0;
        }

        void bindPointLights(std::shared_ptr<SceneUtil::PPLightBuffer> buffer)
        {
            mPointLightBuffer = std::move(buffer);
        }

        static const std::string& getStructDefinition() { return sDefinition; }

        void setDefaults(osg::StateSet* stateset) override;

        void apply(osg::StateSet* stateset, osg::NodeVisitor* nv) override;

    private:
        struct ProjectionMatrix : Std140::Mat4
        {
            static constexpr std::string_view sName = "projectionMatrix";
        };

        struct InvProjectionMatrix : Std140::Mat4
        {
            static constexpr std::string_view sName = "invProjectionMatrix";
        };

        struct ViewMatrix : Std140::Mat4
        {
            static constexpr std::string_view sName = "viewMatrix";
        };

        struct PrevViewMatrix : Std140::Mat4
        {
            static constexpr std::string_view sName = "prevViewMatrix";
        };

        struct InvViewMatrix : Std140::Mat4
        {
            static constexpr std::string_view sName = "invViewMatrix";
        };

        struct EyePos : Std140::Vec4
        {
            static constexpr std::string_view sName = "eyePos";
        };

        struct EyeVec : Std140::Vec4
        {
            static constexpr std::string_view sName = "eyeVec";
        };

        struct AmbientColor : Std140::Vec4
        {
            static constexpr std::string_view sName = "ambientColor";
        };

        struct SkyColor : Std140::Vec4
        {
            static constexpr std::string_view sName = "skyColor";
        };

        struct FogColor : Std140::Vec4
        {
            static constexpr std::string_view sName = "fogColor";
        };

        struct SunColor : Std140::Vec4
        {
            static constexpr std::string_view sName = "sunColor";
        };

        struct SunPos : Std140::Vec4
        {
            static constexpr std::string_view sName = "sunPos";
        };

        struct Resolution : Std140::Vec2
        {
            static constexpr std::string_view sName = "resolution";
        };

        struct RcpResolution : Std140::Vec2
        {
            static constexpr std::string_view sName = "rcpResolution";
        };

        struct FogNear : Std140::Float
        {
            static constexpr std::string_view sName = "fogNear";
        };

        struct FogFar : Std140::Float
        {
            static constexpr std::string_view sName = "fogFar";
        };

        struct Near : Std140::Float
        {
            static constexpr std::string_view sName = "near";
        };

        struct Far : Std140::Float
        {
            static constexpr std::string_view sName = "far";
        };

        struct Fov : Std140::Float
        {
            static constexpr std::string_view sName = "fov";
        };

        struct GameHour : Std140::Float
        {
            static constexpr std::string_view sName = "gameHour";
        };

        struct SunVis : Std140::Float
        {
            static constexpr std::string_view sName = "sunVis";
        };

        struct WaterHeight : Std140::Float
        {
            static constexpr std::string_view sName = "waterHeight";
        };

        struct IsWaterEnabled : Std140::Bool
        {
            static constexpr std::string_view sName = "isWaterEnabled";
        };

        struct SimulationTime : Std140::Float
        {
            static constexpr std::string_view sName = "simulationTime";
        };

        struct DeltaSimulationTime : Std140::Float
        {
            static constexpr std::string_view sName = "deltaSimulationTime";
        };

        struct FrameNumber : Std140::Int
        {
            static constexpr std::string_view sName = "frameNumber";
        };

        struct WindSpeed : Std140::Float
        {
            static constexpr std::string_view sName = "windSpeed";
        };

        struct WeatherTransition : Std140::Float
        {
            static constexpr std::string_view sName = "weatherTransition";
        };

        struct WeatherID : Std140::Int
        {
            static constexpr std::string_view sName = "weatherID";
        };

        struct NextWeatherID : Std140::Int
        {
            static constexpr std::string_view sName = "nextWeatherID";
        };

        struct IsUnderwater : Std140::Bool
        {
            static constexpr std::string_view sName = "isUnderwater";
        };

        struct IsInterior : Std140::Bool
        {
            static constexpr std::string_view sName = "isInterior";
        };

        using UniformData
            = Std140::UBO<ProjectionMatrix, InvProjectionMatrix, ViewMatrix, PrevViewMatrix, InvViewMatrix, EyePos,
                EyeVec, FogColor, AmbientColor, SkyColor, SunColor, SunPos, Resolution, RcpResolution, FogNear, FogFar,
                Near, Far, Fov, GameHour, SunVis, WaterHeight, IsWaterEnabled, SimulationTime, DeltaSimulationTime,
                FrameNumber, WindSpeed, WeatherTransition, WeatherID, NextWeatherID, IsUnderwater, IsInterior>;

        UniformData mData;
        bool mUseUBO;

        static std::string sDefinition;

        std::shared_ptr<SceneUtil::PPLightBuffer> mPointLightBuffer;
    };
}

#endif
