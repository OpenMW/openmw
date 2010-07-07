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
#include "MyGUI_LogStream.h"
#include "MyGUI_LogManager.h"
#include <iomanip>
#include <time.h>

namespace MyGUI
{

	LogStream::LogStream() { }
	LogStream::~LogStream()
	{
		if (mStream.is_open())
		{
			mStream.close();
			release();
		}
	}

	LogStream::LogStream(const std::string& _file) : mFileName(_file)
	{
		lock();

		struct tm *current_time;
		time_t ctTime; time(&ctTime);
		current_time = localtime( &ctTime );

		mStream.open(mFileName.c_str(), std::ios_base::out);
		if (mStream.is_open())
		{
			mStream << " ---------------------------------------------------------------------------------------------------------------------------------- " << std::endl;
			mStream << "                                          loging report for : "
				<< std::setw(2) << std::setfill('0') << current_time->tm_mon + 1 << "/"
				<< std::setw(2) << std::setfill('0') << current_time->tm_mday << "/"
				<< std::setw(4) << std::setfill('0') << current_time->tm_year + 1900 << "   "
				<< std::setw(2) << std::setfill('0') << current_time->tm_hour << ":"
				<< std::setw(2) << std::setfill('0') << current_time->tm_min << ":"
				<< std::setw(2) << std::setfill('0') << current_time->tm_sec << std::endl;
			mStream << " ---------------------------------------------------------------------------------------------------------------------------------- " << std::endl << std::endl;
			mStream.close();
		}

		release();
	}

	void LogStream::start(const std::string& _section, const std::string& _level)
	{
		if (mStream.is_open())
		{
			mStream.close();
			release();
		}

		lock();

		struct tm *current_time;
		time_t ctTime; time(&ctTime);
		current_time = localtime( &ctTime );

		if (!mFileName.empty())
		{
			mStream.open(mFileName.c_str(), std::ios_base::app);
			if (mStream.is_open())
			{
				mStream << std::setw(2) << std::setfill('0') << current_time->tm_hour << ":"
					<< std::setw(2) << std::setfill('0') << current_time->tm_min << ":"
					<< std::setw(2) << std::setfill('0') << current_time->tm_sec << LogManager::separator
					<< _section << LogManager::separator << _level << LogManager::separator;
			}
		}
	}

	bool LogStream::getSTDOutputEnabled()
	{
		return LogManager::getSTDOutputEnabled();
	}

	LogStream& LogStream::operator<<(const LogStreamEnd& _endl)
	{
		if (getSTDOutputEnabled()) std::cout << std::endl;
		if (mStream.is_open())
		{
			mStream << std::endl;
			mStream.close();
		}
		release();

		return *this;
	}

} // namespace MyGUI
