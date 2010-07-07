/*!
	@file
	@author		Denis Koronchik
	@author		Georgiy Evmenov
	@author		Albert Semenov
	@date		09/2007
*/
/*
	This file is part of MyGUI.

	MyGUI is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MyGUI is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with MyGUI.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __MYGUI_PLATFORM_H__
#define __MYGUI_PLATFORM_H__

// Definnition of platforms
#define MYGUI_PLATFORM_WIN32		1
#define MYGUI_PLATFORM_LINUX		2
#define MYGUI_PLATFORM_APPLE		3

// Definition of compilers
#define MYGUI_COMPILER_MSVC 1
#define MYGUI_COMPILER_GNUC 2


// Find platform
#if defined (__WIN32__) || defined (_WIN32)
#	define MYGUI_PLATFORM MYGUI_PLATFORM_WIN32
#elif defined (__APPLE_CC__)
#	define MYGUI_PLATFORM MYGUI_PLATFORM_APPLE
#else
#	define MYGUI_PLATFORM MYGUI_PLATFORM_LINUX
#endif

// Find compiler
#if defined( _MSC_VER )
#	define MYGUI_COMPILER MYGUI_COMPILER_MSVC
#	define MYGUI_COMP_VER _MSC_VER

#elif defined( __GNUC__ )
#	define MYGUI_COMPILER MYGUI_COMPILER_GNUC
#	define MYGUI_COMP_VER (((__GNUC__)*100) + \
        (__GNUC_MINOR__*10) + \
        __GNUC_PATCHLEVEL__)
#else
#	pragma error "Unknown compiler! Stop building!!!"
#endif

// See if we can use __forceinline or if we need to use __inline instead
#if MYGUI_COMPILER == MYGUI_COMPILER_MSVC
#	if MYGUI_COMP_VER >= 1200
#		define MYGUI_FORCEINLINE __forceinline
#	endif
#elif defined(__MINGW32__)
#	if !defined(MYGUI_FORCEINLINE)
#		define MYGUI_FORCEINLINE __inline
#	endif
#else
#	define MYGUI_FORCEINLINE __inline
#endif


// Windows settings
#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
#
#	if defined( MYGUI_STATIC )
#		define MYGUI_EXPORT
#	elif defined( MYGUI_BUILD )
#		define MYGUI_EXPORT __declspec( dllexport )
#	else
#		if defined( __MINGW32__ )
#			define MYGUI_EXPORT
#		else
#			define MYGUI_EXPORT __declspec( dllimport )
#		endif
#	endif
#
#	if defined( MYGUI_STATIC )
#		define MYGUI_EXPORT_DLL
#	elif defined( MYGUI_BUILD_DLL )
#		define MYGUI_EXPORT_DLL __declspec( dllexport )
#	else
#		if defined( __MINGW32__ )
#			define MYGUI_EXPORT_DLL
#		else
#			define MYGUI_EXPORT_DLL __declspec( dllimport )
#		endif
#	endif
#
#// Win32 compilers use _DEBUG for specifying debug builds.
#	ifdef _DEBUG
#		define MYGUI_DEBUG_MODE 1
#	else
#		define MYGUI_DEBUG_MODE 0
#	endif
#endif


// Linux/Apple Settings
#if MYGUI_PLATFORM == MYGUI_PLATFORM_LINUX || MYGUI_PLATFORM == MYGUI_PLATFORM_APPLE
#
// Add -fvisibility=hidden to compiler options. With -fvisibility=hidden, you are telling
// GCC that every declaration not explicitly marked with a visibility attribute (MYGUI_EXPORT)
// has a hidden visibility (like in windows).
#	if __GNUC__ >= 4
#		define MYGUI_EXPORT  __attribute__ ((visibility("default")))
#	else
#		define MYGUI_EXPORT
#	endif
#
#	if __GNUC__ >= 4
#		define MYGUI_EXPORT_DLL  __attribute__ ((visibility("default")))
#	else
#		define MYGUI_EXPORT_DLL
#	endif
#
// A quick define to overcome different names for the same function
#   define stricmp strcasecmp
#
// Unlike the Win32 compilers, Linux compilers seem to use DEBUG for when
// specifying a debug build.
// (??? this is wrong, on Linux debug builds aren't marked in any way unless
// you mark it yourself any way you like it -- zap ???)
#	ifdef DEBUG
#		define MYGUI_DEBUG_MODE 1
#	else
#		define MYGUI_DEBUG_MODE 0
#	endif

#	if MYGUI_PLATFORM == MYGUI_PLATFORM_APPLE
#		define MYGUI_PLATFORM_LIB "MYGUIPlatform.bundle"
#	else // if MYGUI_PLATFORM_LINUX
#		define MYGUI_PLATFORM_LIB "libMYGUIPlatform.so"
#	endif

#endif


#endif // __MYGUI_PLATFORM_H__
