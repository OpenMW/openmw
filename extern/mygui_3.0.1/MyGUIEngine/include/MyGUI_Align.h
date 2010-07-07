/*!
	@file
	@author		Albert Semenov
	@date		08/2008
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
#ifndef __MYGUI_ALIGN_H__
#define __MYGUI_ALIGN_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Macros.h"
#include "MyGUI_Diagnostic.h"
#include <map>

namespace MyGUI
{

	struct MYGUI_EXPORT Align
	{
		enum Enum
		{
			HCenter = MYGUI_FLAG_NONE, /**< center horizontally */
			VCenter = MYGUI_FLAG_NONE, /**< center vertically */
			Center = HCenter | VCenter, /**< center in the dead center */

			Left = MYGUI_FLAG(1), /**< value from the left (and center vertically) */
			Right = MYGUI_FLAG(2), /**< value from the right (and center vertically) */
			HStretch = Left | Right, /**< stretch horizontally proportionate to parent window (and center vertically) */

			Top = MYGUI_FLAG(3), /**< value from the top (and center horizontally) */
			Bottom = MYGUI_FLAG(4), /**< value from the bottom (and center horizontally) */
			VStretch = Top | Bottom, /**< stretch vertically proportionate to parent window (and center horizontally) */

			Stretch = HStretch | VStretch, /**< stretch proportionate to parent window */
			Default = Left | Top, /**< default value (value from left and top) */

			HRelative = MYGUI_FLAG(5),
			VRelative = MYGUI_FLAG(6),
			Relative = HRelative | VRelative
		};

		Align(Enum _value = Default) : value(_value) { }

		bool isHCenter() const { return HCenter == (value & ((int)HStretch | (int)HRelative)); }
		bool isVCenter() const { return VCenter == (value & ((int)VStretch | (int)VRelative)); }
		bool isCenter() const { return Center == (value & ((int)Stretch | (int)Relative)); }

		bool isLeft() const { return Left == (value & ((int)HStretch | (int)HRelative)); }
		bool isRight() const { return Right == (value & ((int)HStretch | (int)HRelative)); }
		bool isHStretch() const { return HStretch == (value & ((int)HStretch | (int)HRelative)); }

		bool isTop() const { return Top == (value & ((int)VStretch | (int)VRelative)); }
		bool isBottom() const { return (Bottom == (value & ((int)VStretch | (int)VRelative))); }
		bool isVStretch() const { return (VStretch == (value & ((int)VStretch | (int)VRelative))); }

		bool isStretch() const { return (Stretch == (value & ((int)Stretch | (int)Relative))); }
		bool isDefault() const { return (Default == (value & ((int)Stretch | (int)Relative))); }

		bool isHRelative() const { return HRelative == (value & (int)HRelative); }
		bool isVRelative() const { return VRelative == (value & (int)VRelative); }
		bool isRelative() const { return Relative == (value & (int)Relative); }

		Align& operator |= (Align const& _other) { value = Enum(int(value) | int(_other.value)); return *this; }
		friend Align operator | (Enum const& a, Enum const& b) { return Align(Enum(int(a) | int(b))); }
		friend Align operator | (Align const& a, Align const& b) { return Align(Enum(int(a.value) | int(b.value))); }

		friend bool operator == (Align const& a, Align const& b) { return a.value == b.value; }
		friend bool operator != (Align const& a, Align const& b) { return a.value != b.value; }

		typedef std::map<std::string, int> MapAlign;

		static Align parse(const std::string& _value)
		{
			Align result(Enum(0));
			const MapAlign& map_names = result.getValueNames();
			const std::vector<std::string>& vec = utility::split(_value);
			for (size_t pos=0; pos<vec.size(); pos++)
			{
				MapAlign::const_iterator iter = map_names.find(vec[pos]);
				if (iter != map_names.end())
				{
					result.value = Enum(int(result.value) | int(iter->second));
				}
			}
			return result;
		}

		std::string print() const
		{
			std::string result;

			if (value & Left)
			{
				if (value & Right) result = "HStretch";
				else result = "Left";
			}
			else if (value & Right) result = "Right";
			else result = "HCenter";

			if (value & Top)
			{
				if (value & Bottom) result += " VStretch";
				else result += " Top";
			}
			else if (value & Bottom) result += " Bottom";
			else result += " VCenter";

			return result;
		}

		friend std::ostream& operator << ( std::ostream& _stream, const Align&  _value )
		{
			_stream << _value.print();
			return _stream;
		}

		friend std::istream& operator >> ( std::istream& _stream, Align&  _value )
		{
			_value.value = Enum(0);
			std::string value;
			_stream >> value;

			const MapAlign& map_names = _value.getValueNames();
			MapAlign::const_iterator iter = map_names.find(value);
			if (iter != map_names.end())
				_value.value = Enum(int(_value.value) | int(iter->second));


			if (!_stream.eof())
			{
				std::string value2;
				_stream >> value2;
				iter = map_names.find(value2);
				if (iter != map_names.end())
					_value.value = Enum(int(_value.value) | int(iter->second));
			}

			return _stream;
		}

	private:
		const MapAlign& getValueNames() const
		{
			static MapAlign map_names;

			if (map_names.empty())
			{
				// OBSOLETE
				map_names["ALIGN_HCENTER"] = HCenter;
				map_names["ALIGN_VCENTER"] = VCenter;
				map_names["ALIGN_CENTER"] = Center;
				map_names["ALIGN_LEFT"] = Left;
				map_names["ALIGN_RIGHT"] = Right;
				map_names["ALIGN_HSTRETCH"] = HStretch;
				map_names["ALIGN_TOP"] = Top;
				map_names["ALIGN_BOTTOM"] = Bottom;
				map_names["ALIGN_VSTRETCH"] = VStretch;
				map_names["ALIGN_STRETCH"] = Stretch;
				map_names["ALIGN_DEFAULT"] = Default;

				MYGUI_REGISTER_VALUE(map_names, HCenter);
				MYGUI_REGISTER_VALUE(map_names, VCenter);
				MYGUI_REGISTER_VALUE(map_names, Center);
				MYGUI_REGISTER_VALUE(map_names, Left);
				MYGUI_REGISTER_VALUE(map_names, Right);
				MYGUI_REGISTER_VALUE(map_names, HStretch);
				MYGUI_REGISTER_VALUE(map_names, Top);
				MYGUI_REGISTER_VALUE(map_names, Bottom);
				MYGUI_REGISTER_VALUE(map_names, VStretch);
				MYGUI_REGISTER_VALUE(map_names, Stretch);
				MYGUI_REGISTER_VALUE(map_names, Default);
				MYGUI_REGISTER_VALUE(map_names, HRelative);
				MYGUI_REGISTER_VALUE(map_names, VRelative);
				MYGUI_REGISTER_VALUE(map_names, Relative);
			}

			return map_names;
		}

	private:
		Enum value;
	};

} // namespace MyGUI

#endif // __MYGUI_ALIGN_H__
