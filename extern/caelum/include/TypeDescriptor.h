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

#ifndef CAELUM__TYPE_DESCRIPTOR_H
#define CAELUM__TYPE_DESCRIPTOR_H

#include "CaelumPrerequisites.h"

#if CAELUM_TYPE_DESCRIPTORS

#include <typeinfo>

namespace Caelum
{
    class ValuePropertyDescriptor;

    /** Abstract interface for a type descriptor.
     *  A type descriptor contains informations about the properties of
     *  another object. It provides access to a map of strings to
     *  ValuePropertyDescriptor. All methods are const.
     * 
     *  This is not a full reflection mechanism; it doesn't care about
     *  methods and parameters. It's just a way to access object properties
     *  in a uniform way.
     *
     *  The list of properties supported by a type descriptor is fixed.
     *
     *  The type descriptor is responsible for the lifetime of
     *  ValuePropertyDescriptor objects; never the user.
     */
    class CAELUM_EXPORT TypeDescriptor
    {
    public:
        virtual ~TypeDescriptor() {};

        typedef std::map<String, const ValuePropertyDescriptor*> PropertyMap;

        /** Get a property descriptor; or null if not available.
         *  @param name Name of the property to request.
         *  @return A pointer to a property descriptor; or null if not available.
         */
        virtual const ValuePropertyDescriptor* getPropertyDescriptor (const Ogre::String& name) const = 0;

        /** Get a map of all supported properties.
         *  Returns a complete list of all supported properties; by value.
         */
        virtual const std::vector<String> getPropertyNames () const = 0;

        /** Get a map of all supported properties.
         *  Returns a complete list of all supported properties; by value.
         */
        virtual const PropertyMap getFullPropertyMap () const = 0;
    };

    /** Basic property descriptor interface.
     *
     *  A property descriptor provides a uniform way to change the value of a
     *  simple property. The values are safely wrapped inside an Ogre::Any.
     *
     *  This only works for simple properties which are copied by value. This
     *  includes floats strings and vectors but not things like Entity pointers.
     *
     *  All public methods are const because the descriptor itself is not
     *  modified by these methods.
     */
    class CAELUM_EXPORT ValuePropertyDescriptor
    {
    public:
        virtual ~ValuePropertyDescriptor() {};

        /** If the value of the property can be read (true means write-only).
         *
         *  This is false for write-only properties.
         *  Write-only properties are generally a bad idea but they are supported.
         *  Scripting (with .os files) doesn't actually require reading existing values.
         */
        virtual bool canGetValue () const = 0;

        /// If the value of the property can be set (false means read-only).
        virtual bool canSetValue () const = 0;

        /** Get the value of the property packed in an Ogre::Any.
         *  
         *  @param target Object to fetch the property from. If target is
         *  not of the correct type behaviour is undefined.
         */
        virtual const Ogre::Any getValue (const void* target) const = 0;

        /** Set the value of the property packed in an Ogre::Any.
         *  @param target Object set the property on. If target is not of
         *  the correct type then behaviour is undefined.
         *  @param value New value of the property.
         */
        virtual void setValue (void* target, const Ogre::Any& value) const = 0;

        /// Get std::type_info for the type of the value.
        virtual const std::type_info& getValueTypeId () const = 0;

        /** Check if this class also implements TypedValuePropertyDescriptor.
         *
         *  If this property returns true then you can static_cast this object to
         *  a TypedValuePropertyDescriptor<ValueT>; for the appropiate ValueT.
         *  The appropriate ValueT can be obtained with getValueTypeId.
         */
        virtual bool implementsTypedValuePropertyDescriptor () const {
            return false;
        }
    };

    /** Variant of ValuePropertyDescriptor which allows faster typed get/set methods.
     */
    template<typename ValueT>
    class CAELUM_EXPORT TypedValuePropertyDescriptor: public ValuePropertyDescriptor
    {
    public:
        /// Get the property's value.
        virtual const ValueT getValueTyped (const void* target) const = 0;
        /// Set the property's value.
        virtual void setValueTyped (void* target, const ValueT& value) const = 0;

    private:
        virtual const Ogre::Any getValue (const void* target) const {
            return Ogre::Any(this->getValueTyped (target));
        }

        virtual void setValue (void* target, const Ogre::Any& value) const {
            this->setValueTyped (target, Ogre::any_cast<ValueT>(value));
        }

