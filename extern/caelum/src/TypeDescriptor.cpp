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
#include "TypeDescriptor.h"

#if CAELUM_TYPE_DESCRIPTORS

using namespace Ogre;

namespace Caelum
{
    DefaultTypeDescriptor::DefaultTypeDescriptor ()
    {
    }
    
    DefaultTypeDescriptor::~DefaultTypeDescriptor () {
        for (TypeDescriptor::PropertyMap::const_iterator it = mPropertyMap.begin(), end = mPropertyMap.end(); it != end; ++it) {
            delete it->second;
        }
    }

    const ValuePropertyDescriptor* DefaultTypeDescriptor::getPropertyDescriptor (const Ogre::String& name) const
    {
        PropertyMap::const_iterator it = mPropertyMap.find(name);
        if (it != mPropertyMap.end()) {
            return it->second;
        } else {
            return 0;
        }
    }

    const std::vector<String> DefaultTypeDescriptor::getPropertyNames () const
    {
        std::vector<String> result;
        for (TypeDescriptor::PropertyMap::const_iterator it = mPropertyMap.begin(), end = mPropertyMap.end(); it != end; ++it) {
            result.push_back(it->first);
        }
        return result;
    }

    const TypeDescriptor::PropertyMap DefaultTypeDescriptor::getFullPropertyMap () const {
        return mPropertyMap;
    }

    void DefaultTypeDescriptor::add (const Ogre::String& name, const ValuePropertyDescriptor* descriptor)
    {
        mPropertyMap.insert(make_pair(name, descriptor));
    }
}

#endif
