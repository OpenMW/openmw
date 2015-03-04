#include "ogreinit.hpp"

#include <string>
#include <ctime>
#include <cstdio>
#include <cstring>

#include <OgreRoot.h>
#include <OgreParticleEmitterFactory.h>
#include <OgreParticleSystemManager.h>
#include <OgreLogManager.h>
#include <OgreLog.h>

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#include <OSX/macUtils.h>
#endif

#include <components/nifogre/particles.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

#include "ogreplugin.hpp"


namespace bfs = boost::filesystem;

namespace
{
    /** \brief Custom Ogre::LogListener interface implementation being
        able to portably handle UTF-8 encoded path.

        Effectively this is used in conjunction with default listener,
        but since on every message messageLogged() set 'skip' flag to
        true, there should be no troubles sharing same file.
    */
    class LogListener : public Ogre::LogListener
    {
        bfs::ofstream file;
        char buffer[16];


    public:

        LogListener(const std::string &path)
            : file((bfs::path(path)))
        {
            memset(buffer, 0, sizeof(buffer));
        }

        void timestamp()
        {
            int local = time(0) % 86400;
            int sec = local % 60;
            int min = (local / 60) % 60;
            int hrs = local / 3600;
            sprintf(buffer, "%02d:%02d:%02d: ", hrs, min, sec);
        }

        virtual void messageLogged(const std::string &msg, Ogre::LogMessageLevel lvl, bool mask, const std::string &logName, bool &skip)
        {
            timestamp();
            file << buffer << msg << std::endl;
            skip = true;
        }
    };
}

namespace OgreInit
{

    OgreInit::OgreInit()
        : mRoot(NULL)
    #ifdef ENABLE_PLUGIN_CgProgramManager
    , mCgPlugin(NULL)
    #endif
    #ifdef ENABLE_PLUGIN_OctreeSceneManager
    , mOctreePlugin(NULL)
    #endif
    #ifdef ENABLE_PLUGIN_ParticleFX
    , mParticleFXPlugin(NULL)
    #endif
    #ifdef ENABLE_PLUGIN_GL
    , mGLPlugin(NULL)
    #endif
    #ifdef ENABLE_PLUGIN_GLES2
    , mGLES2Plugin(NULL)
    #endif
    
  #ifdef ENABLE_PLUGIN_Direct3D9
    , mD3D9Plugin(NULL)
    #endif
    {}

    Ogre::Root* OgreInit::init(const std::string &logPath)
    {
        if (mRoot)
            throw std::runtime_error("OgreInit was already initialised");

        #ifndef ANDROID
        // Set up logging first
        new Ogre::LogManager;
        Ogre::Log *log = Ogre::LogManager::getSingleton().createLog(logPath);

        #if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        // Use custom listener only on Windows
        log->addListener(new LogListener(logPath));
        #endif

        // Disable logging to cout/cerr
        log->setDebugOutputEnabled(false);
        #endif
        mRoot = new Ogre::Root("", "", "");

        #if defined(ENABLE_PLUGIN_GL) || (ENABLE_PLUGIN_GLES2) || defined(ENABLE_PLUGIN_Direct3D9) || defined(ENABLE_PLUGIN_CgProgramManager) || defined(ENABLE_PLUGIN_OctreeSceneManager) || defined(ENABLE_PLUGIN_ParticleFX)
        loadStaticPlugins();
        #else
        loadPlugins();
        #endif

        loadParticleFactories();

        return mRoot;
    }

