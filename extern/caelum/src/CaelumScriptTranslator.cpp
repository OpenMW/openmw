/*
This file is part of Caelum.
See http://www.ogre3d.org/wiki/index.php/Caelum 

Copyright (c) 2006-2008 Caelum team. See Contributors.txt for details.

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
#include "CaelumPrerequisites.h"

#if CAELUM_SCRIPT_SUPPORT

#include "CaelumScriptTranslator.h"
#include "CaelumSystem.h"
#include "CaelumExceptions.h"

using namespace Ogre;

namespace Caelum
{
    PropScriptResource::PropScriptResource
    (
        Ogre::ResourceManager* creator, const Ogre::String& name, Ogre::ResourceHandle handle,
        const Ogre::String& group, bool isManual, Ogre::ManualResourceLoader* loader
    ):
        Ogre::Resource (creator, name, handle, group, isManual, loader)
    {
        //Ogre::LogManager::getSingleton().logMessage(
        //        "PropScriptResource::PropScriptResource");
    }

    PropScriptResource::~PropScriptResource() {
        //Ogre::LogManager::getSingleton().logMessage(
        //        "PropScriptResource::~PropScriptResource");
    }

    PropScriptResourceManager::PropScriptResourceManager() {
        mLoadOrder = 1000;
        mResourceType = "PropertyScript";
    }

    PropScriptResource* PropScriptResourceManager::createImpl(
                const String& name, ResourceHandle handle, const String& group,
                bool isManual, ManualResourceLoader* loader, const NameValuePairList* createParams)
    {
        //Ogre::LogManager::getSingleton().logMessage(
        //        "PropScriptResourceManager::createImpl");
        return new PropScriptResource (this, name, handle, group, isManual, loader);
    }

    TypeDescriptorScriptTranslator::TypeDescriptorScriptTranslator (TypeDescriptor* typeDescriptor):
            mTypeDescriptor(typeDescriptor)
    {
    }

    bool TypeDescriptorScriptTranslator::getPropValueOrAddError (
            ScriptCompiler* compiler,
            PropertyAbstractNode* prop,
            bool& value)
    {
        if (prop->values.empty ()) {
            compiler->addError (ScriptCompiler::CE_STRINGEXPECTED, prop->file, prop->line);
            return false;
        }
        if (prop->values.size () > 1) {
            compiler->addError (ScriptCompiler::CE_FEWERPARAMETERSEXPECTED, prop->file, prop->line,
                    prop->name + " must have at most 1 argument");
            return false;
        } 
        if (!Ogre::ScriptTranslator::getBoolean(prop->values.front(), &value)) {
            compiler->addError (ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line,
                        prop->values.front()->getValue() + " is not a valid number");
            return false;
        }
        return true;
    }

    bool TypeDescriptorScriptTranslator::getPropValueOrAddError (
            ScriptCompiler* compiler,
            PropertyAbstractNode* prop,
            ColourValue& value)
    {
        if (prop->values.empty ()) {
            compiler->addError (ScriptCompiler::CE_STRINGEXPECTED, prop->file, prop->line);
            return false;
        }
        if (prop->values.size () > 4) {
            compiler->addError (ScriptCompiler::CE_FEWERPARAMETERSEXPECTED, prop->file, prop->line,
                    prop->name + " must have at most 4 arguments");
            return false;
        }
        if (prop->values.size () < 3) {
            compiler->addError (ScriptCompiler::CE_FEWERPARAMETERSEXPECTED, prop->file, prop->line,
                    prop->name + " must have at least 3 arguments");
        }
        if (!getColour(prop->values.begin(), prop->values.end(), &value)) {
            compiler->addError (ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line,
                    prop->name + " requires a colour argument");
        }
        return true;
    }

    bool TypeDescriptorScriptTranslator::getPropValueOrAddError (
            ScriptCompiler* compiler,
            PropertyAbstractNode* prop,
            float& value)
    {
        if (prop->values.empty ()) {
            compiler->addError (ScriptCompiler::CE_STRINGEXPECTED, prop->file, prop->line);
            return false;
        }
        if (prop->values.size () > 1) {
            compiler->addError (ScriptCompiler::CE_FEWERPARAMETERSEXPECTED, prop->file, prop->line,
                    prop->name + " must have at most 1 argument");
            return false;
        } 
        if (!Ogre::ScriptTranslator::getFloat(prop->values.front(), &value)) {
            compiler->addError (ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line,
                        prop->values.front()->getValue() + " is not a valid number");
            return false;
        }
        return true;
    }

    bool TypeDescriptorScriptTranslator::getPropValueOrAddError (
            ScriptCompiler* compiler,
            PropertyAbstractNode* prop,
            int& value)
    {
        if (prop->values.empty ()) {
            compiler->addError (ScriptCompiler::CE_STRINGEXPECTED, prop->file, prop->line);
            return false;
        }
        if (prop->values.size () > 1) {
            compiler->addError (ScriptCompiler::CE_FEWERPARAMETERSEXPECTED, prop->file, prop->line,
                    prop->name + " must have at most 1 argument");
            return false;
        } 
        if (!Ogre::ScriptTranslator::getInt(prop->values.front(), &value)) {
            compiler->addError (ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line,
                        prop->values.front()->getValue() + " is not a valid integer");
            return false;
        }
        return true;
    }

    bool TypeDescriptorScriptTranslator::getPropValueOrAddError (
            ScriptCompiler* compiler,
            PropertyAbstractNode* prop,
            double& value)
    {
        if (prop->values.empty ()) {
            compiler->addError (ScriptCompiler::CE_STRINGEXPECTED, prop->file, prop->line);
            return false;
        }
        if (prop->values.size () > 1) {
            compiler->addError (ScriptCompiler::CE_FEWERPARAMETERSEXPECTED, prop->file, prop->line,
                    prop->name + " must have at most 1 argument");
            return false;
        } 
        // We do need a string stream here for the extra precision.
        std::stringstream strStream (std::string(prop->values.front()->getValue()));
        strStream >> value;
        if (strStream.fail()) {
            compiler->addError (ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line,
                    prop->values.front()->getValue() + " is not a valid number");
            return false;
        }
        return true;
    }

    bool TypeDescriptorScriptTranslator::getPropValueOrAddError (
            ScriptCompiler* compiler,
            PropertyAbstractNode* prop,
            Ogre::Degree& value)
    {
        if (prop->values.size () == 0) {
            compiler->addError (ScriptCompiler::CE_STRINGEXPECTED, prop->file, prop->line);
            return false;
        }
        if (prop->values.size () > 3) {
            compiler->addError (ScriptCompiler::CE_FEWERPARAMETERSEXPECTED, prop->file, prop->line,
                    prop->name + " must have at most 3 arguments");
            return false;
        } 
        // Allow 3 components.
        float degMinSec[3] = { 0, 0, 0 };
        int k = 0;
        for (AbstractNodeList::const_iterator it = prop->values.begin(), endIt = prop->values.end(); it != endIt; ++it, ++k) {
            if (!Ogre::ScriptTranslator::getFloat(*it, &degMinSec[k])) {
                compiler->addError (ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line,
                        (*it)->getValue () + " is not a valid number");
                return false;
            }
        }
        value = Ogre::Degree(degMinSec[0] + degMinSec[1] / 60.0 + degMinSec[2] / 3600);
        return true;
    }

    bool TypeDescriptorScriptTranslator::getPropValueOrAddError (
            ScriptCompiler* compiler,
            PropertyAbstractNode* prop,
            Ogre::String& value)
    {
        if (prop->values.size () == 0) {
            compiler->addError (ScriptCompiler::CE_STRINGEXPECTED, prop->file, prop->line);
            return false;
        }
        if (prop->values.size () > 1) {
            compiler->addError (ScriptCompiler::CE_FEWERPARAMETERSEXPECTED, prop->file, prop->line,
                    prop->name + " must have at most 1 arguments");
            return false;
        } 
        if (!Ogre::ScriptTranslator::getString(prop->values.front(), &value)) {
            compiler->addError (ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line,
                        prop->values.front()->getValue() + " is not a valid string");
            return false;
        }
        return true;
    }

    bool TypeDescriptorScriptTranslator::getPropValueOrAddError (
            ScriptCompiler* compiler,
            PropertyAbstractNode* prop,
            Ogre::Vector3& value)
    {
        if (prop->values.size () == 0) {
            compiler->addError (ScriptCompiler::CE_STRINGEXPECTED, prop->file, prop->line);
            return false;
        }
        if (prop->values.size () > 3) {
            compiler->addError (ScriptCompiler::CE_FEWERPARAMETERSEXPECTED, prop->file, prop->line,
                    prop->name + " must have at most 3 arguments");
            return false;
        } 
        float floats[3];
        if (!Ogre::ScriptTranslator::getFloats(prop->values.begin(), prop->values.end(), floats, 3)) {
            compiler->addError (ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line,         
                    "incorrect vector parameters.");
            return false;
        }
        value.x = floats[0];
        value.y = floats[1];
        value.z = floats[2];
        return true;
    }
 
    bool TypeDescriptorScriptTranslator::getPropValueOrAddError (
            ScriptCompiler* compiler,
            PropertyAbstractNode* prop,
            Ogre::Vector2& value)
    {
        if (prop->values.size () == 0) {
            compiler->addError (ScriptCompiler::CE_STRINGEXPECTED, prop->file, prop->line);
            return false;
        }
        if (prop->values.size () > 2) {
            compiler->addError (ScriptCompiler::CE_FEWERPARAMETERSEXPECTED, prop->file, prop->line,
                    prop->name + " must have at most 3 arguments");
            return false;
        } 
        float floats[2];
        if (!Ogre::ScriptTranslator::getFloats(prop->values.begin(), prop->values.end(), floats, 2)) {
            compiler->addError (ScriptCompiler::CE_INVALIDPARAMETERS, prop->file, prop->line,         
                    "incorrect vector parameters.");
            return false;
        }
        value.x = floats[0];
        value.y = floats[1];
        return true;
    }

    template<class T>
    static bool tryHandlePropertyType (
            ScriptCompiler* compiler, PropertyAbstractNode* propNode,
            void* targetObject, const ValuePropertyDescriptor* propDesc)
    {
        if (propDesc->getValueTypeId () == typeid(T)) {
            T val;
            if (TypeDescriptorScriptTranslator::getPropValueOrAddError (compiler, propNode, val)) {
                propDesc->setValue (targetObject, Ogre::Any(val));
            }
            return true;
        }
        return false;
    }

    void TypeDescriptorScriptTranslator::translateProperty (
            ScriptCompiler* compiler,
            PropertyAbstractNode* prop,
            void* targetObject,
            const TypeDescriptor* typeDescriptor)
    {
        const ValuePropertyDescriptor* propDesc = typeDescriptor->getPropertyDescriptor (prop->name);
        if (!propDesc) {
            compiler->addError (ScriptCompiler::CE_UNEXPECTEDTOKEN, prop->file, prop->line, 
                    "property \"" + prop->name + "\" not recognized; missing from type descriptor.");
            return;
        }
        if (!propDesc->canSetValue ()) {
            compiler->addError (ScriptCompiler::CE_UNEXPECTEDTOKEN, prop->file, prop->line, 
                    "property \"" + prop->name + "\" is read-only and can't be set from a script.");
            return;
        }

        //LogManager::getSingleton ().logMessage ("TypeDescriptorScriptTranslator::translateProperty"
        //            " name '" + prop->name + "'");

        bool handled = false
                || tryHandlePropertyType<int> (compiler, prop, targetObject, propDesc)
                || tryHandlePropertyType<float> (compiler, prop, targetObject, propDesc)
                || tryHandlePropertyType<double> (compiler, prop, targetObject, propDesc)
                || tryHandlePropertyType<bool> (compiler, prop, targetObject, propDesc)
                || tryHandlePropertyType<Ogre::Degree> (compiler, prop, targetObject, propDesc)
                || tryHandlePropertyType<Ogre::String> (compiler, prop, targetObject, propDesc)
                || tryHandlePropertyType<Ogre::Vector3> (compiler, prop, targetObject, propDesc)
                || tryHandlePropertyType<Ogre::Vector2> (compiler, prop, targetObject, propDesc)
                || tryHandlePropertyType<Ogre::ColourValue> (compiler, prop, targetObject, propDesc);

        if (!handled) {
            compiler->addError (ScriptCompiler::CE_UNEXPECTEDTOKEN, prop->file, prop->line, 
                    "property \"" + prop->name + "\" is found in type descriptor but has "
                    "unsupported type. Mangled typeid is '" + propDesc->getValueTypeId().name() + "'");
        }
    }

    void TypeDescriptorScriptTranslator::translate (ScriptCompiler* compiler, const AbstractNodePtr& node)
    {
        //LogManager::getSingleton ().logMessage ("TypeDescriptorScriptTranslator::translate begin");

        // Check type descriptor was set.
        assert (getTypeDescriptor () && "Type descriptor must be set before we can translate.");

        // Fetch target object.
		ObjectAbstractNode *objNode = reinterpret_cast<ObjectAbstractNode*>(node.get());
        assert (!objNode->context.isEmpty ());
        void* targetObject = any_cast<void*> (objNode->context);
        assert (targetObject);

		for (AbstractNodeList::iterator i = objNode->children.begin(); i != objNode->children.end(); ++i)
		{
			if ((*i)->type == ANT_PROPERTY)
            {
				PropertyAbstractNode *prop = reinterpret_cast<PropertyAbstractNode*>((*i).get());

                translateProperty (compiler, prop, targetObject, getTypeDescriptor());
			}
			else if((*i)->type == ANT_OBJECT)
			{
                compiler->addError (ScriptCompiler::CE_INVALIDPARAMETERS, (*i)->file, (*i)->line);
            }
        }

        //LogManager::getSingleton ().logMessage ("TypeDescriptorScriptTranslator::translate end");
    }

    CaelumSystemScriptTranslator::CaelumSystemScriptTranslator ():
            mResourceManager(false),
            mTranslationTarget(0),
            mTranslationTargetFound(false),
            mTypeDescriptor(0)
    {
    }

    void CaelumSystemScriptTranslator::setTranslationTarget (CaelumSystem* target, const Ogre::String& name)
    {
        assert (target != 0);
        this->mTranslationTarget = target;
        this->mTranslationTargetName = name;
        this->mTranslationTargetFound = false;
    }

    void CaelumSystemScriptTranslator::clearTranslationTarget () {
        this->mTranslationTarget = 0;
        this->mTranslationTargetName.clear();
        this->mTranslationTargetFound = false;
    }

    void CaelumSystemScriptTranslator::translate (ScriptCompiler* compiler, const AbstractNodePtr& node)
    {
        //LogManager::getSingleton ().logMessage ("CaelumSystemScriptTranslator::translate begin");

		ObjectAbstractNode *objNode = reinterpret_cast<ObjectAbstractNode*>(node.get());

        CaelumSystem* sys = 0;

        // Check for a translation target.
        if (this->getTranslationTarget ()) {
            sys = this->getTranslationTarget ();

            // Check for a name match.
            if (this->getTranslationTargetName () != objNode->name) {
                //LogManager::getSingleton ().logMessage (
                //        "Caelum: Skipped " + objNode->cls + " name " + objNode->name + " while loading");
                return;
            }
            
            // Clear the target; this ensure that properties which are not
            // mentioned are set to their default values.
            // We only do this after we found a target; this ensure that if
            // the target is not found it's not modified either.
            sys->clear();

            //LogManager::getSingleton ().logMessage (
            //        "Caelum: Found " + objNode->cls + " name " + objNode->name + "; filling properties.");
            mTranslationTargetFound = true;
        } else if (this->getResourceManager ()) {
            // If we don't have a target but have a resource manager then create a resource.
            //LogManager::getSingleton ().logMessage (
            //        "Caelum: Saved " + objNode->cls + " name " + objNode->name + " as a resource");
            PropScriptResourceManager* mgr = this->getResourceManager ();
            ResourcePtr resource = mgr->create (objNode->name, compiler->getResourceGroup());
            resource->_notifyOrigin (objNode->file);
            return;
        }

        objNode->context = sys;

		for (AbstractNodeList::iterator i = objNode->children.begin(); i != objNode->children.end(); ++i)
		{
			if ((*i)->type == ANT_PROPERTY)
            {
				PropertyAbstractNode *prop = reinterpret_cast<PropertyAbstractNode*>((*i).get());

                // Properties implemented through type descriptor.
                TypeDescriptorScriptTranslator::translateProperty(
                        compiler, prop,
                        static_cast<void*>(sys),
                        getTypeDescriptor ());
			}
			else if((*i)->type == ANT_OBJECT)
			{
				ObjectAbstractNode *childObjNode = reinterpret_cast<ObjectAbstractNode*>((*i).get());

                //LogManager::getSingleton ().logMessage ("CaelumSystemScriptTranslator::translate child object"
                //        " value '" + childObjNode->getValue () + "'"
                //        " name '" + childObjNode->name + "'"
                //        " cls '" + childObjNode->cls + "'"
                //        " base '" + childObjNode->base + "'");
                
                // Only allow declarations with one class token; like "moon { }"
#if OGRE_VERSION < 0x010700
                if (childObjNode->name.empty () == false || childObjNode->base.empty () == false) {
#else
                if (childObjNode->name.empty () == false || childObjNode->bases.size () != 0) {
#endif
                    compiler->addError (
                            ScriptCompiler::CE_FEWERPARAMETERSEXPECTED,
                            childObjNode->file, childObjNode->line,
                            "caelum_sky_system components can't have names or bases");
                    continue;
                }
                const String& className = childObjNode->cls;

                try {
                    if (className == "sun") {
                        sys->setSun (new Sun (sys->getSceneMgr (), sys->getCaelumCameraNode ()));
                        childObjNode->context = static_cast<void*> (sys->getSun ());
                    } else if (className == "sky_dome") {
                        sys->setSkyDome (new SkyDome (sys->getSceneMgr (), sys->getCaelumCameraNode ()));
                        childObjNode->context = static_cast<void*>(sys->getSkyDome ());
                    } else if (className == "moon") {
                        sys->setMoon (new Moon (sys->getSceneMgr (), sys->getCaelumCameraNode ()));
                        childObjNode->context = static_cast<void*>(sys->getMoon ());
                    } else if (className == "ground_fog") {
                        sys->setGroundFog (new GroundFog (sys->getSceneMgr (), sys->getCaelumCameraNode ()));
                        childObjNode->context = static_cast<void*>(sys->getGroundFog ());
                    } else if (className == "depth_composer") {
                        sys->setDepthComposer (new DepthComposer (sys->getSceneMgr ()));
                        childObjNode->context = static_cast<void*>(sys->getDepthComposer ());
                    } else if (className == "point_starfield") {
                        sys->setPointStarfield (new PointStarfield (sys->getSceneMgr (), sys->getCaelumCameraNode()));
                        childObjNode->context = static_cast<void*>(sys->getPointStarfield ());
                    } else if (className == "precipitation") {
                        sys->setPrecipitationController (new PrecipitationController (sys->getSceneMgr ()));
                        childObjNode->context = static_cast<void*>(sys->getPrecipitationController ());
                    } else if (className == "cloud_system") {
                        sys->setCloudSystem (new CloudSystem (sys->getSceneMgr (), sys->getCaelumGroundNode ()));
                        childObjNode->context = static_cast<void*>(sys->getCloudSystem ());
                    } else {
                        LogManager::getSingleton ().logMessage ("CaelumSystemScriptTranslator::translate "
                                "unknown child object class '" + className + "'");
                    }
                } catch (Caelum::UnsupportedException& ex) {
                    // Catch all unsupported exceptions and report them.
                    // This should usually happen because the proper shaders are not supported by hardware.
                    //
                    // Script parsing should still succeed.
                    compiler->addError (
                            ScriptCompiler::CE_UNSUPPORTEDBYRENDERSYSTEM,
                            childObjNode->file, childObjNode->line,
                            "Failed to create component \"" + className + "\": " + ex.getFullDescription ());
                    continue;
                }
                processNode (compiler, *i);
            }
        }

        //LogManager::getSingleton ().logMessage ("SkySystemScriptTranslator::translate END");
    }

    void CloudSystemScriptTranslator::translate (ScriptCompiler* compiler, const AbstractNodePtr& node)
    {
        //LogManager::getSingleton ().logMessage ("SkySystemScriptTranslator::translate begin");

		ObjectAbstractNode *objNode = reinterpret_cast<ObjectAbstractNode*>(node.get());
        assert (!objNode->context.isEmpty ());
        void* rawTargetObject = any_cast<void*> (objNode->context);
        assert (rawTargetObject);

        CloudSystem* target = static_cast<CloudSystem*>(rawTargetObject);

		for (AbstractNodeList::iterator i = objNode->children.begin(); i != objNode->children.end(); ++i)
		{
			if ((*i)->type == ANT_PROPERTY)
            {
                compiler->addError (
                            ScriptCompiler::CE_INVALIDPARAMETERS,
                            objNode->file, objNode->line,
                            "cloud_system doesn't have any properties");
			}
			else if((*i)->type == ANT_OBJECT)
			{
				ObjectAbstractNode *childObjNode = reinterpret_cast<ObjectAbstractNode*>((*i).get());

                /*
                LogManager::getSingleton ().logMessage ("CloudSystemScriptTranslator::translate child object"
                        " value '" + childObjNode->getValue () + "'"
                        " name '" + childObjNode->name + "'"
                        " cls '" + childObjNode->cls + "'"
                        " base '" + childObjNode->base + "'");
                 */

                const Ogre::String& className = childObjNode->cls;
        
                if (className == "cloud_layer") {
                    // Don't allow names.
#if OGRE_VERSION < 0x010700
                    if (childObjNode->base.empty () == false) {
#else
                    if (childObjNode->bases.size () != 0) {
#endif
                        compiler->addError (
                                ScriptCompiler::CE_FEWERPARAMETERSEXPECTED,
                                childObjNode->file, childObjNode->line,
                                "cloud_layer can't have a base");
                        continue;
                    }
                    // Height here is irrelevant. It's silly to have it as a FlatCloudLayer ctor parameter.
                    target->createLayerAtHeight (0);
                    FlatCloudLayer* layer = target->getLayer (target->getLayerCount () - 1);

                    // Add the new layer as a context for the object node.
                    // This will eventually pass to the TypeDescriptorScriptTranslator for a cloud layer.
                    childObjNode->context = static_cast<void*>(layer);
                } else {
                    LogManager::getSingleton ().logMessage ("CloudSystemScriptTranslator::translate "
                            "unknown child object class '" + className + "'");
                }
                processNode (compiler, *i);
            }
        }

        //LogManager::getSingleton ().logMessage ("CloudSystemScriptTranslator::translate END");
    }

    CaelumScriptTranslatorManager::CaelumScriptTranslatorManager
    (
        CaelumDefaultTypeDescriptorData* typeData
    ):
        mCaelumSystemTranslator(),
        mCloudSystemTranslator(),
        mFlatCloudLayerTranslator(typeData->FlatCloudLayerTypeDescriptor),
        mSunTranslator(typeData->BaseSkyLightTypeDescriptor),
        mMoonTranslator(typeData->BaseSkyLightTypeDescriptor),
        mPointStarfieldTranslator(typeData->PointStarfieldTypeDescriptor),
        mGroundFogTranslator(typeData->GroundFogTypeDescriptor),
        mDepthComposerTranslator(typeData->DepthComposerTypeDescriptor),
        mPrecipitationTranslator(typeData->PrecipitationTypeDescriptor),
        mSkyDomeTranslator(typeData->SkyDomeTypeDescriptor)
    {
        mCaelumSystemTranslator.setTypeDescriptor(typeData->CaelumSystemTypeDescriptor);

        // Build translator map to member translators.
        mTranslatorMap.insert (std::make_pair ("caelum_sky_system", &mCaelumSystemTranslator));
        mTranslatorMap.insert (std::make_pair ("cloud_system", &mCloudSystemTranslator));
        mTranslatorMap.insert (std::make_pair ("cloud_layer", &mFlatCloudLayerTranslator));
        mTranslatorMap.insert (std::make_pair ("sun", &mSunTranslator));
        mTranslatorMap.insert (std::make_pair ("moon", &mMoonTranslator));
        mTranslatorMap.insert (std::make_pair ("point_starfield", &mPointStarfieldTranslator));
        mTranslatorMap.insert (std::make_pair ("ground_fog", &mGroundFogTranslator));
        mTranslatorMap.insert (std::make_pair ("depth_composer", &mDepthComposerTranslator));
        mTranslatorMap.insert (std::make_pair ("precipitation", &mPrecipitationTranslator));
        mTranslatorMap.insert (std::make_pair ("sky_dome", &mSkyDomeTranslator));
    }

    size_t CaelumScriptTranslatorManager::getNumTranslators () const {
        // Turns out this is never called.
        assert(0 && "This method should be removed from Ogre::ScriptTranslatorManager");
        return mTranslatorMap.size ();
    }

    void CaelumScriptTranslatorManager::_setPropScriptResourceManager (PropScriptResourceManager* mgr)
    {
        mCaelumSystemTranslator.setResourceManager (mgr);
    }

    ScriptTranslator* CaelumScriptTranslatorManager::getTranslator (const AbstractNodePtr& node)
    {
        //LogManager::getSingleton ().logMessage ("CaelumScriptTranslatorManager::getTranslator");
        if (node->type == ANT_ATOM) {
            //ObjectAbstractNode* atomNode = reinterpret_cast<ObjectAbstractNode*>(node.get());
            //LogManager::getSingleton ().logMessage ("CaelumScriptTranslatorManager::getTranslator atom node " + atomNode->getValue ());
        } else if (node->type == ANT_OBJECT) {
            ObjectAbstractNode* objNode = reinterpret_cast<ObjectAbstractNode*>(node.get());
            //LogManager::getSingleton ().logMessage ("CaelumScriptTranslatorManager::getTranslator object node " + objNode->getValue ());

            // Pass down the context.
            ScriptTranslatorMap::const_iterator it = mTranslatorMap.find(objNode->cls);
            if (it != mTranslatorMap.end()) {
                return it->second;
            }
        }

        // Not found in this manager.
        return 0;
    }
}

#endif // CAELUM_SCRIPT_SUPPORT
