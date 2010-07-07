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
#ifndef __MYGUI_I_WIDGET_FACTORY_H__
#define __MYGUI_I_WIDGET_FACTORY_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Types.h"
#include "MyGUI_WidgetStyle.h"
#include "MyGUI_WidgetDefines.h"
#include "MyGUI_ICroppedRectangle.h"

#include "MyGUI_WidgetManager.h"
#include "MyGUI_SkinManager.h"

namespace MyGUI
{

	//OBSOLETE
	class MYGUI_EXPORT IWidgetFactory
	{
	public:
		virtual ~IWidgetFactory() { }

		virtual const std::string& getTypeName() = 0;
		virtual Widget* createWidget(WidgetStyle _style, const std::string& _skin, const IntCoord& _coord, Align _align, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name) = 0;
	};

	namespace factory
	{

		//OBSOLETE
		template <typename T>
		class MYGUI_EXPORT BaseWidgetFactory : public IWidgetFactory
		{
		public:
			BaseWidgetFactory()
			{
				// регестрируем себя
				MyGUI::WidgetManager& manager = MyGUI::WidgetManager::getInstance();
				manager.registerFactory(this);
			}

			~BaseWidgetFactory()
			{
				// удаляем себя
				MyGUI::WidgetManager& manager = MyGUI::WidgetManager::getInstance();
				manager.unregisterFactory(this);
			}

			const std::string& getTypeName()
			{
				return T::getClassTypeName();
			}

			Widget* createWidget(WidgetStyle _style, const std::string& _skin, const IntCoord& _coord, Align _align, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name)
			{
				T* instance = new T(_style, _coord, _align, SkinManager::getInstance().getByName(_skin), _parent, _croppedParent, _creator, _name);
				return instance;
			}

			bool isFalseType(Widget* _ptr, const std::string &_key)
			{
				if (!_ptr->isType<T>())
				{
					MYGUI_LOG(Error, "Property '" << _key << "' is not supported by '" << _ptr->getTypeName() << "' widget");
					return true;
				}
				return false;
			}
		};

	} // namespace factory
} // namespace MyGUI

#endif // __MYGUI_I_WIDGET_FACTORY_H__
