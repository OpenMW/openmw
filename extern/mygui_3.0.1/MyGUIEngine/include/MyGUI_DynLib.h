/*!
	@file
	@author		Denis Koronchik
	@author		Georgiy Evmenov
	@date		09/2007
	@module
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

#ifndef __MYGUI_DYNLIB_H__
#define __MYGUI_DYNLIB_H__

#include "MyGUI_Prerequest.h"
#include <string>


#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
#    define MYGUI_DYNLIB_HANDLE hInstance
#    define MYGUI_DYNLIB_LOAD( a ) LoadLibrary( a )
#    define MYGUI_DYNLIB_GETSYM( a, b ) GetProcAddress( a, b )
#    define MYGUI_DYNLIB_UNLOAD( a ) !FreeLibrary( a )

struct HINSTANCE__;
typedef struct HINSTANCE__* hInstance;

#elif MYGUI_PLATFORM == MYGUI_PLATFORM_LINUX
#    define MYGUI_DYNLIB_HANDLE void*
#    define MYGUI_DYNLIB_LOAD( a ) dlopen( a, RTLD_LAZY | RTLD_GLOBAL)
#    define MYGUI_DYNLIB_GETSYM( a, b ) dlsym( a, b )
#    define MYGUI_DYNLIB_UNLOAD( a ) dlclose( a )

#elif MYGUI_PLATFORM == MYGUI_PLATFORM_APPLE
#    include <CoreFoundation/CFBundle.h>
#    define MYGUI_DYNLIB_HANDLE CFBundleRef
#    define MYGUI_DYNLIB_LOAD( a ) mac_loadExeBundle( a )
#    define MYGUI_DYNLIB_GETSYM( a, b ) mac_getBundleSym( a, b )
#    define MYGUI_DYNLIB_UNLOAD( a ) mac_unloadExeBundle( a )
#endif

namespace MyGUI
{

	/*! @brief Resource holding data about a dynamic library.

		@remarks
		This class holds the data required to get symbols from
		libraries loaded at run-time (i.e. from DLL's for so's)
	*/
	class MYGUI_EXPORT DynLib
	{
		friend class DynLibManager;

	protected:
		DynLib(const std::string &name);

		~DynLib();

	public:

		/*! Load the library
		*/
		bool load();

		/*! Unload the library
		*/
		void unload();

		//! Get the name of the library
		std::string getName(void) const { return mName; }

		/**
			Returns the address of the given symbol from the loaded library.
			@param
				strName The name of the symbol to search for
			@returns
				If the function succeeds, the returned value is a handle to the symbol.
				If the function fails, the returned value is <b>nullptr</b>.
		*/
		void* getSymbol( const std::string& strName ) const throw();

	protected:
		//! Gets the last loading error
		std::string dynlibError(void);


	protected:
		//!	Name of library
		std::string mName;

		//! Handle to the loaded library.
		MYGUI_DYNLIB_HANDLE mInstance;
	};

}

#endif // __MYGUI_DYNLIB_H__
