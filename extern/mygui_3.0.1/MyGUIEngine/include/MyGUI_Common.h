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
#ifndef __MYGUI_COMMON_H__
#define __MYGUI_COMMON_H__

#include "MyGUI_Prerequest.h"

#include <string>
#include <list>
#include <set>
#include <map>
#include <vector>
#include <deque>
#include <exception>
#include <math.h>

#ifdef MYGUI_CUSTOM_ALLOCATOR
#    include "MyGUI_CustomAllocator.h"
#else // MYGUI_CUSTOM_ALLOCATOR
#    include "MyGUI_Allocator.h"
#endif // MYGUI_CUSTOM_ALLOCATOR

#include "MyGUI_Macros.h"
#include "MyGUI_Diagnostic.h"
#include "MyGUI_LogManager.h"
#include "MyGUI_Instance.h"
#include "MyGUI_Types.h"
#include "MyGUI_RenderOut.h"
#include "MyGUI_Utility.h"
#include "MyGUI_InputDefine.h"
#include "MyGUI_Version.h"
#include "MyGUI_WidgetStyle.h"
#include "MyGUI_UString.h"
#include "MyGUI_Delegate.h"

#endif // __MYGUI_COMMON_H__
