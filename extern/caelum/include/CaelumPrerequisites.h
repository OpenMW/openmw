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

#ifndef CAELUM__CAELUM_PREREQUISITES_H
#define CAELUM__CAELUM_PREREQUISITES_H

// Include external headers
#ifdef __APPLE__
#include "Ogre/Ogre.h"
#else
#include "Ogre.h"
#endif

#include <memory>

// Define the dll export qualifier if compiling for Windows
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	#ifdef CAELUM_LIB
		#define CAELUM_EXPORT __declspec (dllexport)
	#else
		#ifdef __MINGW32__
			#define CAELUM_EXPORT
		#else
			#define CAELUM_EXPORT __declspec (dllimport)
		#endif
	#endif
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	#define CAELUM_EXPORT __attribute__ ((visibility("default")))
#else
	#define CAELUM_EXPORT
#endif

// Define the version code
#define CAELUM_VERSION_MAIN 0
#define CAELUM_VERSION_SEC 5
#define CAELUM_VERSION_TER 0
#define CAELUM_VERSION = (CAELUM_VERSION_MAIN << 16) | (CAELUM_VERSION_SEC << 8) | CAELUM_VERSION_TER

// By default only compile type descriptors for scripting.
#ifndef CAELUM_TYPE_DESCRIPTORS
    #if (OGRE_VERSION >= 0x00010600) && OGRE_USE_NEW_COMPILERS
        #define CAELUM_TYPE_DESCRIPTORS 1
    #else
        #define CAELUM_TYPE_DESCRIPTORS 0
    #endif
#endif

// Scripting support requires Ogre 1.6
// Can be also configured on compiler command line
#ifndef CAELUM_SCRIPT_SUPPORT
    #if (OGRE_VERSION >= 0x00010600) && OGRE_USE_NEW_COMPILERS
        #define CAELUM_SCRIPT_SUPPORT 1
    #else
        #define CAELUM_SCRIPT_SUPPORT 0
    #endif
#else
    #if !(OGRE_VERSION > 0x00010600)
        #error "Caelum script support requires Ogre 1.6."
    #endif
    #if !(OGRE_USE_NEW_COMPILERS)
        #error "Caelum script support requires Ogre 1.6 with OGRE_USE_NEW_COMPILERS."
    #endif
    #if !(CAELUM_TYPE_DESCRIPTORS)
        #error "Caelum script support also requires type descriptors."
    #endif
#endif

/// @file

/** @mainpage
 *
 *  %Caelum is an Ogre add-on for atmospheric rendering. It is composed of a
 *  number of small mostly self-contained components and a big
 *  Caelum::CaelumSystem class which ties them all together in an easy-to-use
 *  way.
 *
 *  More information is available on the wiki page:
 *  http://www.ogre3d.org/wiki/index.php/Caelum
 *
 *  You can discuss and report issues in the forum:
 *  http://www.ogre3d.org/addonforums/viewforum.php?f=21
 */

/** Caelum namespace
 *
 *  All of %Caelum is inside this namespace (except for macros).
 *
 *  @note: This was caelum with a lowercase 'c' in version 0.3
 */
namespace Caelum
{
    // Caelum needs a lot of precission for astronomical calculations.
    // Very few calculations use it, and the precission IS required.
    typedef double LongReal;

    // Use some ogre types.
    using Ogre::uint8;
    using Ogre::uint16;
    using Ogre::ushort;
    using Ogre::uint32;
    using Ogre::uint;

    using Ogre::Real;
    using Ogre::String;

    /// Resource group name for caelum resources.
    static const String RESOURCE_GROUP_NAME = "Caelum";

    // Render group for caelum stuff
    // It's best to have them all together
    enum CaelumRenderQueueGroupId
    {
        CAELUM_RENDER_QUEUE_STARFIELD       = Ogre::RENDER_QUEUE_SKIES_EARLY + 0,
		CAELUM_RENDER_QUEUE_MOON_BACKGROUND = Ogre::RENDER_QUEUE_SKIES_EARLY + 1,
        CAELUM_RENDER_QUEUE_SKYDOME         = Ogre::RENDER_QUEUE_SKIES_EARLY + 2,
		CAELUM_RENDER_QUEUE_MOON            = Ogre::RENDER_QUEUE_SKIES_EARLY + 3,
        CAELUM_RENDER_QUEUE_SUN             = Ogre::RENDER_QUEUE_SKIES_EARLY + 4,
        CAELUM_RENDER_QUEUE_CLOUDS          = Ogre::RENDER_QUEUE_SKIES_EARLY + 5,
        CAELUM_RENDER_QUEUE_GROUND_FOG      = Ogre::RENDER_QUEUE_SKIES_EARLY + 6,
    };

    // Forward declarations
    class UniversalClock;
    class SkyDome;
    class BaseSkyLight;
    class Moon;
    class SphereSun;
    class SpriteSun;
    class ImageStarfield;
    class PointStarfield;
    class CloudSystem;
    class CaelumSystem;
	class FlatCloudLayer;
    class PrecipitationController;
    class PrecipitationInstance;
    class GroundFog;
    class DepthComposer;
    class DepthComposerInstance;
    class DepthRenderer;
}

namespace Ogre 
{
#if OGRE_VERSION <= 0x010602
    // Write an Ogre::Degree to a stream.
    //
    // Ogre::Any requires that the wrapped type can be written to a stream;
    // otherwise it will fail on instantation. This function was placed here
    // so it's available everywhere. This can't be placed in namespace Caelum.
    //
    // Ogre 1.6.3 and up already include this operator; so it's ifdefed out.
    //
    // This function is never actually used; the output does not matter.
    inline std::ostream& operator << (std::ostream& out, Ogre::Degree deg) {
        return out << deg.valueDegrees();
    }
#endif
}

#endif // CAELUM__CAELUM_PREREQUISITES_H
