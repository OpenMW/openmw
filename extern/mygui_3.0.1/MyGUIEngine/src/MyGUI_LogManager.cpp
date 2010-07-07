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
#include "MyGUI_Precompiled.h"
#include "MyGUI_LogManager.h"
#include <sstream>
#include <assert.h> // REMOVEME

namespace MyGUI
{

	const std::string LogManager::LevelsName[EndLogLevel] =
	{
		"Info",
		"Warning",
		"Error",
		"Critical"
	};

	const std::string LogManager::General = "General";
	const std::string LogManager::separator = "  |  ";

	LogStream::LogStreamEnd LogManager::endl;
	LogManager* LogManager::msInstance = 0;

	LogManager::LogManager()
	{
		msInstance = this;
		mSTDOut = true;
	}

	LogManager::~LogManager()
	{
		MapLogStream& mapStream = msInstance->mMapSectionFileName;
		for (MapLogStream::iterator iter=mapStream.begin(); iter!=mapStream.end(); ++iter)
		{
			LogStream * stream = iter->second;
			if (stream == 0) continue;

			// ищем все такие потоки и обнуляем
			for (MapLogStream::iterator iter2=iter; iter2!=mapStream.end(); ++iter2)
			{
				if (iter2->second == stream) iter2->second = 0;
			}
			delete stream;
		}
		mapStream.clear();
		msInstance = nullptr;
	}

	void LogManager::shutdown()
	{
		if (msInstance != nullptr)
		{
			delete msInstance;
			msInstance = nullptr;
		}
	}

	void LogManager::initialise()
	{
		if (msInstance == nullptr)
		{
			msInstance = new LogManager();
		}
	}

	LogStream& LogManager::out(const std::string& _section, LogLevel _level)
	{
		static LogStream empty;

		if (msInstance == nullptr)
			return empty;

		MapLogStream& mapStream = msInstance->mMapSectionFileName;
		MapLogStream::iterator iter = mapStream.find(_section);
		if (iter == mapStream.end())
			return empty;

		if (_level >= EndLogLevel)
			_level = Info;

		iter->second->start(_section, LevelsName[_level]);

		return *(iter->second);
	}

	void LogManager::registerSection(const std::string& _section, const std::string& _file)
	{
		if (0 == msInstance) new LogManager();

		// ищем такую же секцию и удаляем ее
		MapLogStream& mapStream = msInstance->mMapSectionFileName;
		/*MapLogStream::iterator iter = mapStream.find(_section);
		if (iter != mapStream.end())
		{
			delete iter->second;
			mapStream.erase(iter);
		}*/

		// ищем поток с таким же именем, если нет, то создаем
		LogStream * stream = 0;
		for (MapLogStream::iterator iter=mapStream.begin(); iter!=mapStream.end(); ++iter)
		{
			if (iter->second->getFileName() == _file)
			{
				stream = iter->second;
				break;
			}
		}
		if (0 == stream)
			stream = new LogStream(_file);

		mapStream[_section] = stream;
	}

	void LogManager::unregisterSection(const std::string& _section)
	{
		MapLogStream& mapStream = msInstance->mMapSectionFileName;
		MapLogStream::iterator iter = mapStream.find(_section);
		if (iter == mapStream.end()) return;

		LogStream * stream = iter->second;
		mapStream.erase(iter);

		// если файл еще используеться то удалять не надо
		for (iter=mapStream.begin(); iter!=mapStream.end(); ++iter)
		{
			if (iter->second == stream)
				return;
		}

		delete stream;

		if (mapStream.size() == 0) shutdown();
	}

	const std::string& LogManager::info(const char * _file /* = __FILE__*/, int _line /* = __LINE__*/)
	{
		std::ostringstream stream;
		stream << separator << _file << separator << _line;

		static std::string ret;
		ret = stream.str();
		return ret;
	}

	const LogStream::LogStreamEnd& LogManager::end()
	{
		return endl;
	}

	void LogManager::setSTDOutputEnabled(bool _enable)
	{
		assert(msInstance);
		msInstance->mSTDOut = _enable;
	}

	bool LogManager::getSTDOutputEnabled()
	{
		assert(msInstance);
		return msInstance->mSTDOut;
	}

} // namespace MyGUI
