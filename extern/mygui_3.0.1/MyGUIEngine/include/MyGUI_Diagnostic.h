/*!
	@file
	@author		Albert Semenov
	@author		baho_is
	@date		11/2007
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
#ifndef __MYGUI_DIAGNOSTIC_H__
#define __MYGUI_DIAGNOSTIC_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Exception.h"
#include "MyGUI_LogManager.h"
#include <sstream>

// for debugging
#if MYGUI_COMPILER == MYGUI_COMPILER_MSVC
	#include <crtdbg.h>
#endif

#define MYGUI_LOG_SECTION "Core"
#define MYGUI_LOG_FILENAME "MyGUI.log"
#define MYGUI_LOG(level, text) MYGUI_LOGGING(MYGUI_LOG_SECTION, level, text)

#define MYGUI_BASE_EXCEPT(desc, src)	 throw MyGUI::Exception(desc, src, __FILE__, __LINE__);

// MSVC specific: sets the breakpoint
#if MYGUI_COMPILER == MYGUI_COMPILER_MSVC
	#define MYGUI_DBG_BREAK _CrtDbgBreak();
#else
	#define MYGUI_DBG_BREAK
#endif

#define MYGUI_EXCEPT(dest) \
{ \
	MYGUI_LOG(Critical, dest); \
	MYGUI_DBG_BREAK;\
	std::ostringstream stream; \
	stream << dest << "\n"; \
	MYGUI_BASE_EXCEPT(stream.str().c_str(), "MyGUI"); \
}

#define MYGUI_ASSERT(exp, dest) \
{ \
	if ( ! (exp) ) \
	{ \
		MYGUI_LOG(Critical, dest); \
		MYGUI_DBG_BREAK;\
		std::ostringstream stream; \
		stream << dest << "\n"; \
		MYGUI_BASE_EXCEPT(stream.str().c_str(), "MyGUI"); \
	} \
}

#define MYGUI_ASSERT_RANGE(index, size, owner) MYGUI_ASSERT(index < size, owner << " : index number " << index << " out of range [" << size << "]");
#define MYGUI_ASSERT_RANGE_AND_NONE(index, size, owner) MYGUI_ASSERT(index < size || index == ITEM_NONE, owner << " : index number " << index << " out of range [" << size << "]");
#define MYGUI_ASSERT_RANGE_INSERT(index, size, owner) MYGUI_ASSERT((index <= size) || (index == MyGUI::ITEM_NONE), owner << " : insert index number " << index << " out of range [" << size << "] or not ITEM_NONE");

#if MYGUI_DEBUG_MODE == 1
	#define MYGUI_REGISTER_VALUE(map, value) \
	{ \
		MYGUI_LOG(Info, "Register value : '" << #value << "' = " << (int)value); \
		map[#value] = value; \
	}
	#define MYGUI_DEBUG_ASSERT(exp, dest) MYGUI_ASSERT(exp, dest)
#else
	#define MYGUI_REGISTER_VALUE(map, value) map[#value] = value;
	#define MYGUI_DEBUG_ASSERT(exp, dest) ((void)0)
#endif


// for more info see: http://mdf-i.blogspot.com/2008/09/deprecated-gcc-vs-vs-vs-vs.html
#if MYGUI_COMPILER == MYGUI_COMPILER_MSVC
	#if MYGUI_COMP_VER == 1310 	// VC++ 7.1
		#define MYGUI_OBSOLETE_START(text)
	    #define MYGUI_OBSOLETE_END
	#else
		#define MYGUI_OBSOLETE_START(text) __declspec(deprecated(text))
	    #define MYGUI_OBSOLETE_END
	#endif

#elif MYGUI_COMPILER == MYGUI_COMPILER_GNUC
	#if MYGUI_PLATFORM == MYGUI_PLATFORM_LINUX && MYGUI_COMP_VER == 412
		#define MYGUI_OBSOLETE_START(text)
        #define MYGUI_OBSOLETE_END
	#else
        #define MYGUI_OBSOLETE_START(text)
		#define MYGUI_OBSOLETE_END __attribute__((deprecated))
	#endif

#else
	#define MYGUI_OBSOLETE_START(text)
	#define MYGUI_OBSOLETE_END

#endif

#define MYGUI_OBSOLETE(text) /*! \deprecated text */ MYGUI_OBSOLETE_START(text)MYGUI_OBSOLETE_END

#endif // __MYGUI_DIAGNOSTIC_H__
