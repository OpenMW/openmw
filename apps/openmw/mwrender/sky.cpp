#include "sky.hpp"
#include "Caelum.h"

#include <OgreMesh.h>
#include <OgreSceneNode.h>
#include <OgreSceneManager.h>
#include <OgreCamera.h>
#include <OgreHardwareVertexBuffer.h>
#include <OgreHighLevelGpuProgramManager.h>

#include <components/nifogre/ogre_nif_loader.hpp>

// this distance has to be set accordingly so that the
// celestial bodies are behind the clouds, but in front of the atmosphere
#define CELESTIAL_BODY_DISTANCE 1000.f

using namespace Ogre;

namespace MWRender
{
    class CelestialBody
    {
    public:
        CelestialBody(  const String& pTextureName,
                        const unsigned int pInitialSize,
                        const Vector3& pInitialPosition,
                        SceneNode* pRootNode
                    );
                    
        void setPosition(const Vector3& pPosition);
        
    private:
        SceneNode* mNode;
    };
    
    CelestialBody::CelestialBody( const String& textureName,
                        const unsigned int initialSize,
                        const Vector3& pInitialPosition,
                        SceneNode* pRootNode)
    {
        SceneManager* sceneMgr = pRootNode->getCreator();
        
        const float scale = initialSize*700.f;
        
        Vector3 finalPosition = pInitialPosition.normalisedCopy() * CELESTIAL_BODY_DISTANCE;
        
        static unsigned int bodyCount=0;
        
        // Create a camera-aligned billboard
        BillboardSet* bbSet = sceneMgr->createBillboardSet("SkyBillboardSet"+StringConverter::toString(bodyCount), 1);
        bbSet->setDefaultDimensions(scale, scale);
        bbSet->setRenderQueueGroup(RENDER_QUEUE_SKIES_EARLY);
        SceneNode* mNode = pRootNode->createChildSceneNode();
        mNode->setPosition(finalPosition);
        mNode->attachObject(bbSet);
        bbSet->createBillboard(0,0,0);
        
        MaterialPtr material = MaterialManager::getSingleton().create("CelestialBody"+StringConverter::toString(bodyCount), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        material->removeAllTechniques();
        Pass* p = material->createTechnique()->createPass();
        p->setSceneBlending(SBT_TRANSPARENT_ALPHA);
        p->setDepthCheckEnabled(false);
        p->setDepthWriteEnabled(false);
        p->createTextureUnitState(textureName /*"textures\\tx_sun_05.dds"*/);
        bbSet->setMaterialName("CelestialBody"+StringConverter::toString(bodyCount));
        
        bodyCount++;

    }
    
    void CelestialBody::setPosition(const Vector3& pPosition)
    {
        Vector3 finalPosition = pPosition.normalisedCopy() * CELESTIAL_BODY_DISTANCE;

        mNode->setPosition(finalPosition);
    }
    
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
        
        virtual void setMoonColour (bool red) {}
        ///< change Secunda colour to red
        
    private:
        CelestialBody* mSun;
    
        Camera* mCamera;
        Viewport* mViewport;
        SceneNode* mRootNode;
        SceneManager* mSceneMgr;
        
        MaterialPtr mCloudMaterial;
        MaterialPtr mAtmosphereMaterial;
        
        HighLevelGpuProgramPtr mCloudFragmentShader;
        
        void ModVertexAlpha(Entity* ent, unsigned int meshType);
    };
    
