#ifndef _GAME_RENDER_SKY_H
#define _GAME_RENDER_SKY_H

#include <OgreVector3.h>
#include <OgreString.h>
#include <OgreMaterial.h>
#include <OgreColourValue.h>
#include <OgreHighLevelGpuProgram.h>

#include "sky.hpp"

namespace Ogre
{
    class RenderWindow;
    class SceneNode;
    class Camera;
    class Viewport;
    class SceneManager;
    class Entity;
}

namespace MWRender
{
   class CelestialBody
    {
    public:
        CelestialBody(  const Ogre::String& pTextureName,
                        const unsigned int pInitialSize,
                        const Ogre::Vector3& pInitialPosition,
                        Ogre::SceneNode* pRootNode
                    );
        CelestialBody();
                            
        void setPosition(const Ogre::Vector3& pPosition);
        void setVisible(const bool visible);
        
    protected:
        virtual void init(const Ogre::String& pTextureName,
                        const unsigned int pInitialSize,
                        const Ogre::Vector3& pInitialPosition,
                        Ogre::SceneNode* pRootNode);
    
        Ogre::SceneNode* mNode;
        Ogre::MaterialPtr mMaterial;
    };
    
    
    /*
     * The moons need a seperate class because of their shader (which allows them to be partially transparent)
     */
    class Moon : public CelestialBody
    {
    public:
        Moon(  const Ogre::String& pTextureName,
                        const unsigned int pInitialSize,
                        const Ogre::Vector3& pInitialPosition,
                        Ogre::SceneNode* pRootNode
                    );
    
        void setVisibility(const float pVisibility);
        ///< set the transparency factor for this moon
        
        void setColour(const Ogre::ColourValue& pColour);
                
        enum Phase
        {
            Phase_New = 0,
            Phase_WaxingCrescent,
            Phase_WaxingHalf,
            Phase_WaxingGibbous,
            Phase_WaningCrescent,
            Phase_WaningHalf,
            Phase_WaningGibbous,
            Phase_Full
        };
        
        enum Type
        {
            Type_Masser = 0,
            Type_Secunda
        };
        
        void setPhase(const Phase& phase);
        void setType(const Type& type);
        
        Phase getPhase() const;
        unsigned int getPhaseInt() const;
    
    private:
        Type mType;
        Phase mPhase;
    };
        
    class SkyManager
    {
    public:
        SkyManager(Ogre::SceneNode* pMwRoot, Ogre::Camera* pCamera);
        ~SkyManager();
        
        void update(float duration);
        
        void enable();
        
        void disable();
        
        void setHour (double hour) {}
        ///< will be called even when sky is disabled.
        
        void setDate (int day, int month) {}
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
        
    private:
        CelestialBody* mSun;
        Moon* mMasser;
        Moon* mSecunda;
    
        Ogre::Viewport* mViewport;
        Ogre::SceneNode* mRootNode;
        Ogre::SceneManager* mSceneMgr;
        
        Ogre::MaterialPtr mCloudMaterial;
        Ogre::MaterialPtr mAtmosphereMaterial;
        
        Ogre::HighLevelGpuProgramPtr mCloudFragmentShader;
        
        void ModVertexAlpha(Ogre::Entity* ent, unsigned int meshType);
    };
}

#endif // _GAME_RENDER_SKY_H
