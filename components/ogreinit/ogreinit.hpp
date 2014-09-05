#ifndef OPENMW_COMPONENTS_OGREINIT_H
#define OPENMW_COMPONENTS_OGREINIT_H

#include <vector>
#include <string>

// Static plugin headers
#ifdef ENABLE_PLUGIN_CgProgramManager
# include "OgreCgPlugin.h"
#endif
#ifdef ENABLE_PLUGIN_OctreeSceneManager
# include "OgreOctreePlugin.h"
#endif
#ifdef ENABLE_PLUGIN_ParticleFX
# include "OgreParticleFXPlugin.h"
#endif
#ifdef ENABLE_PLUGIN_GL
# include "OgreGLPlugin.h"
#endif
#ifdef ENABLE_PLUGIN_GLES2
# include "OgreGLES2Plugin.h"
#endif

#ifdef ENABLE_PLUGIN_Direct3D9
# include "OgreD3D9Plugin.h"
#endif

namespace Ogre
{
    class ParticleEmitterFactory;
    class ParticleAffectorFactory;
    class Root;
}

namespace OgreInit
{
    /**
     * @brief Starts Ogre::Root and loads required plugins and NIF particle factories
     */
    class OgreInit
    {
    public:
        OgreInit();

        Ogre::Root* init(const std::string &logPath // Path to directory where to store log files
            );

        ~OgreInit();

    private:
        std::vector<Ogre::ParticleEmitterFactory*> mEmitterFactories;
        std::vector<Ogre::ParticleAffectorFactory*> mAffectorFactories;
        Ogre::Root* mRoot;

        void loadStaticPlugins();
        void loadPlugins();
        void loadParticleFactories();

        #ifdef ENABLE_PLUGIN_CgProgramManager
        Ogre::CgPlugin* mCgPlugin;
        #endif
        #ifdef ENABLE_PLUGIN_OctreeSceneManager
        Ogre::OctreePlugin* mOctreePlugin;
        #endif
        #ifdef ENABLE_PLUGIN_ParticleFX
        Ogre::ParticleFXPlugin* mParticleFXPlugin;
        #endif
        #ifdef ENABLE_PLUGIN_GL
        Ogre::GLPlugin* mGLPlugin;
        #endif
        #ifdef ENABLE_PLUGIN_GLES2
        Ogre::GLES2Plugin* mGLES2Plugin;
        #endif
        #ifdef ENABLE_PLUGIN_Direct3D9
        Ogre::D3D9Plugin* mD3D9Plugin;
        #endif

    };
}

#endif
