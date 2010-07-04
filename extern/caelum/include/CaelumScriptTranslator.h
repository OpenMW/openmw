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

#ifndef CAELUM__CAELUM_SCRIPT_TRANSLATOR_H
#define CAELUM__CAELUM_SCRIPT_TRANSLATOR_H

#if CAELUM_SCRIPT_SUPPORT

#include "CaelumPrerequisites.h"
#include "OgreScriptTranslator.h"
#include "TypeDescriptor.h"

namespace Caelum
{
    /** Dummy resources created for property script blocks.
     *
     *  When parsing material scripts the singleton rendersystem is available
     *  and materials are created using it. But Caelum's scriptable components
     *  require at least an active scene scene manager; and you can't require
     *  something like that when initializing resources.
     *
     *  So instead a dummy resource like this is created which only remembers
     *  the location of the script block in the resources. Actually loading the
     *  properties will always reparse the script.
     *
     *  The original file name is available from Ogre::Resource::getOrigin
     *
     *  These resources are managed by the PropScriptResourceManager. Resource
     *  operations like loading and unloading are meaningless.
     */
    class CAELUM_EXPORT PropScriptResource: public Ogre::Resource {
    protected:
        virtual void loadImpl () { }
        virtual void unloadImpl () { }
        virtual size_t calculateSize () const { return 0; }

    public:
		PropScriptResource (
                Ogre::ResourceManager* creator, const Ogre::String& name, Ogre::ResourceHandle handle,
			    const Ogre::String& group, bool isManual, Ogre::ManualResourceLoader* loader);
		~PropScriptResource();
    };

    /** Resource manager for PropScriptResource.
     */
    class CAELUM_EXPORT PropScriptResourceManager: public Ogre::ResourceManager
    {
    public:
        PropScriptResourceManager();

		virtual PropScriptResource* createImpl(
                const String& name, Ogre::ResourceHandle handle, const String& group,
                bool isManual, Ogre::ManualResourceLoader* loader, const Ogre::NameValuePairList* createParams);
    };

    /** An Ogre::ScriptTranslator based on a TypeDescriptor.
     *  This class implements an Ogre::ScriptTranslator based on data from a TypeDescriptor.
     *
     *  The target object is never created; it must be passed in the context member of the
     *  root node. Some other ScriptTranslator must cooperate and set the context member;
     *  this is similar to how Ogre::PassTranslator depends on Ogre::MaterialTranslator
     *  setting the context.
     *
     *  Ogre::AbstractNode::context is an Ogre::Any which boxes objects in a way which
     *  stores the static (compile-time) type at assignment. You must cast the
     *  object into a void* before setting it as the context.
     *
     *  Most of the actual translation functionality is in static functions; a class can
     *  translate based on TypeDescriptor data without deriving from this class.
     */
    class CAELUM_EXPORT TypeDescriptorScriptTranslator: public Ogre::ScriptTranslator
    {
    public:
        /** Get the value of a property or report the appropriate error.
         *  @return Success value.
         */
        static bool getPropValueOrAddError (Ogre::ScriptCompiler* compiler, Ogre::PropertyAbstractNode* prop, int& value);
        static bool getPropValueOrAddError (Ogre::ScriptCompiler* compiler, Ogre::PropertyAbstractNode* prop, float& value);
        static bool getPropValueOrAddError (Ogre::ScriptCompiler* compiler, Ogre::PropertyAbstractNode* prop, double& value);
        static bool getPropValueOrAddError (Ogre::ScriptCompiler* compiler, Ogre::PropertyAbstractNode* prop, bool& value);
        static bool getPropValueOrAddError (Ogre::ScriptCompiler* compiler, Ogre::PropertyAbstractNode* prop, Ogre::Degree& value);
        static bool getPropValueOrAddError (Ogre::ScriptCompiler* compiler, Ogre::PropertyAbstractNode* prop, Ogre::ColourValue& value);
        static bool getPropValueOrAddError (Ogre::ScriptCompiler* compiler, Ogre::PropertyAbstractNode* prop, Ogre::String& value);
        static bool getPropValueOrAddError (Ogre::ScriptCompiler* compiler, Ogre::PropertyAbstractNode* prop, Ogre::Vector3& value);
        static bool getPropValueOrAddError (Ogre::ScriptCompiler* compiler, Ogre::PropertyAbstractNode* prop, Ogre::Vector2& value);

        /** Translate a property using a TypeDescriptor; or report error to compiler.
         */
        static void translateProperty (
                Ogre::ScriptCompiler* compiler,
                Ogre::PropertyAbstractNode* prop,
                void* targetObject,
                const TypeDescriptor* typeDescriptor);

