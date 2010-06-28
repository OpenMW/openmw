/*
This file is part of Caelum.
See http://www.ogre3d.org/wiki/index.php/Caelum 

Copyright (c) 2009 Caelum team. See Contributors.txt for details.

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

#ifndef CAELUM__FAST_GPU_PARAM_REF_H
#define CAELUM__FAST_GPU_PARAM_REF_H

#include "CaelumPrerequisites.h"

namespace Caelum
{
    /** @file */ 

    /** Controls if FastGpuParamRef checks pointer match when setting.
     *  This setting trades safety for performance. By default it's equal to OGRE_DEBUG_MODE.
     */
    #define CAELUM_DEBUG_PARAM_REF OGRE_DEBUG_MODE

    /** An optimized reference to a gpu shared parameter.
     *
     *  Profiling shows that GpuProgramParameters::_findNamedConstantDefinition is not free.
     *
     *  This class avoids hash lookups when updating. It's uses no additional
     *  memory than if you were to implement the same thing manually.
     *
     *  You must also keep the matching Ogre::GpuProgramParametersSharedPtr
     *  and send it whenever you call FastGpuParamRef::set. This is required
     *  to save memory in release mode. Debug mode checks the pointer you
     *  pass to set is the same as the pointer you called bind on; but uses more memory.
     *
     *  Also; please note that fetching gpu params from a material every frame is not free either.
     */
    class CAELUM_EXPORT FastGpuParamRef
    {
    public:
        /// Default constructor. Starts as unbound
        FastGpuParamRef(): mPhysicalIndex(InvalidPhysicalIndex)
        {
            // mParams needs no initialization; SharedPtrs start as 0.
        }

        /// Create and bind.
        FastGpuParamRef(Ogre::GpuProgramParametersSharedPtr paramsPtr, const Ogre::String& name);

        /** Bind to a certain parameter.
         *
         *  @param paramsPtr params to bind to. Can't be null; you must unbind explicitly.
         *  @param name The name of the parameter to bind.
         *  @param throwIfNotFound Argument to GpuProgramParameters::_findNamedConstantDefinition.
         *
         *  If throwIfNotFound is false (the default) missing parameters are
         *  ignored and the param ref will remand unbound. Calling set will
         *  then have no effect.
         */
        void bind(
                Ogre::GpuProgramParametersSharedPtr paramsPtr,
                const Ogre::String& name,
                bool throwIfNotFound = false);

        /** Unbind ParamRef.
         *
         *  If CAELUM_DEBUG_PARAM_REF is 1 this will also release the hold
         *  on GpuProgramParametersSharedPtr.
         */
        void unbind();

        /// Return if this param ref is bound to an actual param.
        inline bool isBound() const { return mPhysicalIndex != InvalidPhysicalIndex; }

        /// Return the physical index. Only valid if this->isBound().
        inline size_t getPhysicalIndex () const { return mPhysicalIndex; }

    protected:
        /** Set the value. No effect if !this->isBound()
         *  
         *  @param params Parameter pointer. Can't be null
         *  @param arg Argument to set.
         *
         *  Will check params pointer matches the bound pointer if #CAELUM_DEBUG_PARAM_REF.
         *  Otherwise a mismatched params pointer can result in memory corruption.
         */
        template<typename ArgumentT>
        inline void doSet(const Ogre::GpuProgramParametersSharedPtr& params, ArgumentT arg) const {
            #if CAELUM_DEBUG_PARAM_REF
                assert(params.getPointer() == mParams.getPointer());
            #endif
            assert(!params.isNull());
            if (mPhysicalIndex != InvalidPhysicalIndex) {
                params->_writeRawConstant(mPhysicalIndex, arg);
            }
        }

        template<typename ArgumentT>
        inline void doSet(const Ogre::GpuProgramParametersSharedPtr& params, ArgumentT arg, size_t count) const {
            #if CAELUM_DEBUG_PARAM_REF
                assert(params.getPointer() == mParams.getPointer());
            #endif
            assert(!params.isNull());
            if (mPhysicalIndex != InvalidPhysicalIndex) {
                params->_writeRawConstant(mPhysicalIndex, arg, count);
            }
        }

    public:
        /// @copydoc FastGpuParamRef::doSet
        inline void set(const Ogre::GpuProgramParametersSharedPtr& params, int val) const { doSet<int>(params, val); }
        /// @copydoc FastGpuParamRef::doSet
        inline void set(const Ogre::GpuProgramParametersSharedPtr& params, Ogre::Real val) const { doSet<Ogre::Real>(params, val); }
        /// @copydoc FastGpuParamRef::doSet
        inline void set(const Ogre::GpuProgramParametersSharedPtr& params, const Ogre::Vector3& val) const { doSet<const Ogre::Vector3&>(params, val); }
        /// @copydoc FastGpuParamRef::doSet
        inline void set(const Ogre::GpuProgramParametersSharedPtr& params, const Ogre::Vector4& val) const { doSet<const Ogre::Vector4&>(params, val); }
        /// @copydoc FastGpuParamRef::doSet
        inline void set(const Ogre::GpuProgramParametersSharedPtr& params, const Ogre::ColourValue& val) const { doSet<const Ogre::ColourValue&>(params, val); }
        /// @copydoc FastGpuParamRef::doSet
        inline void set(const Ogre::GpuProgramParametersSharedPtr& params, const Ogre::Matrix4& val) const { doSet<const Ogre::Matrix4*>(params, &val, 1); }

    private:
        #if CAELUM_DEBUG_PARAM_REF
            Ogre::GpuProgramParametersSharedPtr mParams;
        #endif
        static const size_t InvalidPhysicalIndex = 0xFFFFFFFF;
        size_t mPhysicalIndex;
    };
}

#endif /* CAELUM__FAST_GPU_PARAM_REF_H */
