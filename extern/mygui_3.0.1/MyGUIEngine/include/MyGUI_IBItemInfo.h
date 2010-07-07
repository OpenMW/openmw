/*!
	@file
	@author		Albert Semenov
	@date		11/2008
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
#ifndef __MYGUI_ITEM_INFO_H__
#define __MYGUI_ITEM_INFO_H__

#include "MyGUI_Prerequest.h"

namespace MyGUI
{

	// структура информации об отображении элемента
	/** Info about ItemBox item*/
	struct MYGUI_EXPORT IBDrawItemInfo
	{

		IBDrawItemInfo() :
			index(ITEM_NONE),
			update(false),
			select(false),
			active(false),
			drag(false),
			drop_accept(false),
			drop_refuse(false)
		{
		}

		IBDrawItemInfo(size_t _index, size_t _select, size_t _active, size_t _accept, size_t _refuse, bool _update, bool _drag) :
			index(_index),
			update(_update),
			select(_index == _select),
			active(_index == _active),
			drag(_drag),
			drop_accept(_index == _accept),
			drop_refuse(_index == _refuse)
		{
		}

		/** Index of element */
		size_t index;
		/** State and interdan data changed */
		bool update;
		/** Is widget selected */
		bool select;
		/** Is widget active */
		bool active;
		/** Is widget able to be dragged */
		bool drag;
		/** Is widget accepting drop */
		bool drop_accept;
		/** Is widget refuseing drop */
		bool drop_refuse;
	};

	struct MYGUI_EXPORT IBNotifyItemData
	{
		enum NotifyItem
		{
			MousePressed,
			MouseReleased,
			KeyPressed,
			KeyReleased
		};

		IBNotifyItemData(size_t _index, NotifyItem _notify, int _x, int _y, MouseButton _id) :
			index(_index), notify(_notify), x(_x), y(_y), id(_id), code(KeyCode::None), key(0) { }

		IBNotifyItemData(size_t _index, NotifyItem _notify, KeyCode _code, Char _key) :
			index(_index), notify(_notify), x(0), y(0), id(MouseButton::None), code(_code), key(_key) { }

		IBNotifyItemData(size_t _index, NotifyItem _notify, KeyCode _code) :
			index(_index), notify(_notify), x(0), y(0), id(MouseButton::None), code(_code), key(KeyCode::None) { }

		/** Item index */
		size_t index;
		/** Notify type */
		NotifyItem notify;
		/** If Mouse* notify type - mouse x position, else 0 */
		int x;
		/** If Mouse* notify type - mouse y position, else 0 */
		int y;
		/** If Mouse* notify type - mouse button id position, else 0 */
		MouseButton id;
		/** If Key* notify type - key code, else 0 */
		KeyCode code;
		/** If Key* notify type - mouse button id position, else 0 */
		Char key;
	};

} // namespace MyGUI

#endif // __MYGUI_ITEM_INFO_H__
