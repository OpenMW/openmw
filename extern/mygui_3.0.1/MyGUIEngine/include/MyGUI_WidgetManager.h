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
#ifndef __MYGUI_WIDGET_MANAGER_H__
#define __MYGUI_WIDGET_MANAGER_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Instance.h"
#include "MyGUI_IWidgetCreator.h"
#include "MyGUI_IUnlinkWidget.h"
#include "MyGUI_ICroppedRectangle.h"
#include "MyGUI_Widget.h"
#include <set>

namespace MyGUI
{

	//OBSOLETE
	typedef delegates::CDelegate3<Widget*,  const std::string &, const std::string &> ParseDelegate;

	class MYGUI_EXPORT WidgetManager
	{
		MYGUI_INSTANCE_HEADER( WidgetManager )

	public:
		//OBSOLETE
		typedef std::map<std::string, ParseDelegate> MapDelegate;
		//OBSOLETE
		typedef std::set<IWidgetFactory*> SetWidgetFactory;

	public:
		void initialise();
		void shutdown();

		Widget* createWidget(WidgetStyle _style, const std::string& _type, const std::string& _skin, const IntCoord& _coord, Align _align, Widget* _parent, ICroppedRectangle * _cropeedParent, IWidgetCreator * _creator, const std::string& _name);

		/** Destroy _widget */
		void destroyWidget(Widget* _widget);
		/** Destroy vector of widgets */
		void destroyWidgets(const VectorWidgetPtr &_widgets);
		/** Destroy Enumerator of widgets */
		void destroyWidgets(EnumeratorWidgetPtr _widgets);

		/** Register unlinker (call unlink if for any destroyed widget)*/
		void registerUnlinker(IUnlinkWidget * _unlink);
		/** Unregister unlinker (call unlink if for any destroyed widget)*/
		void unregisterUnlinker(IUnlinkWidget * _unlink);
		/** Unlink widget */
		void unlinkFromUnlinkers(Widget* _widget);

		// добавляет виджет в список для анлинка
		void addWidgetToUnlink(Widget* _widget);

		// проверяет, и если надо обнуляет виджет из списка анликнутых
		void removeWidgetFromUnlink(Widget*& _widget);

		bool isFactoryExist(const std::string& _type);

	/*obsolete:*/
#ifndef MYGUI_DONT_USE_OBSOLETE

		MYGUI_OBSOLETE("use : void WidgetManager::destroyWidgets(VectorWidgetPtr &_widgets)")
		void destroyWidgetsVector(VectorWidgetPtr &_widgets) { destroyWidgets(_widgets); }
		MYGUI_OBSOLETE("")
		Widget* findWidgetT(const std::string& _name, bool _throw = true);
		MYGUI_OBSOLETE("")
		Widget* findWidgetT(const std::string& _name, const std::string& _prefix, bool _throw = true);
		MYGUI_OBSOLETE("")
		void registerFactory(IWidgetFactory * _factory);
		MYGUI_OBSOLETE("")
		void unregisterFactory(IWidgetFactory * _factory);
		MYGUI_OBSOLETE("use : void Widget::setProperty(const std::string &_key, const std::string &_value)")
		void parse(Widget* _widget, const std::string &_key, const std::string &_value);
		MYGUI_OBSOLETE("")
		ParseDelegate& registerDelegate(const std::string& _key);
		MYGUI_OBSOLETE("")
		void unregisterDelegate(const std::string& _key);

		template <typename T>
		MYGUI_OBSOLETE("")
		T* findWidget(const std::string& _name, bool _throw = true)
		{
			Widget* widget = findWidgetT(_name, _throw);
			if (nullptr == widget) return nullptr;
			return widget->castType<T>(_throw);
		}

		template <typename T>
		MYGUI_OBSOLETE("")
		T* findWidget(const std::string& _name, const std::string& _prefix, bool _throw = true)
		{
			return findWidget<T>(_prefix + _name, _throw);
		}

#endif // MYGUI_DONT_USE_OBSOLETE

	protected:
		SetWidgetFactory mFactoryList;
		//MapWidgetPtr mWidgets;
		MapDelegate mDelegates;

		// список менеджеров для отписки при удалении
		VectorIUnlinkWidget mVectorIUnlinkWidget;

		// список виджетов для отписки
		VectorWidgetPtr mUnlinkWidgets;
	};

} // namespace MyGUI

#endif // __MYGUI_WIDGET_MANAGER_H__