        virtual const std::type_info& getValueTypeId () const {
            return typeid(ValueT);
        }

        virtual bool implementsTypedValuePropertyDescriptor () const {
            return true;
        }
    };

    /** ValuePropertyDescriptor implementation based on function pointers to get/set methods.
     */
    template <class TargetT, typename ParamT, typename InParamT = const ParamT&, typename OutParamT = const ParamT>
    class AccesorPropertyDescriptor: public TypedValuePropertyDescriptor<ParamT>
    {
    public:
        typedef void(TargetT::*SetFunc)(InParamT);
        typedef OutParamT(TargetT::*GetFunc)(void) const;

    private:
        GetFunc mGetFunc;
        SetFunc mSetFunc;

    public:
        AccesorPropertyDescriptor (GetFunc getFunc, SetFunc setFunc)
        {
            mGetFunc = getFunc;
            mSetFunc = setFunc;
        }

        virtual bool canGetValue () const {
            return mGetFunc != 0;
        }

        virtual bool canSetValue () const {
            return mSetFunc != 0;
        }

        virtual const ParamT getValueTyped(const void* target) const
        {
            const TargetT* typedTarget = reinterpret_cast<const TargetT*>(target);
            return (typedTarget->*mGetFunc)();
        }

        virtual void setValueTyped(void* target, const ParamT& value) const
        {
            TargetT* typedTarget = reinterpret_cast<TargetT*>(target);
            (typedTarget->*mSetFunc)(value);
        }
    };

    /** Default implementation of a TypeDescriptor.
     *  This is a standard implementation of a type descriptor.
     *
     *  It allows direct access to an internal PropertyMap. The user must
     *  manually fill the map with property descriptors; probably in an init
     *  method of sorts.
     */
    class DefaultTypeDescriptor: public TypeDescriptor 
    {
    public:
        DefaultTypeDescriptor ();
        virtual ~DefaultTypeDescriptor ();

        /** Direct access to the internal property map.
         *  Get the property map used to implement this type descriptor.
         *  Once initialisation is complete the property map should no longer
         *  be modified.
         */
        inline PropertyMap& getPropertyMap () { return mPropertyMap; }

        /// Add a property. Type descriptor takes ownership.
        void add (const Ogre::String& name, const ValuePropertyDescriptor* descriptor);

        /// Clear the property map; delete all property descriptors.
        void clear ();

        /// @copydoc TypeDescriptor::getPropertyDescriptor
        virtual const ValuePropertyDescriptor* getPropertyDescriptor (const Ogre::String& name) const;

        /// @copydoc TypeDescriptor::getPropertyNames
        virtual const std::vector<String> getPropertyNames () const;

        /// @copydoc TypeDescriptor::getFullPropertyMap
        virtual const PropertyMap getFullPropertyMap () const;

    private:
        void deleteAllPropertyDescriptors ();

        PropertyMap mPropertyMap;
    };

    /** Standard type descriptors for caelum components.
     *
     *  This class hold pointers to several type descriptors for classes
     *  inside Caelum. All the pointers are initialize in the contructor and
     *  properly destroyed in the destructor.
     *
     *  The CaelumPlugin singleton contains a const instance of this class. You
     *  should fetch that instead of creating a new object; using
     *  CaelumPlugin::getTypeDescriptorData().
     */
    class CAELUM_EXPORT CaelumDefaultTypeDescriptorData
    {
    private:
        void load();
        void unload();

    public:
        CaelumDefaultTypeDescriptorData();
        ~CaelumDefaultTypeDescriptorData();

        DefaultTypeDescriptor* CaelumSystemTypeDescriptor;
        DefaultTypeDescriptor* PointStarfieldTypeDescriptor;
        DefaultTypeDescriptor* BaseSkyLightTypeDescriptor;
        DefaultTypeDescriptor* GroundFogTypeDescriptor;
        DefaultTypeDescriptor* PrecipitationTypeDescriptor;
        DefaultTypeDescriptor* DepthComposerTypeDescriptor;
        DefaultTypeDescriptor* FlatCloudLayerTypeDescriptor;
        DefaultTypeDescriptor* SkyDomeTypeDescriptor;
    };
}

#endif // CAELUM_TYPE_DESCRIPTORS

#endif // CAELUM__TYPE_DESCRIPTOR_H