    void MWSkyManager::ModVertexAlpha(Entity* ent, unsigned int meshType)
    {
        // Get the vertex colour buffer of this mesh
        const Ogre::VertexElement* ves_diffuse = ent->getMesh()->getSubMesh(0)->vertexData->vertexDeclaration->findElementBySemantic( Ogre::VES_DIFFUSE );
        HardwareVertexBufferSharedPtr colourBuffer = ent->getMesh()->getSubMesh(0)->vertexData->vertexBufferBinding->getBuffer(ves_diffuse->getSource());
        
        // Lock
        void* pData = colourBuffer->lock(HardwareBuffer::HBL_NORMAL);
        
        // Iterate over all vertices
        int vertex_size = colourBuffer->getVertexSize();
        float * currentVertex = NULL;
        for (unsigned int i=0; i<colourBuffer->getNumVertices(); ++i)
        {
            // Get a pointer to the vertex colour
            ves_diffuse->baseVertexPointerToElement( pData, &currentVertex );
            
            unsigned char alpha;
            if (meshType == 0) alpha = i%2 ? 0 : 255; // this is a cylinder, so every second vertex belongs to the bottom-most row
            else if (meshType == 1)
            {
                if (i>= 49 && i <= 64) alpha = 0; // bottom-most row
                else if (i>= 33 && i <= 48) alpha = 64; // second bottom-most row
                else alpha = 255;
            }
            
            uint8 tmpR = static_cast<uint8>(255);
            uint8 tmpG = static_cast<uint8>(255);
            uint8 tmpB = static_cast<uint8>(255);
            uint8 tmpA = static_cast<uint8>(alpha);
            
            // This does not matter since R and B are always 1.
            /*VertexElementType format = Root::getSingleton().getRenderSystem()->getColourVertexElementType();
            switch (format)
            {
            case VET_COLOUR_ARGB:
                std::swap(tmpR, tmpB);
                break;
            case VET_COLOUR_ABGR:
                break;
            default:
                break;
            }*/
             
            // Modify
            *((uint32*)currentVertex) = tmpR | (tmpG << 8) | (tmpB << 16) | (tmpA << 24);
            
            // Move to the next vertex
            pData+=vertex_size;
        }
        
        // Unlock
        ent->getMesh()->getSubMesh(0)->vertexData->vertexBufferBinding->getBuffer(ves_diffuse->getSource())->unlock();
    }
    