    OgreInit::~OgreInit()
    {
        delete mRoot;
        delete Ogre::LogManager::getSingletonPtr();

        std::vector<Ogre::ParticleEmitterFactory*>::iterator ei;
        for(ei = mEmitterFactories.begin();ei != mEmitterFactories.end();++ei)
            OGRE_DELETE (*ei);
        mEmitterFactories.clear();

        std::vector<Ogre::ParticleAffectorFactory*>::iterator ai;
        for(ai = mAffectorFactories.begin();ai != mAffectorFactories.end();++ai)
            OGRE_DELETE (*ai);
        mAffectorFactories.clear();

        #ifdef ENABLE_PLUGIN_GL
        delete mGLPlugin;
        mGLPlugin = NULL;
        #endif
        #ifdef ENABLE_PLUGIN_GLES2
        delete mGLES2Plugin;
        mGLES2Plugin = NULL;
        #endif   
        #ifdef ENABLE_PLUGIN_Direct3D9
        delete mD3D9Plugin;
        mD3D9Plugin = NULL;
        #endif
        #ifdef ENABLE_PLUGIN_CgProgramManager
        delete mCgPlugin;
        mCgPlugin = NULL;
        #endif
        #ifdef ENABLE_PLUGIN_OctreeSceneManager
        delete mOctreePlugin;
        mOctreePlugin = NULL;
        #endif
        #ifdef ENABLE_PLUGIN_ParticleFX
        delete mParticleFXPlugin;
        mParticleFXPlugin = NULL;
        #endif
    }

    void OgreInit::loadStaticPlugins()
    {
        #ifdef ENABLE_PLUGIN_GL
        mGLPlugin = new Ogre::GLPlugin();
        mRoot->installPlugin(mGLPlugin);
        #endif
        #ifdef ENABLE_PLUGIN_GLES2
        mGLES2Plugin = new Ogre::GLES2Plugin();
        mRoot->installPlugin(mGLES2Plugin);
        #endif       
        #ifdef ENABLE_PLUGIN_Direct3D9
        mD3D9Plugin = new Ogre::D3D9Plugin();
        mRoot->installPlugin(mD3D9Plugin);
        #endif
        #ifdef ENABLE_PLUGIN_CgProgramManager
        mCgPlugin = new Ogre::CgPlugin();
        mRoot->installPlugin(mCgPlugin);
        #endif
        #ifdef ENABLE_PLUGIN_OctreeSceneManager
        mOctreePlugin = new Ogre::OctreePlugin();
        mRoot->installPlugin(mOctreePlugin);
        #endif
        #ifdef ENABLE_PLUGIN_ParticleFX
        mParticleFXPlugin = new Ogre::ParticleFXPlugin();
        mRoot->installPlugin(mParticleFXPlugin);
        #endif
    }

    void OgreInit::loadPlugins()
    {
        std::string pluginDir;
        const char* pluginEnv = getenv("OPENMW_OGRE_PLUGIN_DIR");
        if (pluginEnv)
            pluginDir = pluginEnv;
        else
        {
    #if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
            pluginDir = ".\\";
    #endif
    #if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
            pluginDir = OGRE_PLUGIN_DIR;
            // if path is not specified try to find plugins inside the app bundle
            if (pluginDir.empty())
                pluginDir = Ogre::macFrameworksPath();
    #endif
    #if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
            pluginDir = OGRE_PLUGIN_DIR;
    #endif
        }
        Files::loadOgrePlugin(pluginDir, "RenderSystem_GL", *mRoot);
        Files::loadOgrePlugin(pluginDir, "RenderSystem_GLES2", *mRoot);
        Files::loadOgrePlugin(pluginDir, "RenderSystem_GL3Plus", *mRoot);
        Files::loadOgrePlugin(pluginDir, "RenderSystem_Direct3D9", *mRoot);
        Files::loadOgrePlugin(pluginDir, "Plugin_CgProgramManager", *mRoot);
        if (!Files::loadOgrePlugin(pluginDir, "Plugin_ParticleFX", *mRoot))
            throw std::runtime_error("Required Plugin_ParticleFX for Ogre not found!");
    }

    void OgreInit::loadParticleFactories()
    {
        Ogre::ParticleEmitterFactory *emitter;
        emitter = OGRE_NEW NifEmitterFactory();
        Ogre::ParticleSystemManager::getSingleton().addEmitterFactory(emitter);
        mEmitterFactories.push_back(emitter);

        Ogre::ParticleAffectorFactory *affector;
        affector = OGRE_NEW GrowFadeAffectorFactory();
        Ogre::ParticleSystemManager::getSingleton().addAffectorFactory(affector);
        mAffectorFactories.push_back(affector);

        affector = OGRE_NEW GravityAffectorFactory();
        Ogre::ParticleSystemManager::getSingleton().addAffectorFactory(affector);
        mAffectorFactories.push_back(affector);
    }

}
