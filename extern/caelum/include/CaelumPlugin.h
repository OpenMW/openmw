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

#ifndef CAELUM__CAELUM_PLUGIN_H
#define CAELUM__CAELUM_PLUGIN_H

#include "CaelumPrerequisites.h"
#include "CaelumScriptTranslator.h"
#include "TypeDescriptor.h"
#include "OgrePlugin.h"

namespace Caelum
{
    /** Implement an Ogre::Plugin for Caelum.
     *
     *  Ogre plugins are usually loaded from config files and they register
     *  various stuff in ogre managers. But you can also just link to the
     *  library normally and call install functions manually.
     */
    class CAELUM_EXPORT CaelumPlugin: public Ogre::Singleton<CaelumPlugin>, public Ogre::Plugin
    {
    public:
        /// Get reference to singleton instance; or crash if N/A.
        static CaelumPlugin& getSingleton(void);
        /// Get pointer to singleton instance; or pointer if N/A.
        static CaelumPlugin* getSingletonPtr(void);

        CaelumPlugin();
        ~CaelumPlugin();

        virtual void install ();
        virtual void initialise ();
        virtual void shutdown ();
        virtual void uninstall ();

        static const Ogre::String CAELUM_PLUGIN_NAME;
        virtual const String& getName () const;

        // Determine if the plugin was installed (if install was called).
        inline bool isInstalled () const { return mIsInstalled; }

   private:
        bool mIsInstalled;

#if CAELUM_TYPE_DESCRIPTORS
   public:
        /// Get default type descriptor data for caelum components.
        CaelumDefaultTypeDescriptorData* getTypeDescriptorData () { return &mTypeDescriptorData; }

   private:
        CaelumDefaultTypeDescriptorData mTypeDescriptorData;
#endif

#if CAELUM_SCRIPT_SUPPORT
   public:
        /** Load CaelumSystem and it's components from a script file.
         *  @param sys Target CaelumSystem.
         *      This is cleared using CaelumSystem::clear before loading.
         *      If scripting data is not found then this is not modified.
         *  @param objectName Name of caelum_sky_system from *.os file.
         *  @param scriptFileGroup The group to search in (unused in Ogre 1.6)
         */
        void loadCaelumSystemFromScript (
                CaelumSystem* sys,
                const Ogre::String& objectName,
                const Ogre::String& scriptFileGroup = Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME
                );

        /// @see PropScriptResourceManager
        PropScriptResourceManager* getPropScriptResourceManager () { return &mPropScriptResourceManager; }
        CaelumScriptTranslatorManager* getScriptTranslatorManager () { return &mScriptTranslatorManager; }

   private:
        PropScriptResourceManager mPropScriptResourceManager;
        CaelumScriptTranslatorManager mScriptTranslatorManager;
#endif
    };
}

#endif // CAELUM__CAELUM_PLUGIN_H