    MWSkyManager::MWSkyManager (SceneNode* pMwRoot, Camera* pCamera)
    {
        mViewport = pCamera->getViewport();
        mSceneMgr = pMwRoot->getCreator();
        mRootNode = pMwRoot->createChildSceneNode();
        mRootNode->setScale(100.f, 100.f, 100.f);

        mViewport->setBackgroundColour(ColourValue(0.87, 0.87, 0.87));
        
        mSun = new CelestialBody("textures\\tx_sun_05.dds", 1, Vector3(0.4, 0.4, 1.0), mRootNode);
        
        HighLevelGpuProgramManager& mgr = HighLevelGpuProgramManager::getSingleton();

        // Atmosphere
        MeshPtr mesh = NifOgre::NIFLoader::load("meshes\\sky_atmosphere.nif");        
        Entity* atmosphere_ent = mSceneMgr->createEntity("meshes\\sky_atmosphere.nif");
        
        ModVertexAlpha(atmosphere_ent, 0);
        
        atmosphere_ent->setRenderQueueGroup(RENDER_QUEUE_SKIES_EARLY);
        Ogre::SceneNode* atmosphere_node = mRootNode->createChildSceneNode();
        atmosphere_node->attachObject(atmosphere_ent);
        mAtmosphereMaterial = atmosphere_ent->getSubEntity(0)->getMaterial();
        
        // Atmosphere shader
		HighLevelGpuProgramPtr vshader = mgr.createProgram("Atmosphere_VP", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, 
			"cg", GPT_VERTEX_PROGRAM);

        vshader->setParameter("profiles", "vs_2_x arbvp1");
		vshader->setParameter("entry_point", "main_vp");
		
		StringUtil::StrStreamType outStream;
		outStream <<
		"void main_vp(	\n"
		"	float4 position : POSITION,	\n"
		"	in float4 color	: COLOR,	\n"
		"	out float4 oPosition : POSITION,	\n"
		"	out float4 oColor    : COLOR, \n"
        "   uniform float4 emissive, \n"
		"	uniform float4x4 worldViewProj	\n"
        ")	\n"
		"{	\n"
		"	oPosition = mul( worldViewProj, position );  \n"
        "   oColor = color * emissive; \n"
		"}";
		vshader->setSource(outStream.str());
		vshader->load();
		
		vshader->getDefaultParameters()->setNamedAutoConstant("worldViewProj", GpuProgramParameters::ACT_WORLDVIEWPROJ_MATRIX);
        vshader->getDefaultParameters()->setNamedAutoConstant("emissive", GpuProgramParameters::ACT_SURFACE_EMISSIVE_COLOUR);
        mAtmosphereMaterial->getTechnique(0)->getPass(0)->setVertexProgram(vshader->getName());

        // Clouds
        NifOgre::NIFLoader::load("meshes\\sky_clouds_01.nif");
        Entity* clouds_ent = mSceneMgr->createEntity("meshes\\sky_clouds_01.nif");
        clouds_ent->setRenderQueueGroup(RENDER_QUEUE_SKIES_EARLY+1);
        SceneNode* clouds_node = mRootNode->createChildSceneNode();
        clouds_node->attachObject(clouds_ent);
        mCloudMaterial = clouds_ent->getSubEntity(0)->getMaterial();
        
        // Clouds vertex shader
		HighLevelGpuProgramPtr vshader2 = mgr.createProgram("Clouds_VP", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, 
			"cg", GPT_VERTEX_PROGRAM);
        vshader2->setParameter("profiles", "vs_2_x arbvp1");
		vshader2->setParameter("entry_point", "main_vp");
		StringUtil::StrStreamType outStream3;
		outStream3 <<
		"void main_vp(	\n"
		"	float4 position : POSITION,	\n"
		"	in float4 color	: COLOR,	\n"
        "   out float4 oColor : TEXCOORD1, \n"
        "   in float2 uv : TEXCOORD0, \n"
        "   out float2 oUV : TEXCOORD0, \n"
		"	out float4 oPosition : POSITION,	\n"
		"	uniform float4x4 worldViewProj	\n"
        ")	\n"
		"{	\n"
        "   oUV = uv; \n"
        "   oColor = color; \n"
		"	oPosition = mul( worldViewProj, position );  \n"
		"}";
		vshader2->setSource(outStream3.str());
		vshader2->load();
		vshader2->getDefaultParameters()->setNamedAutoConstant("worldViewProj", GpuProgramParameters::ACT_WORLDVIEWPROJ_MATRIX);
        mCloudMaterial->getTechnique(0)->getPass(0)->setVertexProgram(vshader2->getName());
        
        // Clouds fragment shader
		mCloudFragmentShader = mgr.createProgram("Clouds_FP", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, 
			"cg", GPT_FRAGMENT_PROGRAM);
        mCloudFragmentShader->setParameter("profiles", "ps_2_x arbfp1");
		mCloudFragmentShader->setParameter("entry_point", "main_fp");
		StringUtil::StrStreamType outStream2;
		outStream2 <<
		"void main_fp(	\n"
        "   in float2 uv : TEXCOORD0, \n"
		"	out float4 oColor    : COLOR, \n"
        "   in float4 color : TEXCOORD1, \n"
        "   uniform sampler2D texture : TEXUNIT0, \n"
        "   uniform float time, \n"
        "   uniform float4 emissive \n"
        ")	\n"
		"{	\n"
        "   uv += float2(1,1) * time * 0.01; \n" // Scroll in x,y direction
        "   float4 tex = tex2D(texture, uv); \n"
        "   clip(tex.a<0.5); \n"
        "   oColor = color * float4(emissive.xyz,1) * tex2D(texture, uv); \n"
		"}";
		mCloudFragmentShader->setSource(outStream2.str());
		mCloudFragmentShader->load();
        mCloudFragmentShader->getDefaultParameters()->setNamedAutoConstant("emissive", GpuProgramParameters::ACT_SURFACE_EMISSIVE_COLOUR);
        mCloudMaterial->getTechnique(0)->getPass(0)->setFragmentProgram(mCloudFragmentShader->getName());
        
        ModVertexAlpha(clouds_ent, 1);
        
        // I'm not sure if the materials are being used by any other objects
        // Make a unique "modifiable" copy of the materials to be sure
        mCloudMaterial = mCloudMaterial->clone("Clouds");
        clouds_ent->getSubEntity(0)->setMaterial(mCloudMaterial);
        mAtmosphereMaterial = mAtmosphereMaterial->clone("Atmosphere");
        atmosphere_ent->getSubEntity(0)->setMaterial(mAtmosphereMaterial);
        
        // Default atmosphere color: light blue
        mAtmosphereMaterial->getTechnique(0)->getPass(0)->setSelfIllumination(0.235, 0.5, 0.73);
        mAtmosphereMaterial->getTechnique(0)->getPass(0)->setDiffuse(0.0, 0.0, 0.0, 0.0);
        mAtmosphereMaterial->getTechnique(0)->getPass(0)->setAmbient(0.0, 0.0, 0.0);
        // Set up an UV scroll animation to move the clouds
        mCloudMaterial->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setScrollAnimation(0.01f, 0.01f);
        mCloudMaterial->getTechnique(0)->getPass(0)->setSelfIllumination(1.0, 1.0, 1.0);
        // Disable depth writing so that the sky does not cover any objects
        mCloudMaterial->getTechnique(0)->getPass(0)->setDepthWriteEnabled(false);
        //mAtmosphereMaterial->getTechnique(0)->getPass(0)->setDepthWriteEnabled(false);
        mAtmosphereMaterial->getTechnique(0)->getPass(0)->setSceneBlending(SBT_TRANSPARENT_ALPHA);
        mCloudMaterial->getTechnique(0)->getPass(0)->setSceneBlending(SBT_TRANSPARENT_ALPHA);
        
        mCamera = pCamera;
        mCamera->setFarClipDistance(500000.f);
    }
    MWSkyManager::~MWSkyManager()
    {
        delete mSun;
    }
    
