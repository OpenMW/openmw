/* -------------------------------------------------------
Copyright (c) 2011 Alberto G. Salguero (alberto.salguero (at) uca.es)

Permission is hereby granted, free of charge, to any
person obtaining a copy of this software and associated
documentation files (the "Software"), to deal in the
Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the
Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice
shall be included in all copies or substantial portions of
the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------- */

//! @todo add mouse wheel support

#ifndef _InputControlSystem_Prerequisites_H_
#define _InputControlSystem_Prerequisites_H_

/// Include external headers
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include <list>
#include <limits>
#include <algorithm> /* std::min and std::max for MSVC 2013 */

#include "tinyxml.h"

#include "SDL_keyboard.h"
#include "SDL_mouse.h"
#include "SDL_joystick.h"
#include "SDL_events.h"

/// Define the dll export qualifier if compiling for Windows

/*
#ifdef ICS_PLATFORM_WIN32
   #ifdef ICS_LIB
     #define DllExport __declspec (dllexport)
   #else
     #define DllExport __declspec (dllimport)
   #endif
#else
   #define DllExport
#endif
*/
#define DllExport

// Define some macros
#define ICS_DEPRECATED __declspec(deprecated("Deprecated. It will be removed in future versions."))

/// Version defines
#define ICS_VERSION_MAJOR 0
#define ICS_VERSION_MINOR 4
#define ICS_VERSION_PATCH 0

#define ICS_MAX_DEVICE_BUTTONS 30

namespace ICS
{
	template <typename T>
	bool StringIsNumber ( const std::string &Text )
	{
		std::stringstream ss(Text);
		T result;
        return (ss >> result) ? true : false;
	}

	// from http://www.cplusplus.com/forum/articles/9645/
	template <typename T>
	std::string ToString ( T value )
	{
		std::stringstream ss;
		ss << value;
		return ss.str();
	}

	// from http://www.cplusplus.com/forum/articles/9645/
	template <typename T>
	T FromString ( const std::string &Text )//Text not by const reference so that the function can be used with a
	{											//character array as argument
		std::stringstream ss(Text);
		T result;
        return (ss >> result) ? result : 0;
	}

	class InputControlSystem;
    class Channel;
    class ChannelListener;
    class Control;
	class ControlListener;
	class DetectingBindingListener;
	class InputControlSystemLog;
}

#endif
