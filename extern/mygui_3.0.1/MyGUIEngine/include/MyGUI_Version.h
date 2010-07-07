/*!
	@file
	@author		Albert Semenov
	@date		12/2008
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
#ifndef __MYGUI_VERSION_H__
#define __MYGUI_VERSION_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Types.h"
#include "MyGUI_Utility.h"

namespace MyGUI
{

	class MYGUI_EXPORT Version
	{
	public:
		Version() : value(0) { }
		Version(uint8 _major, uint8 _minor, uint16 _patch) : value((uint32(_major) << 24) + (uint32(_minor) << 16) + uint32(_patch)) { }
		Version(uint8 _major, uint8 _minor) : value((uint32(_major) << 24) + (uint32(_minor) << 16)) { }
		explicit Version(const std::string& _value) : value(parse(_value).value) { }

		friend bool operator < (Version const& a, Version const& b) { return a.getPoorVersion() < b.getPoorVersion(); }
		friend bool operator >= (Version const& a, Version const& b) { return !(a < b); }
		friend bool operator > (Version const& a, Version const& b) { return (b < a); }
		friend bool operator <= (Version const& a, Version const& b) { return !(a > b); }

		friend bool operator == (Version const& a, Version const& b) { return !(a < b) && !(a > b); }
		friend bool operator != (Version const& a, Version const& b) { return !(a == b); }

		friend std::ostream& operator << ( std::ostream& _stream, const Version&  _value )
		{
			_stream << _value.print();
			return _stream;
		}

		friend std::istream& operator >> ( std::istream& _stream, Version&  _value )
		{
			std::string value;
			_stream >> value;
			_value = Version::parse(value);
			return _stream;
		}

		uint8 getMajor() const { return uint8((value & 0xFF000000) >> 24); }
		uint8 getMinor() const { return uint8((value & 0x00FF0000) >> 16); }
		uint16 getPatch() const { return uint16(value & 0x0000FFFF); }

		uint32 getPoorVersion() const { return value & 0xFFFF0000; }
		uint32 getFullVersion() const { return value; }

		std::string print() const
		{
			if (getPatch() == 0) return utility::toString(getMajor(), ".", getMinor());
			return utility::toString(getMajor(), ".", getMinor(), ".", getPatch());
		}

		static Version parse(const std::string& _value)
		{
			const std::vector<std::string>& vec = utility::split(_value, ".");
			if (vec.empty()) return Version();
			uint8 major = (uint8)utility::parseValue<int>(vec[0]);
			uint8 minor = vec.size() > 1 ? (uint8)utility::parseValue<int>(vec[1]) : uint8(0);
			uint16 patch = vec.size() > 2 ? (uint16)utility::parseValue<int>(vec[2]) : uint16(0);
			return Version(major, minor, patch);
		}

	private:
		union
		{
			uint32 value;
			uint8 value_data[4];
		};
	};

} // namespace MyGUI

#endif // __MYGUI_VERSION_H__
