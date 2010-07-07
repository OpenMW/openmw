/*!
	@file
	@author		Albert Semenov
	@date		01/2008
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
#ifndef __MYGUI_LOG_STREAM_H__
#define __MYGUI_LOG_STREAM_H__

#include "MyGUI_Prerequest.h"
#include <fstream>
#include <iostream>
#include <string>

namespace MyGUI
{

	class MYGUI_EXPORT LogStream
	{
		friend class LogManager;

	public:
		struct LogStreamEnd { };

	public:
		LogStream& operator<<(const LogStreamEnd& _endl);

		template <typename T>
		inline LogStream& operator<<(T _value)
		{
			if (getSTDOutputEnabled()) std::cout << _value;
			if (mStream.is_open()) mStream << _value;
			return *this;
		}

		const std::string& getFileName() const { return mFileName; }

	private:
		LogStream();
		~LogStream();

		LogStream(const std::string& _file);

		void start(const std::string& _section, const std::string& _level);

		bool getSTDOutputEnabled();

		void lock() const { }
		void release() const { }

	private:
		std::ofstream mStream;
		std::string mFileName;
	};

} // namespace MyGUI

#endif // __MYGUI_LOG_STREAM_H__