    void MWSkyManager::update(float duration)
    {
        // Sync the position of the skydomes with the camera
        /// \todo for some reason this is 1 frame delayed, which causes the skydome move funnily when the camera moves
        mRootNode->_setDerivedPosition(mCamera->getParentSceneNode()->_getDerivedPosition());
        
        // UV Scroll the clouds
        mCloudMaterial->getTechnique(0)->getPass(0)->getFragmentProgramParameters()->setNamedConstantFromTime("time", 1);
    }
    
    void MWSkyManager::enable()
    {
        mRootNode->setVisible(true);
    }
    
    void MWSkyManager::disable()
    {
        mRootNode->setVisible(false);
    }
    
    
    //
    // Implements a Caelum sky with default settings.
    //
    // Note: this is intended as a temporary solution to provide some form of 
    // sky rendering.  This code will obviously need significant tailoring to
    // support fidelity with Morrowind's rendering.  Before doing major work
    // on this class, more research should be done to determine whether
    // Caelum or another plug-in such as SkyX would be best for the long-term.
    //
    class CaelumManager : public SkyManager
    {
    protected:
        Caelum::CaelumSystem*   mpCaelumSystem;

    public:
                 CaelumManager (Ogre::RenderWindow* pRenderWindow, 
                                   Ogre::Camera* pCamera,
                                   const boost::filesystem::path& resDir);
        virtual ~CaelumManager ();
        
        virtual void update(float duration) {}
        
        virtual void enable() {}
        
        virtual void disable() {}
        
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
        
        virtual void setMoonColour (bool red) {}
    };

    CaelumManager::CaelumManager (Ogre::RenderWindow* pRenderWindow, 
                                  Ogre::Camera* pCamera,
                                  const boost::filesystem::path& resDir)
        : mpCaelumSystem        (NULL)
    {
        using namespace Caelum;

        assert(pCamera);
        assert(pRenderWindow);

        // Load the Caelum resources
        //
        ResourceGroupManager::getSingleton().addResourceLocation((resDir / "caelum").string(), "FileSystem", "Caelum");
        ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

        // Load the Caelum resources
        //
        Ogre::SceneManager* pScene = pCamera->getSceneManager();
        Caelum::CaelumSystem::CaelumComponent componentMask = CaelumSystem::CAELUM_COMPONENTS_DEFAULT;
        mpCaelumSystem = new Caelum::CaelumSystem (Root::getSingletonPtr(), pScene, componentMask);
        
        // Set time acceleration.
        mpCaelumSystem->getUniversalClock()->setTimeScale(128);       

        // Disable fog since OpenMW is handling OGRE fog elsewhere
        mpCaelumSystem->setManageSceneFog(false);

        // Change the camera far distance to make sure the sky is not clipped
        pCamera->setFarClipDistance(50000);

        // Register Caelum as an OGRE listener
        pRenderWindow->addListener(mpCaelumSystem);
        Root::getSingletonPtr()->addFrameListener(mpCaelumSystem);
    }

    CaelumManager::~CaelumManager() 
    {
        if (mpCaelumSystem) 
            mpCaelumSystem->shutdown (false);
    }

    /// Creates and connects the sky rendering component to OGRE.
    ///
    /// \return NULL on failure.
    /// 
    SkyManager* SkyManager::create (Ogre::RenderWindow* pRenderWindow, 
                                    Ogre::Camera*       pCamera,
                                    Ogre::SceneNode* pMwRoot,
                                    const boost::filesystem::path& resDir)
    {
        SkyManager* pSkyManager = NULL;

        //try
        //{
            //pSkyManager = new CaelumManager(pRenderWindow, pCamera, resDir);
            pSkyManager = new MWSkyManager(pMwRoot, pCamera);
        //}
        /*catch (Ogre::Exception& e)
        {
            std::cout << "\nOGRE Exception when attempting to add sky: " 
                << e.getFullDescription().c_str() << std::endl;
        }
        catch (std::exception& e)
        {
            std::cout << "\nException when attempting to add sky: " 
                << e.what() << std::endl;
        }*/

        return pSkyManager;
    }
} 