    public:
        explicit TypeDescriptorScriptTranslator (TypeDescriptor* type = 0);

        virtual void translate (Ogre::ScriptCompiler* compiler, const Ogre::AbstractNodePtr& node);

        inline const TypeDescriptor* getTypeDescriptor () const { return mTypeDescriptor; }
        inline TypeDescriptor* getTypeDescriptor () { return mTypeDescriptor; }

    private:
        TypeDescriptor* mTypeDescriptor;
    };

    /** Script translator for CaelumSystem
     */
    struct CAELUM_EXPORT CaelumSystemScriptTranslator: public Ogre::ScriptTranslator
    {
    public:
        CaelumSystemScriptTranslator();

        virtual void translate (Ogre::ScriptCompiler* compiler, const Ogre::AbstractNodePtr& node);

        void setTranslationTarget (CaelumSystem* target, const Ogre::String& name);
        void clearTranslationTarget ();

        inline bool hasTranslationTarget () const { return mTranslationTarget != 0; }
        inline bool foundTranslationTarget () const { return mTranslationTargetFound; }
        inline CaelumSystem* getTranslationTarget () const { return mTranslationTarget; }
        inline const Ogre::String& getTranslationTargetName () const { return mTranslationTargetName; }

        inline void setResourceManager (PropScriptResourceManager* value) { mResourceManager = value; }
        inline PropScriptResourceManager* getResourceManager () const { return mResourceManager; }

    private:
        PropScriptResourceManager* mResourceManager;
        CaelumSystem* mTranslationTarget;
        Ogre::String mTranslationTargetName;
        bool mTranslationTargetFound;

    public:
        /** Type descriptor for CaelumSystem itself.
         *  This is use for simple top-level properties.
         *  Components (sun, moon etc) are handled with custom code.
         */
        inline const TypeDescriptor* getTypeDescriptor () const { return mTypeDescriptor; }
        inline void setTypeDescriptor (const TypeDescriptor* value) { mTypeDescriptor = value; }

    private:
        const TypeDescriptor* mTypeDescriptor;
    };

    /** Script translator for CloudSystem
     *  Caelum::CloudSystem requires a special translator because it's made up of separate layers.
     *
     *  Layers of different types are not supported; only instances of FlatCloudLayer.
     *  CloudSystem doesn't have any top-level properties.
     *
     *  Overriding works just like for ogre texture units; and you can use name-based overriding.
     *  Names are not preserved after script translation; they're only used inside Ogre's script
     *  compilation steps.
     */
    struct CAELUM_EXPORT CloudSystemScriptTranslator: public Ogre::ScriptTranslator
    {
    public:
        virtual void translate (Ogre::ScriptCompiler* compiler, const Ogre::AbstractNodePtr& node);
    };

    /** ScriptTranslatorManager for caelum's scriptable objects.
     *  This class contains Ogre::ScriptTranslators for Caelum components.
     */ 
    class CAELUM_EXPORT CaelumScriptTranslatorManager: public Ogre::ScriptTranslatorManager
    {
    public:
        explicit CaelumScriptTranslatorManager (CaelumDefaultTypeDescriptorData* typeData);

        virtual size_t getNumTranslators () const;

        /// @copydoc Ogre::ScriptTranslatorManager::getTranslator.
        virtual Ogre::ScriptTranslator* getTranslator (const Ogre::AbstractNodePtr& node);

        void _setPropScriptResourceManager (PropScriptResourceManager* mgr);

        inline CaelumSystemScriptTranslator* getCaelumSystemTranslator () { return &mCaelumSystemTranslator; }

    private:
        CaelumSystemScriptTranslator mCaelumSystemTranslator;
        CloudSystemScriptTranslator mCloudSystemTranslator;
        TypeDescriptorScriptTranslator mFlatCloudLayerTranslator;
        TypeDescriptorScriptTranslator mSunTranslator;
        TypeDescriptorScriptTranslator mMoonTranslator;
        TypeDescriptorScriptTranslator mPointStarfieldTranslator;
        TypeDescriptorScriptTranslator mGroundFogTranslator;
        TypeDescriptorScriptTranslator mDepthComposerTranslator;
        TypeDescriptorScriptTranslator mPrecipitationTranslator;
        TypeDescriptorScriptTranslator mSkyDomeTranslator;

        /// Maps class name to script translator.
        /// Does not own memory; just holds pointers to members.
        typedef std::map<Ogre::String, Ogre::ScriptTranslator*> ScriptTranslatorMap;
        ScriptTranslatorMap mTranslatorMap;
    };
}

#endif // CAELUM_SCRIPT_SUPPORT

#endif // CAELUM__CAELUM_SCRIPT_TRANSLATOR_H
