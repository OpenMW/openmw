#ifndef _SKY_IMPL_H
#define _SKY_IMPL_H

#include <OgreVector3.h>
#include <OgreString.h>
#include <OgreMaterial.h>
#include <OgreColourValue.h>
#include <OgreHighLevelGpuProgram.h>

#include "sky.hpp"

namespace Ogre
{
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
        
        /// \todo Moon phases
    };
    
    
    class MWSkyManager : public SkyManager
    {
    public:
        MWSkyManager(Ogre::SceneNode* pMwRoot, Ogre::Camera* pCamera);
        virtual ~MWSkyManager();
        
        virtual void update(float duration);
        
        virtual void enable();
        
        virtual void disable();
        
        virtual void setHour (double hour) {}
        ///< will be called even when sky is disabled.
        
        virtual void setDate (int day, int month) {}
        ///< will be called even when sky is disabled.
        
        virtual int getMasserPhase() const { return 0; }
        ///< 0 new moon, 1 waxing or waning cresecent, 2 waxing or waning half,
        /// 3 waxing or waning gibbous, 4 full moon
        
        virtual int getSecundaPhase() const { return 0; }
        ///< 0 new moon, 1 waxing or waning cresecent, 2 waxing or waning half,
        /// 3 waxing or waning gibbous, 4 full moon
        
        virtual void setMoonColour (bool red);
        ///< change Secunda colour to red
        
    private:
        CelestialBody* mSun;
        Moon* mMasser;
        Moon* mSecunda;
    
        Ogre::Camera* mCamera;
        Ogre::Viewport* mViewport;
        Ogre::SceneNode* mRootNode;
        Ogre::SceneManager* mSceneMgr;
        
        Ogre::MaterialPtr mCloudMaterial;
        Ogre::MaterialPtr mAtmosphereMaterial;
        
        Ogre::HighLevelGpuProgramPtr mCloudFragmentShader;
        
        void ModVertexAlpha(Ogre::Entity* ent, unsigned int meshType);
    };
} // namespace

#endif // SKY_IMPL_H
