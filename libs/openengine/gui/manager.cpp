#include "manager.hpp"
#include "loglistener.hpp"

#include <MyGUI_Gui.h>
#include <MyGUI_OgrePlatform.h>
#include <MyGUI_Timer.h>

#include <MyGUI_LevelLogFilter.h>
#include <MyGUI_LogSource.h>
#include <MyGUI_ConsoleLogListener.h>

#include <cassert>

#include <extern/shiny/Main/Factory.hpp>
#include <extern/shiny/Platforms/Ogre/OgreMaterial.hpp>

using namespace OEngine::GUI;

namespace MyGUI
{

/*
 *  As of MyGUI 3.2.0, MyGUI::OgreDataManager::isDataExist is unnecessarily complex
 *  this override fixes the resulting performance issue.
 */
// Remove for MyGUI 3.2.2
class FixedOgreDataManager : public MyGUI::OgreDataManager
{
public:
    bool isDataExist(const std::string& _name)
    {
        return Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup (_name);
    }
};


/*
 * As of MyGUI 3.2.0, rendering with shaders is not supported.
 * We definitely need this though to run in GL3 core / DX11 at all.
 * To make matters worse, subclassing OgreRenderManager doesn't seem to be possible here. :/
 */
class ShaderBasedRenderManager : 		public RenderManager,
        public IRenderTarget,
        public Ogre::WindowEventListener,
        public Ogre::RenderQueueListener,
        public Ogre::RenderSystem::Listener
{
    // флаг для обновления всех и вся
    bool mUpdate;

    IntSize mViewSize;

    Ogre::SceneManager* mSceneManager;

    VertexColourType mVertexFormat;

    // окно, на которое мы подписываемся для изменения размеров
    Ogre::RenderWindow* mWindow;

    // вьюпорт, с которым работает система
    unsigned short mActiveViewport;

    Ogre::RenderSystem* mRenderSystem;
    Ogre::TextureUnitState::UVWAddressingMode mTextureAddressMode;
    Ogre::LayerBlendModeEx mColorBlendMode, mAlphaBlendMode;

    RenderTargetInfo mInfo;

    typedef std::map<std::string, ITexture*> MapTexture;
    MapTexture mTextures;

    bool mIsInitialise;
    bool mManualRender;
    size_t mCountBatch;

    // ADDED
    Ogre::GpuProgram* mVertexProgramNoTexture;
    Ogre::GpuProgram* mVertexProgramOneTexture;
    Ogre::GpuProgram* mFragmentProgramNoTexture;
    Ogre::GpuProgram* mFragmentProgramOneTexture;

public:
    ShaderBasedRenderManager& getInstance()
    {
        return *getInstancePtr();
    }
    ShaderBasedRenderManager* getInstancePtr()
    {
        return static_cast<ShaderBasedRenderManager*>(RenderManager::getInstancePtr());
    }

    ShaderBasedRenderManager() :
        mUpdate(false),
        mSceneManager(nullptr),
        mWindow(nullptr),
        mActiveViewport(0),
        mRenderSystem(nullptr),
        mIsInitialise(false),
        mManualRender(false),
        mCountBatch(0),
        mVertexProgramNoTexture(NULL),
        mVertexProgramOneTexture(NULL),
        mFragmentProgramNoTexture(NULL),
        mFragmentProgramOneTexture(NULL)
    {
        mTextureAddressMode.u = Ogre::TextureUnitState::TAM_CLAMP;
        mTextureAddressMode.v = Ogre::TextureUnitState::TAM_CLAMP;
        mTextureAddressMode.w = Ogre::TextureUnitState::TAM_CLAMP;
    }

    void initialise(Ogre::RenderWindow* _window, Ogre::SceneManager* _scene)
    {
        MYGUI_PLATFORM_ASSERT(!mIsInitialise, getClassTypeName() << " initialised twice");
        MYGUI_PLATFORM_LOG(Info, "* Initialise: " << getClassTypeName());

        mColorBlendMode.blendType = Ogre::LBT_COLOUR;
        mColorBlendMode.source1 = Ogre::LBS_TEXTURE;
        mColorBlendMode.source2 = Ogre::LBS_DIFFUSE;
        mColorBlendMode.operation = Ogre::LBX_MODULATE;

        mAlphaBlendMode.blendType = Ogre::LBT_ALPHA;
        mAlphaBlendMode.source1 = Ogre::LBS_TEXTURE;
        mAlphaBlendMode.source2 = Ogre::LBS_DIFFUSE;
        mAlphaBlendMode.operation = Ogre::LBX_MODULATE;

        mTextureAddressMode.u = Ogre::TextureUnitState::TAM_CLAMP;
        mTextureAddressMode.v = Ogre::TextureUnitState::TAM_CLAMP;
        mTextureAddressMode.w = Ogre::TextureUnitState::TAM_CLAMP;

        mSceneManager = nullptr;
        mWindow = nullptr;
        mUpdate = false;
        mRenderSystem = nullptr;
        mActiveViewport = 0;

        Ogre::Root* root = Ogre::Root::getSingletonPtr();
        if (root != nullptr)
            setRenderSystem(root->getRenderSystem());
        setRenderWindow(_window);
        setSceneManager(_scene);


        MYGUI_PLATFORM_LOG(Info, getClassTypeName() << " successfully initialized");
        mIsInitialise = true;
    }

    void shutdown()
    {
        MYGUI_PLATFORM_ASSERT(mIsInitialise, getClassTypeName() << " is not initialised");
        MYGUI_PLATFORM_LOG(Info, "* Shutdown: " << getClassTypeName());

        destroyAllResources();

        setSceneManager(nullptr);
        setRenderWindow(nullptr);
        setRenderSystem(nullptr);

        MYGUI_PLATFORM_LOG(Info, getClassTypeName() << " successfully shutdown");
        mIsInitialise = false;
    }

    void setRenderSystem(Ogre::RenderSystem* _render)
    {
        // отписываемся
        if (mRenderSystem != nullptr)
        {
            mRenderSystem->removeListener(this);
            mRenderSystem = nullptr;
        }

        mRenderSystem = _render;

        // подписываемся на рендер евент
        if (mRenderSystem != nullptr)
        {
            mRenderSystem->addListener(this);

            // формат цвета в вершинах
            Ogre::VertexElementType vertex_type = mRenderSystem->getColourVertexElementType();
            if (vertex_type == Ogre::VET_COLOUR_ARGB)
                mVertexFormat = VertexColourType::ColourARGB;
            else if (vertex_type == Ogre::VET_COLOUR_ABGR)
                mVertexFormat = VertexColourType::ColourABGR;

            updateRenderInfo();
        }
    }

    Ogre::RenderSystem* getRenderSystem()
    {
        return mRenderSystem;
    }

    void setRenderWindow(Ogre::RenderWindow* _window)
    {
        // отписываемся
        if (mWindow != nullptr)
        {
            Ogre::WindowEventUtilities::removeWindowEventListener(mWindow, this);
            mWindow = nullptr;
        }

        mWindow = _window;

        if (mWindow != nullptr)
        {
            Ogre::WindowEventUtilities::addWindowEventListener(mWindow, this);
            windowResized(mWindow);
        }
    }

    void setSceneManager(Ogre::SceneManager* _scene)
    {
        if (nullptr != mSceneManager)
        {
            mSceneManager->removeRenderQueueListener(this);
            mSceneManager = nullptr;
        }

        mSceneManager = _scene;

        if (nullptr != mSceneManager)
        {
            mSceneManager->addRenderQueueListener(this);
        }
    }

    void setActiveViewport(unsigned short _num)
    {
        mActiveViewport = _num;

        if (mWindow != nullptr)
        {
            Ogre::WindowEventUtilities::removeWindowEventListener(mWindow, this);
            Ogre::WindowEventUtilities::addWindowEventListener(mWindow, this);

            // рассылка обновлений
            windowResized(mWindow);
        }
    }

    void renderQueueStarted(Ogre::uint8 queueGroupId, const Ogre::String& invocation, bool& skipThisInvocation)
    {
        Gui* gui = Gui::getInstancePtr();
        if (gui == nullptr)
            return;

        if (Ogre::RENDER_QUEUE_OVERLAY != queueGroupId)
            return;

        Ogre::Viewport* viewport = mSceneManager->getCurrentViewport();
        if (nullptr == viewport
            || !viewport->getOverlaysEnabled())
            return;

        if (mWindow->getNumViewports() <= mActiveViewport
            || viewport != mWindow->getViewport(mActiveViewport))
            return;

        mCountBatch = 0;

        static Timer timer;
        static unsigned long last_time = timer.getMilliseconds();
        unsigned long now_time = timer.getMilliseconds();
        unsigned long time = now_time - last_time;

        onFrameEvent((float)((double)(time) / (double)1000));

        last_time = now_time;

        //begin();
        setManualRender(true);
        onRenderToTarget(this, mUpdate);
        //end();

        // сбрасываем флаг
        mUpdate = false;
    }

    void renderQueueEnded(Ogre::uint8 queueGroupId, const Ogre::String& invocation, bool& repeatThisInvocation)
    {
    }

    void eventOccurred(const Ogre::String& eventName, const Ogre::NameValuePairList* parameters)
    {
        if (eventName == "DeviceLost")
        {
        }
        else if (eventName == "DeviceRestored")
        {
            // обновить всех
            mUpdate = true;
        }
    }

    IVertexBuffer* createVertexBuffer()
    {
        return new OgreVertexBuffer();
    }

    void destroyVertexBuffer(IVertexBuffer* _buffer)
    {
        delete _buffer;
    }

    // для оповещений об изменении окна рендера
    void windowResized(Ogre::RenderWindow* _window)
    {
        if (_window->getNumViewports() > mActiveViewport)
        {
            Ogre::Viewport* port = _window->getViewport(mActiveViewport);
#if OGRE_VERSION >= MYGUI_DEFINE_VERSION(1, 7, 0) && OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
            Ogre::OrientationMode orient = port->getOrientationMode();
            if (orient == Ogre::OR_DEGREE_90 || orient == Ogre::OR_DEGREE_270)
                mViewSize.set(port->getActualHeight(), port->getActualWidth());
            else
                mViewSize.set(port->getActualWidth(), port->getActualHeight());
#else
            mViewSize.set(port->getActualWidth(), port->getActualHeight());
#endif

            // обновить всех
            mUpdate = true;

            updateRenderInfo();

            onResizeView(mViewSize);
        }
    }

    void updateRenderInfo()
    {
        if (mRenderSystem != nullptr)
        {
            mInfo.maximumDepth = mRenderSystem->getMaximumDepthInputValue();
            mInfo.hOffset = mRenderSystem->getHorizontalTexelOffset() / float(mViewSize.width);
            mInfo.vOffset = mRenderSystem->getVerticalTexelOffset() / float(mViewSize.height);
            mInfo.aspectCoef = float(mViewSize.height) / float(mViewSize.width);
            mInfo.pixScaleX = 1.0f / float(mViewSize.width);
            mInfo.pixScaleY = 1.0f / float(mViewSize.height);
        }
    }

    void initShaders()
    {
        // ADDED
        sh::MaterialInstance* mat = sh::Factory::getInstance().getMaterialInstance("MyGUI/NoTexture");
        sh::Factory::getInstance()._ensureMaterial("MyGUI/NoTexture", "Default");
        mVertexProgramNoTexture = static_cast<sh::OgreMaterial*>(mat->getMaterial())->getOgreTechniqueForConfiguration("Default")->getPass(0)
                ->getVertexProgram()->_getBindingDelegate();

        mat = sh::Factory::getInstance().getMaterialInstance("MyGUI/OneTexture");
        sh::Factory::getInstance()._ensureMaterial("MyGUI/OneTexture", "Default");
        mVertexProgramOneTexture = static_cast<sh::OgreMaterial*>(mat->getMaterial())->getOgreTechniqueForConfiguration("Default")->getPass(0)
                ->getVertexProgram()->_getBindingDelegate();

        mat = sh::Factory::getInstance().getMaterialInstance("MyGUI/NoTexture");
        sh::Factory::getInstance()._ensureMaterial("MyGUI/NoTexture", "Default");
        mFragmentProgramNoTexture = static_cast<sh::OgreMaterial*>(mat->getMaterial())->getOgreTechniqueForConfiguration("Default")->getPass(0)
                ->getFragmentProgram()->_getBindingDelegate();

        mat = sh::Factory::getInstance().getMaterialInstance("MyGUI/OneTexture");
        sh::Factory::getInstance()._ensureMaterial("MyGUI/OneTexture", "Default");
        mFragmentProgramOneTexture = static_cast<sh::OgreMaterial*>(mat->getMaterial())->getOgreTechniqueForConfiguration("Default")->getPass(0)
                ->getFragmentProgram()->_getBindingDelegate();
    }

    void doRender(IVertexBuffer* _buffer, ITexture* _texture, size_t _count)
    {
        if (getManualRender())
        {
            begin();
            setManualRender(false);
        }

        // ADDED
        if (!mVertexProgramNoTexture)
            initShaders();

        if (_texture)
        {
            Ogre::Root::getSingleton().getRenderSystem()->bindGpuProgram(mVertexProgramOneTexture);
            Ogre::Root::getSingleton().getRenderSystem()->bindGpuProgram(mFragmentProgramOneTexture);
        }
        else
        {
            Ogre::Root::getSingleton().getRenderSystem()->bindGpuProgram(mVertexProgramNoTexture);
            Ogre::Root::getSingleton().getRenderSystem()->bindGpuProgram(mFragmentProgramNoTexture);
        }

        if (_texture)
        {
            OgreTexture* texture = static_cast<OgreTexture*>(_texture);
            Ogre::TexturePtr texture_ptr = texture->getOgreTexture();
            if (!texture_ptr.isNull())
            {
                mRenderSystem->_setTexture(0, true, texture_ptr);
                mRenderSystem->_setTextureUnitFiltering(0, Ogre::FO_LINEAR, Ogre::FO_LINEAR, Ogre::FO_NONE);
            }
        }

        OgreVertexBuffer* buffer = static_cast<OgreVertexBuffer*>(_buffer);
        Ogre::RenderOperation* operation = buffer->getRenderOperation();
        operation->vertexData->vertexCount = _count;

        mRenderSystem->_render(*operation);

        ++ mCountBatch;
    }

    void begin()
    {
        // set-up matrices
        mRenderSystem->_setWorldMatrix(Ogre::Matrix4::IDENTITY);
        mRenderSystem->_setViewMatrix(Ogre::Matrix4::IDENTITY);

#if OGRE_VERSION >= MYGUI_DEFINE_VERSION(1, 7, 0) && OGRE_NO_VIEWPORT_ORIENTATIONMODE == 0
        Ogre::OrientationMode orient = mWindow->getViewport(mActiveViewport)->getOrientationMode();
        mRenderSystem->_setProjectionMatrix(Ogre::Matrix4::IDENTITY * Ogre::Quaternion(Ogre::Degree(orient * 90.f), Ogre::Vector3::UNIT_Z));
#else
        mRenderSystem->_setProjectionMatrix(Ogre::Matrix4::IDENTITY);
#endif

        // initialise render settings
        mRenderSystem->setLightingEnabled(false);
        mRenderSystem->_setDepthBufferParams(false, false);
        mRenderSystem->_setDepthBias(0, 0);
        mRenderSystem->_setCullingMode(Ogre::CULL_NONE);
        mRenderSystem->_setFog(Ogre::FOG_NONE);
        mRenderSystem->_setColourBufferWriteEnabled(true, true, true, true);
        mRenderSystem->unbindGpuProgram(Ogre::GPT_FRAGMENT_PROGRAM);
        mRenderSystem->unbindGpuProgram(Ogre::GPT_VERTEX_PROGRAM);
        mRenderSystem->setShadingType(Ogre::SO_GOURAUD);

        // initialise texture settings
        mRenderSystem->_setTextureCoordCalculation(0, Ogre::TEXCALC_NONE);
        mRenderSystem->_setTextureCoordSet(0, 0);
        mRenderSystem->_setTextureUnitFiltering(0, Ogre::FO_LINEAR, Ogre::FO_LINEAR, Ogre::FO_NONE);
        mRenderSystem->_setTextureAddressingMode(0, mTextureAddressMode);
        mRenderSystem->_setTextureMatrix(0, Ogre::Matrix4::IDENTITY);
#if OGRE_VERSION < MYGUI_DEFINE_VERSION(1, 6, 0)
        mRenderSystem->_setAlphaRejectSettings(Ogre::CMPF_ALWAYS_PASS, 0);
#else
        mRenderSystem->_setAlphaRejectSettings(Ogre::CMPF_ALWAYS_PASS, 0, false);
#endif
        mRenderSystem->_setTextureBlendMode(0, mColorBlendMode);
        mRenderSystem->_setTextureBlendMode(0, mAlphaBlendMode);
        mRenderSystem->_disableTextureUnitsFrom(1);

        // enable alpha blending
        mRenderSystem->_setSceneBlending(Ogre::SBF_SOURCE_ALPHA, Ogre::SBF_ONE_MINUS_SOURCE_ALPHA);

        // always use wireframe
        mRenderSystem->_setPolygonMode(Ogre::PM_SOLID);
    }

    void end()
    {
    }

    ITexture* createTexture(const std::string& _name)
    {
        MapTexture::const_iterator item = mTextures.find(_name);
        MYGUI_PLATFORM_ASSERT(item == mTextures.end(), "Texture '" << _name << "' already exist");

        OgreTexture* texture = new OgreTexture(_name, OgreDataManager::getInstance().getGroup());
        mTextures[_name] = texture;
        return texture;
    }

    void destroyTexture(ITexture* _texture)
    {
        if (_texture == nullptr) return;

        MapTexture::iterator item = mTextures.find(_texture->getName());
        MYGUI_PLATFORM_ASSERT(item != mTextures.end(), "Texture '" << _texture->getName() << "' not found");

        mTextures.erase(item);
        delete _texture;
    }

    ITexture* getTexture(const std::string& _name)
    {
        MapTexture::const_iterator item = mTextures.find(_name);
        if (item == mTextures.end())
        {
            Ogre::TexturePtr texture = (Ogre::TexturePtr)Ogre::TextureManager::getSingleton().getByName(_name);
            if (!texture.isNull())
            {
                ITexture* result = createTexture(_name);
                static_cast<OgreTexture*>(result)->setOgreTexture(texture);
                return result;
            }
            return nullptr;
        }
        return item->second;
    }

    bool isFormatSupported(PixelFormat _format, TextureUsage _usage)
    {
        return Ogre::TextureManager::getSingleton().isFormatSupported(
            Ogre::TEX_TYPE_2D,
            OgreTexture::convertFormat(_format),
            OgreTexture::convertUsage(_usage));
    }

    void destroyAllResources()
    {
        for (MapTexture::const_iterator item = mTextures.begin(); item != mTextures.end(); ++item)
        {
            delete item->second;
        }
        mTextures.clear();
    }

#if MYGUI_DEBUG_MODE == 1
    bool checkTexture(ITexture* _texture)
    {
        for (MapTexture::const_iterator item = mTextures.begin(); item != mTextures.end(); ++item)
        {
            if (item->second == _texture)
                return true;
        }
        return false;
    }
#endif

    const IntSize& getViewSize() const
    {
        return mViewSize;
    }

    VertexColourType getVertexFormat()
    {
        return mVertexFormat;
    }

    const RenderTargetInfo& getInfo()
    {
        return mInfo;
    }

    size_t getActiveViewport()
    {
        return mActiveViewport;
    }

    Ogre::RenderWindow* getRenderWindow()
    {
        return mWindow;
    }

    bool getManualRender()
    {
        return mManualRender;
    }

    void setManualRender(bool _value)
    {
        mManualRender = _value;
    }

    size_t getBatchCount() const
    {
        return mCountBatch;
    }
};

/// \brief Helper class holding data that required during
/// MyGUI log creation
class LogFacility
{
    ConsoleLogListener  mConsole;
    CustomLogListener   mFile;
    LevelLogFilter      mFilter;
    LogSource           mSource;

public:

