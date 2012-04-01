#ifndef _GAME_RENDER_SKY_H
#define _GAME_RENDER_SKY_H

#include <OgreVector3.h>
#include <OgreString.h>
#include <OgreMaterial.h>
#include <OgreColourValue.h>
#include <OgreHighLevelGpuProgram.h>

#include "sky.hpp"
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
    class Overlay;
}

namespace MWRender
{
    class BillboardObject
    {
    public:
        BillboardObject(  const Ogre::String& textureName,
                        const float size,
                        const Ogre::Vector3& position,
                        Ogre::SceneNode* rootNode
                    );
        BillboardObject();

        virtual ~BillboardObject() {}
        
        void setColour(const Ogre::ColourValue& pColour);
        void setPosition(const Ogre::Vector3& pPosition);
        void setVisible(const bool visible);
        void setRenderQueue(unsigned int id);
        void setSize(const float size);
        Ogre::Vector3 getPosition() const;
        
        void setVisibility(const float visibility);
        
        Ogre::SceneNode* getNode();
        
    protected:
        virtual void init(const Ogre::String& textureName,
                        const float size,
                        const Ogre::Vector3& position,
                        Ogre::SceneNode* rootNode);
    
        Ogre::SceneNode* mNode;
        Ogre::MaterialPtr mMaterial;
        Ogre::BillboardSet* mBBSet;
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
                        Ogre::SceneNode* rootNode
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
        void setSkyColour(const Ogre::ColourValue& colour);
        
        Phase getPhase() const;
        unsigned int getPhaseInt() const;
    
    private:
        Type mType;
        Phase mPhase;
    };
        
    class SkyManager
    {
    public:
        SkyManager(Ogre::SceneNode* pMwRoot, Ogre::Camera* pCamera, MWWorld::Environment* env);
        ~SkyManager();

        void update(float duration);

        void create();
        ///< no need to call this, automatically done on first enable()

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

        void setCloudsOpacity(float opacity);
        ///< change opacity of the clouds

        void setWeather(const MWWorld::WeatherResult& weather);

        Ogre::SceneNode* getSunNode();

        void sunEnable();

        void sunDisable();

        void setSunDirection(const Ogre::Vector3& direction);

        void setMasserDirection(const Ogre::Vector3& direction);

        void setSecundaDirection(const Ogre::Vector3& direction);

        void setMasserFade(const float fade);

        void setSecundaFade(const float fade);

        void masserEnable();
        void masserDisable();

        void secundaEnable();
        void secundaDisable();

        void setThunder(const float factor);

        void setGlare(const float glare);
        Ogre::Vector3 getRealSunPos();

    private:
        bool mCreated;

        MWWorld::Environment* mEnvironment;
        float mHour;
        int mDay;
        int mMonth;

        BillboardObject* mSun;
        BillboardObject* mSunGlare;
        Moon* mMasser;
        Moon* mSecunda;
    
        Ogre::Viewport* mViewport;
        Ogre::SceneNode* mRootNode;
        Ogre::SceneManager* mSceneMgr;
        
        Ogre::SceneNode* mAtmosphereDay;
        Ogre::SceneNode* mAtmosphereNight;
        
        Ogre::MaterialPtr mCloudMaterial;
        Ogre::MaterialPtr mAtmosphereMaterial;
        
        Ogre::MaterialPtr mStarsMaterials[7];
        
        Ogre::HighLevelGpuProgramPtr mCloudFragmentShader;
        
        // remember some settings so we don't have to apply them again if they didnt change
        Ogre::String mClouds;
        Ogre::String mNextClouds;
        float mCloudBlendFactor;
        float mCloudOpacity;
        float mCloudSpeed;
        float mStarsOpacity;
        Ogre::ColourValue mCloudColour;
        Ogre::ColourValue mSkyColour;
        
        Ogre::Overlay* mThunderOverlay;
        Ogre::TextureUnitState* mThunderTextureUnit;
                
        float mRemainingTransitionTime;
        
        float mGlare; // target
        float mGlareFade; // actual
        
        void ModVertexAlpha(Ogre::Entity* ent, unsigned int meshType);
        
        bool mEnabled;
        bool mSunEnabled;
        bool mMasserEnabled;
        bool mSecundaEnabled;
    };
}

#endif // _GAME_RENDER_SKY_H
