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

#ifndef CAELUM__CAELUM_EXCEPTIONS_H
#define CAELUM__CAELUM_EXCEPTIONS_H

#include "CaelumPrerequisites.h"

namespace Caelum
{
    /** Exception class for unsupported features.
     *  This is frequently thrown if a certain required material does not load;
     *  most likely because the hardware does not support the required shaders.
     */
    class CAELUM_EXPORT UnsupportedException : public Ogre::Exception
    {
    public:
        /// Constructor.
        UnsupportedException
        (
                int number,
                const Ogre::String &description,
                const Ogre::String &source,
                const char *file,
                long line
        ):
                Ogre::Exception (number, description, source, "UnsupportedException", file, line)
        {
        }
    };

#define CAELUM_THROW_UNSUPPORTED_EXCEPTION(desc, src) \
        throw UnsupportedException(-1, (desc), (src), __FILE__, __LINE__);

}

#endif // CAELUM__CAELUM_EXCEPTIONS_H