    LogFacility(const std::string &output, bool console)
      : mFile(output)
    {
        mConsole.setEnabled(console);
        mFilter.setLoggingLevel(LogLevel::Info);

        mSource.addLogListener(&mFile);
        mSource.addLogListener(&mConsole);
        mSource.setLogFilter(&mFilter);

        mSource.open();
    }

    LogSource *getSource() { return &mSource; }
};

}

void MyGUIManager::setup(Ogre::RenderWindow *wnd, Ogre::SceneManager *mgr, bool logging, const std::string& logDir)
{
    assert(wnd);
    assert(mgr);

    mSceneMgr = mgr;
    mShaderRenderManager = NULL;
    mRenderManager = NULL;

    using namespace MyGUI;

    // Enable/disable MyGUI logging to stdout. (Logging to MyGUI.log is
    // still enabled.) In order to do this we have to initialize the log
    // manager before the main gui system itself, otherwise the main
    // object will get the chance to spit out a few messages before we
    // can able to disable it.

    std::string theLogFile = std::string(MYGUI_PLATFORM_LOG_FILENAME);
    if(!logDir.empty())
        theLogFile.insert(0, logDir);

    // Set up OGRE platform (bypassing OgrePlatform). We might make this more generic later.
    mLogManager = new LogManager();
    if (!Ogre::Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(Ogre::RSC_FIXED_FUNCTION))
        mShaderRenderManager = new MyGUI::ShaderBasedRenderManager();
    else
        mRenderManager = new MyGUI::OgreRenderManager();
    mDataManager = new MyGUI::FixedOgreDataManager();

    // Do not use default log since it don't support Unicode path on Windows.
    // Instead, manually create log source using LogFacility and pass it.
    mLogFacility = new MyGUI::LogFacility(theLogFile, logging);
    LogManager::getInstance().addLogSource(mLogFacility->getSource());

    if (mShaderRenderManager)
        mShaderRenderManager->initialise(wnd, mgr);
    else
        mRenderManager->initialise(wnd, mgr);
    mDataManager->initialise("General");

    // Create GUI
    mGui = new Gui();
    mGui->initialise("");
}

void MyGUIManager::windowResized()
{
#ifndef ANDROID
    mRenderManager->setActiveViewport(0);
#endif
}

void MyGUIManager::shutdown()
{
    mGui->shutdown ();
    delete mGui;
    if(mRenderManager)
    {
        mRenderManager->shutdown();
        delete mRenderManager;
        mRenderManager = NULL;
    }
    if(mShaderRenderManager)
    {
        mShaderRenderManager->shutdown();
        delete mShaderRenderManager;
        mShaderRenderManager = NULL;
    }
    if(mDataManager)
    {
        mDataManager->shutdown();
        delete mDataManager;
        mDataManager = NULL;
    }
    if (mLogManager)
    {
        delete mLogManager;
        mLogManager = NULL;
    }
    delete mLogFacility;

    mGui = NULL;
}
