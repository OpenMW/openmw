/*!
	@file
	@author		Albert Semenov
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
#ifndef __MYGUI_TEXT_CHANGE_HISTORY_H__
#define __MYGUI_TEXT_CHANGE_HISTORY_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Macros.h"
#include "MyGUI_UString.h"
#include <deque>

namespace MyGUI
{

	// инфо об одной операции
	struct TextCommandInfo
	{
		// типы операций
		enum CommandType
		{
			COMMAND_POSITION,
			COMMAND_INSERT,
			COMMAND_ERASE
		};

		// для удаления и вставки текста
		TextCommandInfo(const UString& _text, size_t _start, CommandType _type)
			: text(_text), type(_type), start(_start), undo(ITEM_NONE), redo(ITEM_NONE), length(ITEM_NONE) { }
		// для указания позиции
		TextCommandInfo(size_t _undo, size_t _redo, size_t _length)
			: type(COMMAND_POSITION), start(ITEM_NONE), undo(_undo), redo(_redo), length(_length) { }

		// строка харрактиризуещая изменения
		UString text;
		// тип операции
		CommandType type;
		// инфа о начале позиции
		size_t start;
		// инфа о псевдо позиции
		size_t undo, redo, length;
	};

	typedef std::vector<TextCommandInfo> VectorChangeInfo;
	typedef std::deque<VectorChangeInfo> DequeUndoRedoInfo;

} // namespace MyGUI

#endif // __MYGUI_TEXT_CHANGE_HISTORY_H__
