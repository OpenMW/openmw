#ifndef OPENMW_COMPONENTS_FX_STATEUPDATER_H
#define OPENMW_COMPONENTS_FX_STATEUPDATER_H

#include <osg/BufferTemplate>

#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/statesetupdater.hpp>
#include <components/std140/ubo.hpp>

namespace fx
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
        struct ProjectionMatrix : std140::Mat4
        {
            static constexpr std::string_view sName = "projectionMatrix";
        };

        struct InvProjectionMatrix : std140::Mat4
        {
            static constexpr std::string_view sName = "invProjectionMatrix";
        };

        struct ViewMatrix : std140::Mat4
        {
            static constexpr std::string_view sName = "viewMatrix";
        };

        struct PrevViewMatrix : std140::Mat4
        {
            static constexpr std::string_view sName = "prevViewMatrix";
        };

        struct InvViewMatrix : std140::Mat4
        {
            static constexpr std::string_view sName = "invViewMatrix";
        };

        struct EyePos : std140::Vec4
        {
            static constexpr std::string_view sName = "eyePos";
        };

        struct EyeVec : std140::Vec4
        {
            static constexpr std::string_view sName = "eyeVec";
        };

        struct AmbientColor : std140::Vec4
        {
            static constexpr std::string_view sName = "ambientColor";
        };

        struct SkyColor : std140::Vec4
        {
            static constexpr std::string_view sName = "skyColor";
        };

        struct FogColor : std140::Vec4
        {
            static constexpr std::string_view sName = "fogColor";
        };

        struct SunColor : std140::Vec4
        {
            static constexpr std::string_view sName = "sunColor";
        };

        struct SunPos : std140::Vec4
        {
            static constexpr std::string_view sName = "sunPos";
        };

        struct Resolution : std140::Vec2
        {
            static constexpr std::string_view sName = "resolution";
        };

        struct RcpResolution : std140::Vec2
        {
            static constexpr std::string_view sName = "rcpResolution";
        };

        struct FogNear : std140::Float
        {
            static constexpr std::string_view sName = "fogNear";
        };

        struct FogFar : std140::Float
        {
            static constexpr std::string_view sName = "fogFar";
        };

        struct Near : std140::Float
        {
            static constexpr std::string_view sName = "near";
        };

        struct Far : std140::Float
        {
            static constexpr std::string_view sName = "far";
        };

        struct Fov : std140::Float
        {
            static constexpr std::string_view sName = "fov";
        };

        struct GameHour : std140::Float
        {
            static constexpr std::string_view sName = "gameHour";
        };

        struct SunVis : std140::Float
        {
            static constexpr std::string_view sName = "sunVis";
        };

        struct WaterHeight : std140::Float
        {
            static constexpr std::string_view sName = "waterHeight";
        };

        struct IsWaterEnabled : std140::Bool
        {
            static constexpr std::string_view sName = "isWaterEnabled";
        };

        struct SimulationTime : std140::Float
        {
            static constexpr std::string_view sName = "simulationTime";
        };

        struct DeltaSimulationTime : std140::Float
        {
            static constexpr std::string_view sName = "deltaSimulationTime";
        };

        struct FrameNumber : std140::Int
        {
            static constexpr std::string_view sName = "frameNumber";
        };

        struct WindSpeed : std140::Float
        {
            static constexpr std::string_view sName = "windSpeed";
        };

        struct WeatherTransition : std140::Float
        {
            static constexpr std::string_view sName = "weatherTransition";
        };

        struct WeatherID : std140::Int
        {
            static constexpr std::string_view sName = "weatherID";
        };

        struct NextWeatherID : std140::Int
        {
            static constexpr std::string_view sName = "nextWeatherID";
        };

        struct IsUnderwater : std140::Bool
        {
            static constexpr std::string_view sName = "isUnderwater";
        };

        struct IsInterior : std140::Bool
        {
            static constexpr std::string_view sName = "isInterior";
        };

        using UniformData
            = std140::UBO<ProjectionMatrix, InvProjectionMatrix, ViewMatrix, PrevViewMatrix, InvViewMatrix, EyePos,
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
