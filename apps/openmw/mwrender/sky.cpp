#include "sky.hpp"

#include <OgreCamera.h>
#include <OgreRenderWindow.h>
#include <OgreSceneNode.h>
#include <OgreMesh.h>
#include <OgreSubMesh.h>
#include <OgreSceneManager.h>
#include <OgreHardwareVertexBuffer.h>
#include <OgreHighLevelGpuProgramManager.h>
#include <OgreBillboardSet.h>
#include <OgreEntity.h>
#include <OgreSubEntity.h>
#include <OgreOverlay.h>
#include <OgreOverlayManager.h>
#include <OgreOverlayContainer.h>

#include <boost/lexical_cast.hpp>

#include <components/nifogre/ogre_nif_loader.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "renderconst.hpp"
#include "renderingmanager.hpp"

using namespace MWRender;
using namespace Ogre;

BillboardObject::BillboardObject( const String& textureName,
                    const float initialSize,
                    const Vector3& position,
                    SceneNode* rootNode)
{
    init(textureName, initialSize, position, rootNode);
}

BillboardObject::BillboardObject()
{
}

void BillboardObject::setVisible(const bool visible)
{
    mBBSet->setVisible(visible);
}

void BillboardObject::setSize(const float size)
{
    mNode->setScale(size, size, size);
}

void BillboardObject::setVisibility(const float visibility)
{
    //mMaterial->getTechnique(0)->getPass(0)->setDiffuse(0.0, 0.0, 0.0, visibility);
}

void BillboardObject::setPosition(const Vector3& pPosition)
{
    Vector3 normalised = pPosition.normalisedCopy();
    Vector3 finalPosition = normalised * 1000.f;

    mBBSet->setCommonDirection( -normalised );

    mNode->setPosition(finalPosition);
}

Vector3 BillboardObject::getPosition() const
{
    Vector3 p = mNode->_getDerivedPosition() - mNode->getParentSceneNode()->_getDerivedPosition();
    return Vector3(p.x, -p.z, p.y);
}

void BillboardObject::setVisibilityFlags(int flags)
{
    mBBSet->setVisibilityFlags(flags);
}

void BillboardObject::setColour(const ColourValue& pColour)
{
    //mMaterial->getTechnique(0)->getPass(0)->setSelfIllumination(pColour);
}

void BillboardObject::setRenderQueue(unsigned int id)
{
    mBBSet->setRenderQueueGroup(id);
}

SceneNode* BillboardObject::getNode()
{
    return mNode;
}

