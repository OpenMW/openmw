/*!
	@file
	@author		Albert Semenov
	@date		04/2009
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
#include "MyGUI_Timer.h"

#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
#	include <windows.h>
#	ifndef __MINGW32__
#		pragma comment(lib, "winmm.lib")
#	else
#		pragma comment(lib, "libwinmm.a")
#	endif
#else
#	include <sys/time.h>
#endif

namespace MyGUI
{

	Timer::Timer() :
		mTimeStart(0)
	{

	}

	void Timer::reset()
	{
		mTimeStart = getCurrentMilliseconds();
	}

	unsigned long Timer::getMilliseconds()
	{
		return getCurrentMilliseconds() - mTimeStart;
	}

	unsigned long Timer::getCurrentMilliseconds()
	{
#if MYGUI_PLATFORM == MYGUI_PLATFORM_WIN32
		/*
		We do this because clock() is not affected by timeBeginPeriod on Win32.
		QueryPerformanceCounter is a little overkill for the amount of precision that
		I consider acceptable. If someone submits a patch that replaces this code
		with QueryPerformanceCounter, I wouldn't complain. Until then, timeGetTime
		gets the results I'm after. -EMS

		See: http://www.geisswerks.com/ryan/FAQS/timing.html
		And: http://support.microsoft.com/default.aspx?scid=KB;EN-US;Q274323&
		*/
		return timeGetTime();
#else
		struct timeval now;
		gettimeofday(&now, NULL);
		return (now.tv_sec)*1000+(now.tv_usec)/1000;
		//return ( unsigned long )(( double )( clock() ) / (( double )CLOCKS_PER_SEC / 1000.0 ) );
#endif
	}


} // namespace MyGUI
