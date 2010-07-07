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
#include "MyGUI_Precompiled.h"
#include "MyGUI_DynLib.h"

#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
#	include <Windows.h>
#elif MYGUI_PLATFORM == MYGUI_PLATFORM_LINUX
#       include <dlfcn.h>
#endif

namespace MyGUI
{
	DynLib::DynLib( const std::string& name )
	{
		mName = name;
		mInstance = nullptr;
	}


	DynLib::~DynLib()
	{
	}


	bool DynLib::load()
	{
		// Log library load
		MYGUI_LOG(Info, "Loading library " << mName);

		#if MYGUI_PLATFORM == MYGUI_PLATFORM_APPLE
			//APPLE SPECIFIC CODE HERE
		#else
			mInstance = (MYGUI_DYNLIB_HANDLE)MYGUI_DYNLIB_LOAD( mName.c_str() );
		#endif

		return mInstance != 0;
	}


	void DynLib::unload()
	{
		// Log library unload
		MYGUI_LOG(Info, "Unloading library " << mName);
		#if MYGUI_PLATFORM == MYGUI_PLATFORM_APPLE
			//APPLE SPECIFIC CODE HERE
		#else
			if (MYGUI_DYNLIB_UNLOAD(mInstance))
			{
				MYGUI_EXCEPT("Could not unload dynamic library '" << mName << "'. System Error: " << dynlibError());
			}
		#endif
	}

	void* DynLib::getSymbol( const std::string& strName ) const throw()
	{
		#if MYGUI_PLATFORM == MYGUI_PLATFORM_APPLE
			//APPLE SPECIFIC CODE HERE
			return nullptr;
		#else
			return (void*)MYGUI_DYNLIB_GETSYM(mInstance, strName.c_str());
		#endif
	}

	std::string DynLib::dynlibError( void )
	{
#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
		LPVOID lpMsgBuf;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0,
			NULL
			);
		std::string ret = (char*)lpMsgBuf;
		// Free the buffer.
		LocalFree( lpMsgBuf );
		return ret;
#else
		return "no unix error function defined yet";
#endif
	}

} // namespace MyGUI