void BillboardObject::init(const String& textureName,
                    const float initialSize,
                    const Vector3& position,
                    SceneNode* rootNode)
{
    SceneManager* sceneMgr = rootNode->getCreator();

    Vector3 finalPosition = position.normalisedCopy() * 1000.f;

    static unsigned int bodyCount=0;

    /// \todo These billboards are not 100% correct, might want to revisit them later
    mBBSet = sceneMgr->createBillboardSet("SkyBillboardSet"+StringConverter::toString(bodyCount), 1);
    mBBSet->setDefaultDimensions(550.f*initialSize, 550.f*initialSize);
    mBBSet->setBillboardType(BBT_PERPENDICULAR_COMMON);
    mBBSet->setCommonDirection( -position.normalisedCopy() );
    mBBSet->setVisibilityFlags(RV_Sky);
    mNode = rootNode->createChildSceneNode();
    mNode->setPosition(finalPosition);
    mNode->attachObject(mBBSet);
    mBBSet->createBillboard(0,0,0);
    mBBSet->setCastShadows(false);

    sh::MaterialInstance* m = sh::Factory::getInstance().createMaterialInstance ("BillboardMaterial"+StringConverter::toString(bodyCount), "openmw_sun");
    m->setProperty("texture", sh::makeProperty<sh::StringValue>(new sh::StringValue(textureName)));

    mBBSet->setMaterialName("BillboardMaterial"+StringConverter::toString(bodyCount));

    /*
    mMaterial = MaterialManager::getSingleton().create("BillboardMaterial"+StringConverter::toString(bodyCount), ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    mMaterial->removeAllTechniques();
    Pass* p = mMaterial->createTechnique()->createPass();
    p->setSceneBlending(SBT_TRANSPARENT_ALPHA);
    p->setDepthCheckEnabled(false);
    p->setDepthWriteEnabled(false);
    p->setSelfIllumination(1.0,1.0,1.0);
    p->setDiffuse(0.0,0.0,0.0,1.0);
    p->setAmbient(0.0,0.0,0.0);
    p->setPolygonModeOverrideable(false);
    p->createTextureUnitState(textureName);

    HighLevelGpuProgramManager& mgr = HighLevelGpuProgramManager::getSingleton();
    HighLevelGpuProgramPtr vshader;
    if (mgr.resourceExists("BBO_VP"))
        vshader = mgr.getByName("BBO_VP");
    else
        vshader = mgr.createProgram("BBO_VP", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, "cg", GPT_VERTEX_PROGRAM);
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
    if (mgr.resourceExists("BBO_FP"))
        fshader = mgr.getByName("BBO_FP");
    else
        fshader = mgr.createProgram("BBO_FP", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, "cg", GPT_FRAGMENT_PROGRAM);

    fshader->setParameter("profiles", "ps_2_x arbfp1");
    fshader->setParameter("entry_point", "main_fp");
    StringUtil::StrStreamType outStream2;
    outStream2 <<
    "void main_fp(	\n"
    "   in float2 uv : TEXCOORD0, \n"
    "	out float4 oColor    : COLOR, \n";
    if (RenderingManager::useMRT()) outStream2 <<
        "   out float4 oColor1 : COLOR1, \n";
    outStream2 <<
    "   uniform sampler2D texture : TEXUNIT0, \n"
    "   uniform float4 diffuse, \n"
    "   uniform float4 emissive \n"
    ")	\n"
    "{	\n"
    "   float4 tex = tex2D(texture, uv); \n"
    "   oColor = float4(emissive.xyz,1) * tex * float4(1,1,1,diffuse.a); \n";
    if (RenderingManager::useMRT()) outStream2 <<
        "   oColor1 = float4(1, 0, 0, 1); \n";
    outStream2 <<
    "}";
    fshader->setSource(outStream2.str());
    fshader->load();
    fshader->getDefaultParameters()->setNamedAutoConstant("diffuse", GpuProgramParameters::ACT_SURFACE_DIFFUSE_COLOUR);
    fshader->getDefaultParameters()->setNamedAutoConstant("emissive", GpuProgramParameters::ACT_SURFACE_EMISSIVE_COLOUR);
    mMaterial->getTechnique(0)->getPass(0)->setFragmentProgram(fshader->getName());
*/
    bodyCount++;
}

Moon::Moon( const String& textureName,
                    const float initialSize,
                    const Vector3& position,
                    SceneNode* rootNode)
{
    init(textureName, initialSize, position, rootNode);


    /*
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
    "	out float4 oColor    : COLOR, \n";
    if (RenderingManager::useMRT()) outStream2 <<
        "   out float4 oColor1 : COLOR1, \n";
    outStream2 <<
    "   uniform sampler2D texture : TEXUNIT0, \n"
    "   uniform float4 skyColour, \n"
    "   uniform float4 diffuse, \n"
    "   uniform float4 emissive \n"
    ")	\n"
    "{	\n"
    "   float4 tex = tex2D(texture, uv); \n"
    "   oColor = float4(emissive.xyz,1) * tex; \n";
    if (RenderingManager::useMRT()) outStream2 <<
        "   oColor1 = float4(1, 0, 0, 1); \n";
    outStream2 <<
    // use a circle for the alpha (compute UV distance to center)
    // looks a bit bad because its not filtered on the edges,
    // but it's cheaper than a seperate alpha texture.
    "   float sqrUVdist = pow(uv.x-0.5,2) + pow(uv.y-0.5, 2); \n"
    "   oColor.a = diffuse.a * (sqrUVdist >= 0.24 ? 0 : 1); \n"
    "   oColor.rgb += (1-tex.a) * oColor.a * skyColour.rgb; \n"//fill dark side of moon with skycolour
    "   oColor.rgb += (1-diffuse.a) * skyColour.rgb; \n"//fade bump
    "}";
    fshader->setSource(outStream2.str());
    fshader->load();
    fshader->getDefaultParameters()->setNamedAutoConstant("diffuse", GpuProgramParameters::ACT_SURFACE_DIFFUSE_COLOUR);
    fshader->getDefaultParameters()->setNamedAutoConstant("emissive", GpuProgramParameters::ACT_SURFACE_EMISSIVE_COLOUR);
    mMaterial->getTechnique(0)->getPass(0)->setFragmentProgram(fshader->getName());

    */

    setVisibility(1.0);

    mPhase = Moon::Phase_Full;
}

