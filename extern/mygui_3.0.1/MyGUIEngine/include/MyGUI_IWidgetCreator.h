/*!
	@file
	@author		Albert Semenov
	@date		03/2008
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
#ifndef __MYGUI_WIDGET_CREATOR_H__
#define __MYGUI_WIDGET_CREATOR_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Types.h"
#include "MyGUI_WidgetStyle.h"

namespace MyGUI
{

	class MYGUI_EXPORT IWidgetCreator
	{
		friend class WidgetManager;

	public:
		virtual ~IWidgetCreator() { }

	protected:
		virtual Widget* baseCreateWidget(WidgetStyle _style, const std::string& _type, const std::string& _skin, const IntCoord& _coord, Align _align, const std::string& _layer, const std::string& _name) = 0;

		// удяляет неудачника
		virtual void _destroyChildWidget(Widget* _widget) = 0;

		// удаляет всех детей
		virtual void _destroyAllChildWidget() = 0;

		// удаляет виджет с закрытым конструктором
		void _deleteWidget(Widget* _widget);

	public:
		// добавляет в список виджет
		// имплементировать только для рутовых креаторов
		virtual void _linkChildWidget(Widget* _widget) { }
		// удаляет из списка
		// имплементировать только для рутовых креаторов
		virtual void _unlinkChildWidget(Widget* _widget) { }

	};

} // namespace MyGUI

#endif // __MYGUI_WIDGET_CREATOR_H__
