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
#ifndef __MYGUI_MESSAGE_BOX_STYLE_H__
#define __MYGUI_MESSAGE_BOX_STYLE_H__

#include "MyGUI_Prerequest.h"

namespace MyGUI
{


	struct MYGUI_EXPORT MessageBoxStyle
	{

		enum Enum
		{
			None = MYGUI_FLAG_NONE,
			Ok = MYGUI_FLAG(0),
			Yes = MYGUI_FLAG(1),
			No = MYGUI_FLAG(2),
			Abort = MYGUI_FLAG(3),
			Retry = MYGUI_FLAG(4),
			Ignore = MYGUI_FLAG(5),
			Cancel = MYGUI_FLAG(6),
			Try = MYGUI_FLAG(7),
			Continue = MYGUI_FLAG(8),

			_IndexUserButton1 = 9, // индекс первой кнопки юзера

			Button1 = MYGUI_FLAG(_IndexUserButton1),
			Button2 = MYGUI_FLAG(_IndexUserButton1 + 1),
			Button3 = MYGUI_FLAG(_IndexUserButton1 + 2),
			Button4 = MYGUI_FLAG(_IndexUserButton1 + 3),

			_CountUserButtons = 4, // колличество кнопок юзера
			_IndexIcon1 = _IndexUserButton1 + _CountUserButtons, // индекс первой иконки

			IconDefault = MYGUI_FLAG(_IndexIcon1),

			IconInfo = MYGUI_FLAG(_IndexIcon1),
			IconQuest = MYGUI_FLAG(_IndexIcon1 + 1),
			IconError = MYGUI_FLAG(_IndexIcon1 + 2),
			IconWarning = MYGUI_FLAG(_IndexIcon1 + 3),

			Icon1 = MYGUI_FLAG(_IndexIcon1),
			Icon2 = MYGUI_FLAG(_IndexIcon1 + 1),
			Icon3 = MYGUI_FLAG(_IndexIcon1 + 2),
			Icon4 = MYGUI_FLAG(_IndexIcon1 + 3),
			Icon5 = MYGUI_FLAG(_IndexIcon1 + 4),
			Icon6 = MYGUI_FLAG(_IndexIcon1 + 5),
			Icon7 = MYGUI_FLAG(_IndexIcon1 + 6),
			Icon8 = MYGUI_FLAG(_IndexIcon1 + 7)
		};

		MessageBoxStyle(Enum _value = None) : value(_value) { }

		MessageBoxStyle& operator |= (MessageBoxStyle const& _other) { value = Enum(int(value) | int(_other.value)); return *this; }
		friend MessageBoxStyle operator | (Enum const& a, Enum const& b) { return MessageBoxStyle(Enum(int(a) | int(b))); }
		MessageBoxStyle operator | (Enum const& a) { return MessageBoxStyle(Enum(int(value) | int(a))); }

		friend bool operator == (MessageBoxStyle const& a, MessageBoxStyle const& b) { return a.value == b.value; }
		friend bool operator != (MessageBoxStyle const& a, MessageBoxStyle const& b) { return a.value != b.value; }

		friend std::ostream& operator << ( std::ostream& _stream, const MessageBoxStyle&  _value )
		{
			//_stream << _value.print();
			return _stream;
		}

		friend std::istream& operator >> ( std::istream& _stream, MessageBoxStyle&  _value )
		{
			std::string value;
			_stream >> value;
			_value = MessageBoxStyle::parse(value);
			return _stream;
		}

		// возвращает индекс иконки
		size_t getIconIndex()
		{
			size_t index = 0;
			int num = value >> _IndexIcon1;

			while (num != 0)
			{
				if ((num & 1) == 1) return index;

				++index;
				num >>= 1;
			}

			return ITEM_NONE;
		}

		// возвращает индекс иконки
		size_t getButtonIndex()
		{
			size_t index = 0;
			int num = value;

			while (num != 0)
			{
				if ((num & 1) == 1) return index;

				++index;
				num >>= 1;
			}

			return ITEM_NONE;
		}

		// возвращает список кнопок
		std::vector<MessageBoxStyle> getButtons()
		{
			std::vector<MessageBoxStyle> buttons;

			size_t index = 0;
			int num = value;
			while (index < _IndexIcon1)
			{
				if ((num & 1) == 1)
				{
					buttons.push_back( MessageBoxStyle::Enum( MYGUI_FLAG(index) ) );
				}

				++index;
				num >>= 1;
			}

			return buttons;
		}

		typedef std::map<std::string, int> MapAlign;

		static MessageBoxStyle parse(const std::string& _value)
		{
			MessageBoxStyle result(MessageBoxStyle::Enum(0));
			const MapAlign& map_names = result.getValueNames();
			const std::vector<std::string>& vec = utility::split(_value);
			for (size_t pos=0; pos<vec.size(); pos++)
			{
				MapAlign::const_iterator iter = map_names.find(vec[pos]);
				if (iter != map_names.end())
				{
					result.value = Enum(int(result.value) | int(iter->second));
				}
				else
				{
					MYGUI_LOG(Warning, "Cannot parse type '" << vec[pos] << "'");
				}
			}
			return result;
		}

	private:
		const MapAlign& getValueNames()
		{
			static MapAlign map_names;

			if (map_names.empty())
			{
				MYGUI_REGISTER_VALUE(map_names, None);
				MYGUI_REGISTER_VALUE(map_names, Ok);
				MYGUI_REGISTER_VALUE(map_names, Yes);
				MYGUI_REGISTER_VALUE(map_names, No);
				MYGUI_REGISTER_VALUE(map_names, Abort);
				MYGUI_REGISTER_VALUE(map_names, Retry);
				MYGUI_REGISTER_VALUE(map_names, Ignore);
				MYGUI_REGISTER_VALUE(map_names, Cancel);
				MYGUI_REGISTER_VALUE(map_names, Try);
				MYGUI_REGISTER_VALUE(map_names, Continue);

				MYGUI_REGISTER_VALUE(map_names, Button1);
				MYGUI_REGISTER_VALUE(map_names, Button2);
				MYGUI_REGISTER_VALUE(map_names, Button3);
				MYGUI_REGISTER_VALUE(map_names, Button4);

				MYGUI_REGISTER_VALUE(map_names, IconDefault);

				MYGUI_REGISTER_VALUE(map_names, IconInfo);
				MYGUI_REGISTER_VALUE(map_names, IconQuest);
				MYGUI_REGISTER_VALUE(map_names, IconError);
				MYGUI_REGISTER_VALUE(map_names, IconWarning);

				MYGUI_REGISTER_VALUE(map_names, Icon1);
				MYGUI_REGISTER_VALUE(map_names, Icon2);
				MYGUI_REGISTER_VALUE(map_names, Icon3);
				MYGUI_REGISTER_VALUE(map_names, Icon4);
				MYGUI_REGISTER_VALUE(map_names, Icon5);
				MYGUI_REGISTER_VALUE(map_names, Icon6);
				MYGUI_REGISTER_VALUE(map_names, Icon7);
				MYGUI_REGISTER_VALUE(map_names, Icon8);
			}

			return map_names;
		}

	private:
		Enum value;
	};

} // namespace MyGUI

#endif // __MYGUI_MESSAGE_BOX_STYLE_H__
