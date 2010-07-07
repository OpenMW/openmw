/*!
	@file
	@author		Albert Semenov
	@date		05/2009
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
#ifndef __MYGUI_EXCEPTION_H__
#define __MYGUI_EXCEPTION_H__

#include "MyGUI_Prerequest.h"
#include <exception>
#include <string>

namespace MyGUI
{

	class MYGUI_EXPORT Exception : public std::exception
	{
	protected:
		std::string mDescription;
		std::string mSource;
		std::string mFile;
		long mLine;
		mutable std::string mFullDesc;

	public:
		Exception(const std::string& _description, const std::string& _source, const char* _file, long _line );

		Exception(const Exception& _rhs);

		// Needed for  compatibility with std::exception
		~Exception() throw() { }

		Exception& operator = (const Exception& _rhs);

		virtual const std::string& getFullDescription() const;

		virtual const std::string &getSource() const { return mSource; }

		virtual const std::string &getFile() const { return mFile; }

		virtual long getLine() const { return mLine; }

		virtual const std::string& getDescription() const { return mDescription; }

		// Override std::exception::what
		const char* what() const throw() { return getFullDescription().c_str(); }

	};

} // namespace MyGUI

#endif // __MYGUI_EXCEPTION_H__
