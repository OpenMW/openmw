/*!
	@file
	@author		Albert Semenov
	@date		08/2009
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
#ifndef __MYGUI_I_DATA_STREAM_H__
#define __MYGUI_I_DATA_STREAM_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Types.h"

namespace MyGUI
{

	class MYGUI_EXPORT IDataStream
	{
	public:
		virtual ~IDataStream() { }

		virtual bool eof() = 0;
		virtual size_t size() = 0;
		virtual void readline(std::string& _source, Char _delim = '\n') = 0;
		virtual size_t read(void* _buf, size_t _count) = 0;

	};

} // namespace MyGUI

#endif // __MYGUI_I_DATA_STREAM_H__
