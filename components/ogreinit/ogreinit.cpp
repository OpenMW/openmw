#include "ogreinit.hpp"

#include <string>

#include <OgreRoot.h>
#include <OgreParticleEmitterFactory.h>
#include <OgreParticleSystemManager.h>

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#include <OSX/macUtils.h>
#endif

#include <components/nifogre/particles.hpp>

#include "ogreplugin.hpp"

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
    #ifdef ENABLE_PLUGIN_Direct3D9
    , mD3D9Plugin(NULL)
    #endif
    {}

    Ogre::Root* OgreInit::init(const std::string &logPath)
    {
        // Set up logging first
        new Ogre::LogManager;
        Ogre::Log *log = Ogre::LogManager::getSingleton().createLog(logPath);

        // Disable logging to cout/cerr
        log->setDebugOutputEnabled(false);

        mRoot = new Ogre::Root("", "", "");

        #if defined(ENABLE_PLUGIN_GL) || defined(ENABLE_PLUGIN_Direct3D9) || defined(ENABLE_PLUGIN_CgProgramManager) || defined(ENABLE_PLUGIN_OctreeSceneManager) || defined(ENABLE_PLUGIN_ParticleFX)
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
                pluginDir = Ogre::macPluginPath();
    #endif
    #if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
            pluginDir = OGRE_PLUGIN_DIR_REL;
    #endif
        }

        boost::filesystem::path absPluginPath = boost::filesystem::absolute(boost::filesystem::path(pluginDir));

        pluginDir = absPluginPath.string();

        Files::loadOgrePlugin(pluginDir, "RenderSystem_GL", *mRoot);
        Files::loadOgrePlugin(pluginDir, "RenderSystem_GLES2", *mRoot);
        Files::loadOgrePlugin(pluginDir, "RenderSystem_GL3Plus", *mRoot);
        Files::loadOgrePlugin(pluginDir, "RenderSystem_Direct3D9", *mRoot);
        Files::loadOgrePlugin(pluginDir, "Plugin_CgProgramManager", *mRoot);
        Files::loadOgrePlugin(pluginDir, "Plugin_ParticleFX", *mRoot);
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
