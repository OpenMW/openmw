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

#include "CaelumPrecompiled.h"
#include "FastGpuParamRef.h"

using namespace Ogre;

namespace Caelum
{
    FastGpuParamRef::FastGpuParamRef(Ogre::GpuProgramParametersSharedPtr paramsPtr, const Ogre::String& name)
    {
        this->bind(paramsPtr, name);
    }

    void FastGpuParamRef::bind(
            Ogre::GpuProgramParametersSharedPtr params,
            const Ogre::String& name,
            bool throwIfNotFound/* = false*/)
    {
        assert(!params.isNull());
        #if CAELUM_DEBUG_PARAM_REF
            mParams = params;
        #endif
        const GpuConstantDefinition* def = params->_findNamedConstantDefinition(name, throwIfNotFound);
        if (def) {
            mPhysicalIndex = def->physicalIndex;
            assert(this->isBound());
        } else {
            mPhysicalIndex = InvalidPhysicalIndex;
            assert(!this->isBound());
        }
    }

    void FastGpuParamRef::unbind() {
        #if CAELUM_DEBUG_PARAM_REF
            mParams.setNull();
        #endif
        mPhysicalIndex = InvalidPhysicalIndex;
        assert(!this->isBound());
    }
}