void Moon::setType(const Moon::Type& type)
{
    mType = type;
}

void Moon::setSkyColour(const Ogre::ColourValue& colour)
{
    //mMaterial->getTechnique(0)->getPass(0)->getFragmentProgramParameters()->setNamedConstant("skyColour", colour);
}

void Moon::setPhase(const Moon::Phase& phase)
{
    // Colour texture
    Ogre::String textureName = "textures\\tx_";

    if (mType == Moon::Type_Secunda) textureName += "secunda_";
    else textureName += "masser_";

    if      (phase == Moon::Phase_New)              textureName += "new";
    else if (phase == Moon::Phase_WaxingCrescent)   textureName += "one_wax";
    else if (phase == Moon::Phase_WaxingHalf)       textureName += "half_wax";
    else if (phase == Moon::Phase_WaxingGibbous)    textureName += "three_wax";
    else if (phase == Moon::Phase_WaningCrescent)   textureName += "one_wan";
    else if (phase == Moon::Phase_WaningHalf)       textureName += "half_wan";
    else if (phase == Moon::Phase_WaningGibbous)    textureName += "three_wan";
    else if (phase == Moon::Phase_Full)             textureName += "full";

    textureName += ".dds";

    //mMaterial->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(textureName);

    mPhase = phase;
}

Moon::Phase Moon::getPhase() const
{
    return mPhase;
}

unsigned int Moon::getPhaseInt() const
{
    if      (mPhase == Moon::Phase_New)              return 0;
    else if (mPhase == Moon::Phase_WaxingCrescent)   return 1;
    else if (mPhase == Moon::Phase_WaningCrescent)   return 1;
    else if (mPhase == Moon::Phase_WaxingHalf)       return 2;
    else if (mPhase == Moon::Phase_WaningHalf)       return 2;
    else if (mPhase == Moon::Phase_WaxingGibbous)    return 3;
    else if (mPhase == Moon::Phase_WaningGibbous)    return 3;
    else if (mPhase == Moon::Phase_Full)             return 4;

    return 0;
}

void SkyManager::ModVertexAlpha(Entity* ent, unsigned int meshType)
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

        unsigned char alpha=0;
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
        pData = static_cast<unsigned char *> (pData) + vertex_size;
    }

    // Unlock
    ent->getMesh()->getSubMesh(0)->vertexData->vertexBufferBinding->getBuffer(ves_diffuse->getSource())->unlock();
}

