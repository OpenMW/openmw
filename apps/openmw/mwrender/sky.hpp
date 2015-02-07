#ifndef GAME_RENDER_SKY_H
#define GAME_RENDER_SKY_H

#include <vector>

#include <OgreVector3.h>
#include <OgreString.h>
#include <OgreMaterial.h>
#include <OgreColourValue.h>
#include <OgreHighLevelGpuProgram.h>

#include <extern/shiny/Main/Factory.hpp>

#include <components/nifogre/ogrenifloader.hpp>


#include "../mwworld/weather.hpp"

namespace Ogre
{
    class RenderWindow;
    class SceneNode;
    class Camera;
    class Viewport;
    class SceneManager;
    class Entity;
    class BillboardSet;
    class TextureUnitState;
}

namespace MWRender
{
    class BillboardObject : public sh::MaterialInstanceListener
    {
    public:
        BillboardObject(  const Ogre::String& textureName,
                        const float size,
                        const Ogre::Vector3& position,
                        Ogre::SceneNode* rootNode,
                          const std::string& material
                    );

        void requestedConfiguration (sh::MaterialInstance* m, const std::string& configuration);
        void createdConfiguration (sh::MaterialInstance* m, const std::string& configuration);

        virtual ~BillboardObject() {}

        void setColour(const Ogre::ColourValue& pColour);
        void setPosition(const Ogre::Vector3& pPosition);
        void setVisible(const bool visible);
        void setRenderQueue(unsigned int id);
        void setVisibilityFlags(int flags);
        void setSize(const float size);
        Ogre::Vector3 getPosition() const;

        void setVisibility(const float visibility);

        Ogre::SceneNode* getNode();

    protected:
        float mVisibility;
        Ogre::ColourValue mColour;
        Ogre::SceneNode* mNode;
        sh::MaterialInstance* mMaterial;
        Ogre::Entity* mEntity;
    };


    /*
     * The moons need a seperate class because of their shader (which allows them to be partially transparent)
     */
    class Moon : public BillboardObject
    {
    public:
        Moon(  const Ogre::String& textureName,
                        const float size,
                        const Ogre::Vector3& position,
                        Ogre::SceneNode* rootNode,
               const std::string& material
                    );

        virtual ~Moon() {}

        enum Phase
        {
            Phase_New = 0,
            Phase_WaxingCrescent,
            Phase_WaxingHalf,
            Phase_WaxingGibbous,
            Phase_Full,
            Phase_WaningGibbous,
            Phase_WaningHalf,
            Phase_WaningCrescent
        };

        enum Type
        {
            Type_Masser = 0,
            Type_Secunda
        };

        void setPhase(const Phase& phase);
        void setType(const Type& type);

        unsigned int getPhaseInt() const;

    private:
        Type mType;
        Phase mPhase;
    };

    class SkyManager
    {
    public:
        SkyManager(Ogre::SceneNode* root, Ogre::Camera* pCamera);
        ~SkyManager();

        /// Attach weather particle effects to this scene node (should be the Camera's parent node)
        void attachToNode(Ogre::SceneNode* sceneNode);

        void update(float duration);

        void enable();

        void disable();

        void setHour (double hour);
        ///< will be called even when sky is disabled.

        void setDate (int day, int month);
        ///< will be called even when sky is disabled.

        int getMasserPhase() const;
        ///< 0 new moon, 1 waxing or waning cresecent, 2 waxing or waning half,
        /// 3 waxing or waning gibbous, 4 full moon

        int getSecundaPhase() const;
        ///< 0 new moon, 1 waxing or waning cresecent, 2 waxing or waning half,
        /// 3 waxing or waning gibbous, 4 full moon

        void setMoonColour (bool red);
        ///< change Secunda colour to red

        void setWeather(const MWWorld::WeatherResult& weather);

        Ogre::SceneNode* getSunNode();

        void sunEnable();

        void sunDisable();

        void setRainSpeed(float speed);

        void setStormDirection(const Ogre::Vector3& direction);

        void setSunDirection(const Ogre::Vector3& direction, bool is_night);

        void setMasserDirection(const Ogre::Vector3& direction);

        void setSecundaDirection(const Ogre::Vector3& direction);

        void setMasserFade(const float fade);

        void setSecundaFade(const float fade);

        void masserEnable();
        void masserDisable();

        void secundaEnable();
        void secundaDisable();

        void setLightningStrength(const float factor);

        void setGlare(const float glare);
        void setGlareEnabled(bool enabled);
        Ogre::Vector3 getRealSunPos();

    private:
        void create();
        ///< no need to call this, automatically done on first enable()

        void updateRain(float dt);
        void clearRain();

        bool mCreated;

        bool mMoonRed;

        bool mIsStorm;

        float mHour;
        int mDay;
        int mMonth;

        float mCloudAnimationTimer;

        BillboardObject* mSun;
        BillboardObject* mSunGlare;
        Moon* mMasser;
        Moon* mSecunda;

        Ogre::Camera* mCamera;
        Ogre::SceneNode* mRootNode;
        Ogre::SceneManager* mSceneMgr;

        Ogre::SceneNode* mAtmosphereDay;
        Ogre::SceneNode* mAtmosphereNight;

        Ogre::SceneNode* mCloudNode;

        std::vector<NifOgre::ObjectScenePtr> mObjects;

        Ogre::SceneNode* mParticleNode;
        NifOgre::ObjectScenePtr mParticle;

        std::map<Ogre::SceneNode*, NifOgre::ObjectScenePtr> mRainModels;
        float mRainTimer;

        Ogre::Vector3 mStormDirection;

        // remember some settings so we don't have to apply them again if they didnt change
        Ogre::String mClouds;
        Ogre::String mNextClouds;
        float mCloudBlendFactor;
        float mCloudOpacity;
        float mCloudSpeed;
        float mStarsOpacity;
        Ogre::ColourValue mCloudColour;
        Ogre::ColourValue mSkyColour;
        Ogre::ColourValue mFogColour;

        std::string mCurrentParticleEffect;

        Ogre::Light* mLightning;

        float mRemainingTransitionTime;

        float mGlare; // target
        float mGlareFade; // actual

        bool mRainEnabled;
        std::string mRainEffect;
        float mRainSpeed;
        float mRainFrequency;

        bool mEnabled;
        bool mSunEnabled;
        bool mMasserEnabled;
        bool mSecundaEnabled;
    };
}

#endif // GAME_RENDER_SKY_H
