/*
This file is part of Caelum.
See http://www.ogre3d.org/wiki/index.php/Caelum 

Copyright (c) 2008 Caelum team. See Contributors.txt for details.

Caelum is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Caelum is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Caelum. If not, see <http://www.gnu.org/licenses/>.
*/

#include "CaelumPrecompiled.h"
#include "CaelumPlugin.h"

template<> Caelum::CaelumPlugin* Ogre::Singleton<Caelum::CaelumPlugin>::msSingleton = 0;

namespace Caelum
{
	CaelumPlugin* CaelumPlugin::getSingletonPtr () {
        return msSingleton;
    }

    CaelumPlugin& CaelumPlugin::getSingleton () {  
        assert (msSingleton);
        return *msSingleton;  
    }

    extern "C" void CAELUM_EXPORT dllStartPlugin () {
        assert (CaelumPlugin::getSingletonPtr () == 0);
        CaelumPlugin* plugin = new CaelumPlugin();
        assert (CaelumPlugin::getSingletonPtr () == plugin);
        Ogre::Root::getSingleton ().installPlugin (CaelumPlugin::getSingletonPtr ());
    }

    extern "C" void CAELUM_EXPORT dllStopPlugin () {
        assert (CaelumPlugin::getSingletonPtr () != 0);
        Ogre::Root::getSingleton ().uninstallPlugin (CaelumPlugin::getSingletonPtr ());
        delete CaelumPlugin::getSingletonPtr ();
        assert (CaelumPlugin::getSingletonPtr () == 0);
    }

#if CAELUM_SCRIPT_SUPPORT
    CaelumPlugin::CaelumPlugin(): mScriptTranslatorManager(&mTypeDescriptorData)
#else
    CaelumPlugin::CaelumPlugin()
#endif
    {
        mIsInstalled = false;
    }

    CaelumPlugin::~CaelumPlugin() {
    }

    const Ogre::String CaelumPlugin::CAELUM_PLUGIN_NAME = "Caelum";

    const Ogre::String& CaelumPlugin::getName () const {
        return CAELUM_PLUGIN_NAME;
    }

    void CaelumPlugin::install ()
    {
        assert(!mIsInstalled && "Already installed");

        Ogre::LogManager::getSingleton ().logMessage("Caelum plugin version " + 
                Ogre::StringConverter::toString (CAELUM_VERSION_MAIN) + "." +
                Ogre::StringConverter::toString (CAELUM_VERSION_SEC) + "." +
                Ogre::StringConverter::toString (CAELUM_VERSION_TER) + " "
                "installed");

#if CAELUM_SCRIPT_SUPPORT
        Ogre::ScriptCompilerManager::getSingleton ().addTranslatorManager (
                getScriptTranslatorManager ());
        Ogre::ResourceGroupManager::getSingleton()._registerResourceManager (
                getPropScriptResourceManager ()->getResourceType (),
                getPropScriptResourceManager ());

        getScriptTranslatorManager()->_setPropScriptResourceManager (
                &mPropScriptResourceManager);
#endif // CAELUM_SCRIPT_SUPPORT

        mIsInstalled = true;
    }

    void CaelumPlugin::initialise () {
    }

    void CaelumPlugin::shutdown () {
    }

    void CaelumPlugin::uninstall ()
    {
        assert(mIsInstalled && "Not installed");

#if CAELUM_SCRIPT_SUPPORT
        getScriptTranslatorManager()->_setPropScriptResourceManager (0);

        Ogre::ResourceGroupManager::getSingleton ()._unregisterResourceManager (
                getPropScriptResourceManager ()->getResourceType ());
        Ogre::ScriptCompilerManager::getSingleton ().removeTranslatorManager (
                getScriptTranslatorManager ());
#endif // CAELUM_SCRIPT_SUPPORT

        Ogre::LogManager::getSingleton ().logMessage("Caelum plugin uninstalled");

        mIsInstalled = false;
    }

#if CAELUM_SCRIPT_SUPPORT
    void CaelumPlugin::loadCaelumSystemFromScript (
            CaelumSystem* sys,
            const Ogre::String& objectName,
            const Ogre::String& groupName)
    {
        assert (sys);
        assert (this->isInstalled () && "Must install CaelumPlugin before loading scripts");

        // Fetch raw resource ptr. Attempt to support explicit resource groups currently in Ogre trunk.
#if OGRE_VERSION >= 0x00010700
        Ogre::ResourcePtr res = getPropScriptResourceManager ()->getByName (objectName, groupName);
#else
        Ogre::ResourcePtr res = getPropScriptResourceManager ()->getByName (objectName);
#endif

        // Check a PropScriptResource was found.
        PropScriptResource* propRes = static_cast<PropScriptResource*> (res.get ());
        if (!propRes) {
            OGRE_EXCEPT (Ogre::Exception::ERR_ITEM_NOT_FOUND,
                    "Could not find caelum_sky_system " + objectName,
                    "CaelumPlugin::loadCaelumSystemFromScript");
        }

        // Fetch the resource stream. Look in the actual group of the resource!
        const Ogre::String& scriptFileName = propRes->getOrigin();
        const Ogre::String& scriptFileGroup = propRes->getGroup();
        Ogre::DataStreamPtr streamPtr = Ogre::ResourceGroupManager::getSingleton ().openResource (
                scriptFileName, scriptFileGroup, false);

        // Feed it into the compiler.
        this->getScriptTranslatorManager()->getCaelumSystemTranslator()->setTranslationTarget (sys, objectName);
        Ogre::ScriptCompilerManager::getSingleton ().parseScript (streamPtr, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        bool found = this->getScriptTranslatorManager()->getCaelumSystemTranslator()->foundTranslationTarget ();

        // This shouldn't normally happen.
        if (!found) {
            OGRE_EXCEPT (Ogre::Exception::ERR_ITEM_NOT_FOUND,
                    "Could not find caelum_sky_system " + objectName + " in file " + scriptFileName + " on reparsing. "
                    "Perhaps information in PropScriptResourceManager is out of date?",
                    "CaelumPlugin::loadCaelumSystemFromScript");
        }
        this->getScriptTranslatorManager()->getCaelumSystemTranslator()->clearTranslationTarget ();
    }
#endif // CAELUM_SCRIPT_SUPPORT
}