SkyManager::SkyManager (SceneNode* pMwRoot, Camera* pCamera)
    : mHour(0.0f)
    , mDay(0)
    , mMonth(0)
    , mSun(NULL)
    , mSunGlare(NULL)
    , mMasser(NULL)
    , mSecunda(NULL)
    , mCamera(pCamera)
    , mRootNode(NULL)
    , mSceneMgr(NULL)
    , mAtmosphereDay(NULL)
    , mAtmosphereNight(NULL)
    , mCloudFragmentShader()
    , mClouds()
    , mNextClouds()
    , mCloudBlendFactor(0.0f)
    , mCloudOpacity(0.0f)
    , mCloudSpeed(0.0f)
    , mStarsOpacity(0.0f)
    , mThunderOverlay(NULL)
    , mThunderTextureUnit(NULL)
    , mRemainingTransitionTime(0.0f)
    , mGlareFade(0.0f)
    , mGlare(0.0f)
    , mEnabled(true)
    , mSunEnabled(true)
    , mMasserEnabled(true)
    , mSecundaEnabled(true)
    , mCreated(false)
    , mCloudAnimationTimer(0.f)
{
    mSceneMgr = pMwRoot->getCreator();
    mRootNode = mCamera->getParentSceneNode()->createChildSceneNode();
    mRootNode->pitch(Degree(-90)); // convert MW to ogre coordinates
    mRootNode->setInheritOrientation(false);
}

