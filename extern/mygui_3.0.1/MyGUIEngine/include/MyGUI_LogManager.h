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
#ifndef __MYGUI_LOG_MANAGER_H__
#define __MYGUI_LOG_MANAGER_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_LogStream.h"
#include <map>

namespace MyGUI
{

	#define MYGUI_LOGGING(section, level, text) \
		MyGUI::LogManager::out(section, MyGUI::LogManager::level) \
		<< text \
		<< MyGUI::LogManager::info(__FILE__, __LINE__) \
		<< MyGUI::LogManager::end()

	class MYGUI_EXPORT LogManager
	{

	public:
		enum LogLevel
		{
			Info,
			Warning,
			Error,
			Critical,
			EndLogLevel
		};

	public:
		static void shutdown();
		static void initialise();

		static void registerSection(const std::string& _section, const std::string& _file);
		static void unregisterSection(const std::string& _section);

		static LogStream& out(const std::string& _section, LogLevel _level);
		static const std::string& info(const char * _file /* = __FILE__*/, int _line /* = __LINE__*/);

		static const LogStream::LogStreamEnd& end();

		// set logging enabled on std output device
		static void setSTDOutputEnabled(bool _enable);
		static bool getSTDOutputEnabled();

	private:
		LogManager();
		~LogManager();

	public:
		static const std::string General;
		static const std::string separator;

		static LogStream::LogStreamEnd endl;
		static const std::string LevelsName[EndLogLevel];

	private:
		static LogManager * msInstance;
		typedef std::map<std::string, LogStream*>  MapLogStream;
		MapLogStream mMapSectionFileName;
		bool mSTDOut;
	};

} // namespace MyGUI

#endif // __MYGUI_LOG_MANAGER_H__
