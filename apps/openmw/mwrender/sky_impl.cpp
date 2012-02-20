#include "sky_impl.hpp"

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
using namespace MWRender;

CelestialBody::CelestialBody( const String& textureName,
                    const unsigned int initialSize,
                    const Vector3& pInitialPosition,
                    SceneNode* pRootNode)
{
    init(textureName, initialSize, pInitialPosition, pRootNode);
}

CelestialBody::CelestialBody()
{
}

void CelestialBody::setVisible(const bool visible)
{
    mNode->setVisible(visible);
}

void CelestialBody::setPosition(const Vector3& pPosition)
{
    Vector3 finalPosition = pPosition.normalisedCopy() * CELESTIAL_BODY_DISTANCE;

    mNode->setPosition(finalPosition);
}

void CelestialBody::init(const String& textureName,
                    const unsigned int initialSize,
                    const Vector3& pInitialPosition,
                    SceneNode* pRootNode)
{
    SceneManager* sceneMgr = pRootNode->getCreator();
    
    const float scale = initialSize*550.f;
    
    Vector3 finalPosition = pInitialPosition.normalisedCopy() * CELESTIAL_BODY_DISTANCE;
    
    static unsigned int bodyCount=0;
    
    /// \todo These billboards are not 100% correct, might want to revisit them later
    BillboardSet* bbSet = sceneMgr->createBillboardSet("SkyBillboardSet"+StringConverter::toString(bodyCount), 1);
    bbSet->setDefaultDimensions(scale, scale);
    bbSet->setRenderQueueGroup(RENDER_QUEUE_SKIES_EARLY+1);
    bbSet->setBillboardType(BBT_PERPENDICULAR_COMMON);
    bbSet->setCommonDirection( -pInitialPosition.normalisedCopy() );
    mNode = pRootNode->createChildSceneNode();
    mNode->setPosition(finalPosition);
    mNode->attachObject(bbSet);
    bbSet->createBillboard(0,0,0);
    
    mMaterial = MaterialManager::getSingleton().create("CelestialBody"+StringConverter::toString(bodyCount), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    mMaterial->removeAllTechniques();
    Pass* p = mMaterial->createTechnique()->createPass();
    p->setSceneBlending(SBT_TRANSPARENT_ALPHA);
    p->setDepthCheckEnabled(false);
    p->setDepthWriteEnabled(false);
    p->setSelfIllumination(1.0,1.0,1.0);
    p->setDiffuse(0.0,0.0,0.0,1.0);
    p->setAmbient(0.0,0.0,0.0);
    p->createTextureUnitState(textureName);
    bbSet->setMaterialName("CelestialBody"+StringConverter::toString(bodyCount));
    
    bodyCount++;
}

Moon::Moon( const String& textureName,
                    const unsigned int initialSize,
                    const Vector3& pInitialPosition,
                    SceneNode* pRootNode)
{
    init(textureName, initialSize, pInitialPosition, pRootNode);

    HighLevelGpuProgramManager& mgr = HighLevelGpuProgramManager::getSingleton();
    HighLevelGpuProgramPtr vshader;
    if (mgr.resourceExists("Moon_VP"))
        vshader = mgr.getByName("Moon_VP");
    else
        vshader = mgr.createProgram("Moon_VP", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, "cg", GPT_VERTEX_PROGRAM);
    vshader->setParameter("profiles", "vs_2_x arbvp1");
    vshader->setParameter("entry_point", "main_vp");
    StringUtil::StrStreamType outStream;
    outStream <<
    "void main_vp(	\n"
    "	float4 position : POSITION,	\n"
    "   in float2 uv : TEXCOORD0, \n"
    "   out float2 oUV : TEXCOORD0, \n"
    "	out float4 oPosition : POSITION,	\n"
    "	uniform float4x4 worldViewProj	\n"
    ")	\n"
    "{	\n"
    "   oUV = uv; \n"
    "	oPosition = mul( worldViewProj, position );  \n"
    "}";
    vshader->setSource(outStream.str());
    vshader->load();
    vshader->getDefaultParameters()->setNamedAutoConstant("worldViewProj", GpuProgramParameters::ACT_WORLDVIEWPROJ_MATRIX);
    mMaterial->getTechnique(0)->getPass(0)->setVertexProgram(vshader->getName());
    
    HighLevelGpuProgramPtr fshader;
    if (mgr.resourceExists("Moon_FP"))
        fshader = mgr.getByName("Moon_FP");
    else
        fshader = mgr.createProgram("Moon_FP", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, "cg", GPT_FRAGMENT_PROGRAM);

    fshader->setParameter("profiles", "ps_2_x arbfp1");
    fshader->setParameter("entry_point", "main_fp");
    StringUtil::StrStreamType outStream2;
    outStream2 <<
    "void main_fp(	\n"
    "   in float2 uv : TEXCOORD0, \n"
    "	out float4 oColor    : COLOR, \n"
    "   uniform sampler2D texture : TEXUNIT0, \n"
    "   uniform float visibilityFactor, \n"
    "   uniform float4 emissive \n"
    ")	\n"
    "{	\n"
    "   float4 tex = tex2D(texture, uv); \n"
    "   oColor = float4(emissive.xyz,1) * tex2D(texture, uv) * float4(1,1,1,visibilityFactor); \n"
    "}";
    fshader->setSource(outStream2.str());
    fshader->load();
    fshader->getDefaultParameters()->setNamedAutoConstant("emissive", GpuProgramParameters::ACT_SURFACE_EMISSIVE_COLOUR);
    mMaterial->getTechnique(0)->getPass(0)->setFragmentProgram(fshader->getName());
}

void Moon::setVisibility(const float pVisibility)
{
    mMaterial->getTechnique(0)->getPass(0)->getFragmentProgramParameters()->setNamedConstant("visibilityFactor", Real(pVisibility));
}

void Moon::setColour(const ColourValue& pColour)
{
    mMaterial->getTechnique(0)->getPass(0)->setSelfIllumination(pColour);
}


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
    mMasser = new Moon("textures\\tx_masser_full.dds", 1, Vector3(-0.4, -0.4, 0.5), mRootNode);
    mSecunda = new Moon("textures\\tx_secunda_full.dds", 1, Vector3(0.4, -0.4, 0.5), mRootNode);
    mMasser->setVisibility(0.2);
    mSecunda->setVisibility(0.2);
    mMasser->setVisible(false);
    mSecunda->setVisible(false);
    
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
    clouds_ent->setRenderQueueGroup(RENDER_QUEUE_SKIES_EARLY+2);
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
    mAtmosphereMaterial->getTechnique(0)->getPass(0)->setDepthWriteEnabled(false);
    mAtmosphereMaterial->getTechnique(0)->getPass(0)->setSceneBlending(SBT_TRANSPARENT_ALPHA);
    mCloudMaterial->getTechnique(0)->getPass(0)->setSceneBlending(SBT_TRANSPARENT_ALPHA);
    
    mCamera = pCamera;
    mCamera->setFarClipDistance(500000.f);
}

MWSkyManager::~MWSkyManager()
{
    delete mSun;
    delete mMasser;
    delete mSecunda;
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

void MWSkyManager::setMoonColour (bool red)
{
    mSecunda->setColour( red ? ColourValue(1.0, 0.0, 0.0)
                            : ColourValue(1.0, 1.0, 1.0));
}