void SkyManager::create()
{
    sh::Factory::getInstance().setSharedParameter ("cloudBlendFactor",
        sh::makeProperty<sh::FloatValue>(new sh::FloatValue(0)));
    sh::Factory::getInstance().setSharedParameter ("cloudOpacity",
        sh::makeProperty<sh::FloatValue>(new sh::FloatValue(1)));
    sh::Factory::getInstance().setSharedParameter ("cloudColour",
        sh::makeProperty<sh::Vector3>(new sh::Vector3(1,1,1)));
    sh::Factory::getInstance().setSharedParameter ("cloudAnimationTimer",
        sh::makeProperty<sh::FloatValue>(new sh::FloatValue(0)));
    sh::Factory::getInstance().setSharedParameter ("nightFade",
        sh::makeProperty<sh::FloatValue>(new sh::FloatValue(0)));

    sh::Factory::getInstance().setTextureAlias ("cloud_texture_1", "");
    sh::Factory::getInstance().setTextureAlias ("cloud_texture_2", "");

    // Create overlay used for thunderstorm
    MaterialPtr material = MaterialManager::getSingleton().create( "ThunderMaterial", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
    Pass* pass = material->getTechnique(0)->getPass(0);
    pass->setSceneBlending(SBT_TRANSPARENT_ALPHA);
    mThunderTextureUnit = pass->createTextureUnitState();
    mThunderTextureUnit->setColourOperationEx(LBX_SOURCE1, LBS_MANUAL, LBS_CURRENT, ColourValue(1.f, 1.f, 1.f));
    mThunderTextureUnit->setAlphaOperation(LBX_SOURCE1, LBS_MANUAL, LBS_CURRENT, 0.5f);
    OverlayManager& ovm = OverlayManager::getSingleton();
    mThunderOverlay = ovm.create( "ThunderOverlay" );
    OverlayContainer* overlay_panel;
    overlay_panel = (OverlayContainer*)ovm.createOverlayElement("Panel", "ThunderPanel");
    overlay_panel->_setPosition(0, 0);
    overlay_panel->_setDimensions(1, 1);
    overlay_panel->setMaterialName( "ThunderMaterial" );
    overlay_panel->show();
    mThunderOverlay->add2D(overlay_panel);
    mThunderOverlay->hide();

    mSecunda = new Moon("textures\\tx_secunda_full.dds", 0.5, Vector3(-0.4, 0.4, 0.5), mRootNode);
    mSecunda->setType(Moon::Type_Secunda);
    mSecunda->setRenderQueue(RQG_SkiesEarly+4);

    mMasser = new Moon("textures\\tx_masser_full.dds", 0.75, Vector3(-0.4, 0.4, 0.5), mRootNode);
    mMasser->setRenderQueue(RQG_SkiesEarly+3);
    mMasser->setType(Moon::Type_Masser);

    mSun = new BillboardObject("textures\\tx_sun_05.dds", 1, Vector3(0.4, 0.4, 0.4), mRootNode);
    mSun->setRenderQueue(RQG_SkiesEarly+4);
    mSunGlare = new BillboardObject("textures\\tx_sun_flash_grey_05.dds", 3, Vector3(0.4, 0.4, 0.4), mRootNode);
    mSunGlare->setRenderQueue(RQG_SkiesLate);
    mSunGlare->setVisibilityFlags(RV_Glare);

    // Stars
    MeshPtr mesh = NifOgre::NIFLoader::load("meshes\\sky_night_01.nif");
    Entity* night1_ent = mSceneMgr->createEntity("meshes\\sky_night_01.nif");
    night1_ent->setRenderQueueGroup(RQG_SkiesEarly+1);
    night1_ent->setVisibilityFlags(RV_Sky);
    night1_ent->setCastShadows(false);

    mAtmosphereNight = mRootNode->createChildSceneNode();
    mAtmosphereNight->attachObject(night1_ent);

    for (unsigned int i=0; i<night1_ent->getNumSubEntities(); ++i)
    {
        std::string matName = "openmw_stars_" + boost::lexical_cast<std::string>(i);
        sh::MaterialInstance* m = sh::Factory::getInstance ().createMaterialInstance (matName, "openmw_stars");

        std::string textureName = sh::retrieveValue<sh::StringValue>(
                    sh::Factory::getInstance().getMaterialInstance(night1_ent->getSubEntity (i)->getMaterialName ())->getProperty("diffuseMap"), NULL).get();

        m->setProperty ("texture", sh::makeProperty<sh::StringValue>(new sh::StringValue(textureName)));

        night1_ent->getSubEntity(i)->setMaterialName (matName);
    }

    // Atmosphere (day)
    mesh = NifOgre::NIFLoader::load("meshes\\sky_atmosphere.nif");
    Entity* atmosphere_ent = mSceneMgr->createEntity("meshes\\sky_atmosphere.nif");
    atmosphere_ent->setCastShadows(false);

    ModVertexAlpha(atmosphere_ent, 0);

    atmosphere_ent->setRenderQueueGroup(RQG_SkiesEarly);
    atmosphere_ent->setVisibilityFlags(RV_Sky);
    mAtmosphereDay = mRootNode->createChildSceneNode();
    mAtmosphereDay->attachObject(atmosphere_ent);
    atmosphere_ent->getSubEntity (0)->setMaterialName ("openmw_atmosphere");

    // Clouds
    NifOgre::NIFLoader::load("meshes\\sky_clouds_01.nif");
    Entity* clouds_ent = mSceneMgr->createEntity("meshes\\sky_clouds_01.nif");
    clouds_ent->setVisibilityFlags(RV_Sky);
    clouds_ent->setRenderQueueGroup(RQG_SkiesEarly+5);
    SceneNode* clouds_node = mRootNode->createChildSceneNode();
    clouds_node->attachObject(clouds_ent);
    clouds_ent->getSubEntity(0)->setMaterialName ("openmw_clouds");
    clouds_ent->setCastShadows(false);

    ModVertexAlpha(clouds_ent, 1);

    mCreated = true;
}

SkyManager::~SkyManager()
{
    delete mSun;
    delete mSunGlare;
    delete mMasser;
    delete mSecunda;
}

int SkyManager::getMasserPhase() const
{
    if (!mCreated) return 0;
    return mMasser->getPhaseInt();
}

int SkyManager::getSecundaPhase() const
{
    if (!mCreated) return 0;
    return mSecunda->getPhaseInt();
}

void SkyManager::update(float duration)
{
    if (!mEnabled) return;

    // UV Scroll the clouds
    mCloudAnimationTimer += duration * mCloudSpeed * (MWBase::Environment::get().getWorld()->getTimeScaleFactor()/30.f);
    sh::Factory::getInstance().setSharedParameter ("cloudAnimationTimer",
        sh::makeProperty<sh::FloatValue>(new sh::FloatValue(mCloudAnimationTimer)));


    /// \todo improve this
    mMasser->setPhase( static_cast<Moon::Phase>( (int) ((mDay % 32)/4.f)) );
    mSecunda->setPhase ( static_cast<Moon::Phase>( (int) ((mDay % 32)/4.f)) );


    if (mSunEnabled)
    {
        // take 1/10 sec for fading the glare effect from invisible to full
        if (mGlareFade > mGlare)
        {
            mGlareFade -= duration*10;
            if (mGlareFade < mGlare) mGlareFade = mGlare;
        }
        else if (mGlareFade < mGlare)
        {
            mGlareFade += duration*10;
            if (mGlareFade > mGlare) mGlareFade = mGlare;
        }

        // increase the strength of the sun glare effect depending
        // on how directly the player is looking at the sun
        Vector3 sun = mSunGlare->getPosition();
        sun = Vector3(sun.x, sun.z, -sun.y);
        Vector3 cam = mCamera->getRealDirection();
        const Degree angle = sun.angleBetween( cam );
        float val = 1- (angle.valueDegrees() / 180.f);
        val = (val*val*val*val)*2;

        mSunGlare->setSize(val * mGlareFade);
    }

    mSunGlare->setVisible(mSunEnabled);
    mSun->setVisible(mSunEnabled);
    mMasser->setVisible(mMasserEnabled);
    mSecunda->setVisible(mSecundaEnabled);

    // rotate the stars by 360 degrees every 4 days
    mAtmosphereNight->roll(Degree(MWBase::Environment::get().getWorld()->getTimeScaleFactor()*duration*360 / (3600*96.f)));
}

void SkyManager::enable()
{
    if (!mCreated)
        create();

    mRootNode->setVisible(true);
    mEnabled = true;
}

void SkyManager::disable()
{
    mRootNode->setVisible(false);
    mEnabled = false;
}

void SkyManager::setMoonColour (bool red)
{
    if (!mCreated) return;
    mSecunda->setColour( red ? ColourValue(1.0, 0.0784, 0.0784)
                            : ColourValue(1.0, 1.0, 1.0));
}

void SkyManager::setWeather(const MWWorld::WeatherResult& weather)
{
    if (!mCreated) return;

    if (mClouds != weather.mCloudTexture)
    {
        sh::Factory::getInstance().setTextureAlias ("cloud_texture_1", "textures\\"+weather.mCloudTexture);
        mClouds = weather.mCloudTexture;
    }

    if (mNextClouds != weather.mNextCloudTexture)
    {
        sh::Factory::getInstance().setTextureAlias ("cloud_texture_2", "textures\\"+weather.mNextCloudTexture);
        mNextClouds = weather.mNextCloudTexture;
    }

    if (mCloudBlendFactor != weather.mCloudBlendFactor)
    {
        mCloudBlendFactor = weather.mCloudBlendFactor;
        sh::Factory::getInstance().setSharedParameter ("cloudBlendFactor",
            sh::makeProperty<sh::FloatValue>(new sh::FloatValue(weather.mCloudBlendFactor)));
    }

    if (mCloudOpacity != weather.mCloudOpacity)
    {
        mCloudOpacity = weather.mCloudOpacity;
        sh::Factory::getInstance().setSharedParameter ("cloudOpacity",
            sh::makeProperty<sh::FloatValue>(new sh::FloatValue(weather.mCloudOpacity)));
    }

    if (mCloudColour != weather.mSunColor)
    {
        ColourValue clr( weather.mSunColor.r*0.7 + weather.mAmbientColor.r*0.7,
                        weather.mSunColor.g*0.7 + weather.mAmbientColor.g*0.7,
                        weather.mSunColor.b*0.7 + weather.mAmbientColor.b*0.7);

        sh::Factory::getInstance().setSharedParameter ("cloudColour",
            sh::makeProperty<sh::Vector3>(new sh::Vector3(clr.r, clr.g, clr.b)));

        mCloudColour = weather.mSunColor;
    }

    if (mSkyColour != weather.mSkyColor)
    {
        mSkyColour = weather.mSkyColor;
        sh::Factory::getInstance().setSharedParameter ("atmosphereColour", sh::makeProperty<sh::Vector4>(new sh::Vector4(
            weather.mSkyColor.r, weather.mSkyColor.g, weather.mSkyColor.b, weather.mSkyColor.a)));
    }

    mCloudSpeed = weather.mCloudSpeed;

    if (weather.mNight && mStarsOpacity != weather.mNightFade)
    {
        if (weather.mNightFade == 0)
            mAtmosphereNight->setVisible(false);
        else
        {
            mAtmosphereNight->setVisible(true);

            sh::Factory::getInstance().setSharedParameter ("nightFade",
                sh::makeProperty<sh::FloatValue>(new sh::FloatValue(weather.mNightFade)));

            mStarsOpacity = weather.mNightFade;
        }
    }


    float strength;
    float timeofday_angle = std::abs(mSunGlare->getPosition().z/mSunGlare->getPosition().length());
    if (timeofday_angle <= 0.44)
        strength = timeofday_angle/0.44f;
    else
        strength = 1.f;

    mSunGlare->setVisibility(weather.mGlareView * mGlareFade * strength);

    mSun->setVisibility(weather.mGlareView * strength);

    mAtmosphereNight->setVisible(weather.mNight && mEnabled);
}

void SkyManager::setGlare(const float glare)
{
    mGlare = glare;
}

Vector3 SkyManager::getRealSunPos()
{
    if (!mCreated) return Vector3(0,0,0);
    return mSun->getNode()->_getDerivedPosition();
}

void SkyManager::sunEnable()
{
    mSunEnabled = true;
}

void SkyManager::sunDisable()
{
    mSunEnabled = false;
}

void SkyManager::setSunDirection(const Vector3& direction)
{
    if (!mCreated) return;
    mSun->setPosition(direction);
    mSunGlare->setPosition(direction);
}

void SkyManager::setMasserDirection(const Vector3& direction)
{
    if (!mCreated) return;
    mMasser->setPosition(direction);
}

void SkyManager::setSecundaDirection(const Vector3& direction)
{
    if (!mCreated) return;
    mSecunda->setPosition(direction);
}

void SkyManager::masserEnable()
{
    mMasserEnabled = true;
}

void SkyManager::secundaEnable()
{
    mSecundaEnabled = true;
}

void SkyManager::masserDisable()
{
    mMasserEnabled = false;
}

void SkyManager::secundaDisable()
{
    mSecundaEnabled = false;
}

void SkyManager::setThunder(const float factor)
{
    if (!mCreated) return;
    if (factor > 0.f)
    {
        mThunderOverlay->show();
        mThunderTextureUnit->setAlphaOperation(LBX_SOURCE1, LBS_MANUAL, LBS_CURRENT, factor*0.6);
    }
    else
        mThunderOverlay->hide();
}

void SkyManager::setMasserFade(const float fade)
{
    if (!mCreated) return;
    mMasser->setVisibility(fade);
}

void SkyManager::setSecundaFade(const float fade)
{
    if (!mCreated) return;
    mSecunda->setVisibility(fade);
}

void SkyManager::setHour(double hour)
{
    mHour = hour;
}

void SkyManager::setDate(int day, int month)
{
    mDay = day;
    mMonth = month;
}

Ogre::SceneNode* SkyManager::getSunNode()
{
    if (!mCreated) return 0;
    return mSun->getNode();
}

void SkyManager::setSkyPosition(const Ogre::Vector3& position)
{
    mRootNode->_setDerivedPosition(position);
}

void SkyManager::resetSkyPosition()
{
    mRootNode->setPosition(0,0,0);
}

void SkyManager::scaleSky(float scale)
{
    mRootNode->setScale(scale, scale, scale);
}
