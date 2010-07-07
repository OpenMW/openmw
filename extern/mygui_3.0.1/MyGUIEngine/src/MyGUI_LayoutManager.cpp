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
#include "MyGUI_Precompiled.h"
#include "MyGUI_ResourceManager.h"
#include "MyGUI_LayoutManager.h"
#include "MyGUI_SkinManager.h"
#include "MyGUI_WidgetManager.h"
#include "MyGUI_Widget.h"
#include "MyGUI_CoordConverter.h"
#include "MyGUI_ControllerManager.h"

namespace MyGUI
{

	const std::string XML_TYPE("Layout");

	MYGUI_INSTANCE_IMPLEMENT( LayoutManager )

	void LayoutManager::initialise()
	{
		MYGUI_ASSERT(!mIsInitialise, INSTANCE_TYPE_NAME << " initialised twice");
		MYGUI_LOG(Info, "* Initialise: " << INSTANCE_TYPE_NAME);

		ResourceManager::getInstance().registerLoadXmlDelegate(XML_TYPE) = newDelegate(this, &LayoutManager::_load);
		layoutPrefix = "";
		layoutParent = nullptr;

		MYGUI_LOG(Info, INSTANCE_TYPE_NAME << " successfully initialized");
		mIsInitialise = true;
	}

	void LayoutManager::shutdown()
	{
		if (!mIsInitialise) return;
		MYGUI_LOG(Info, "* Shutdown: " << INSTANCE_TYPE_NAME);

		ResourceManager::getInstance().unregisterLoadXmlDelegate(XML_TYPE);

		MYGUI_LOG(Info, INSTANCE_TYPE_NAME << " successfully shutdown");
		mIsInitialise = false;
	}

	VectorWidgetPtr& LayoutManager::load(const std::string& _file)
	{
		mVectorWidgetPtr.clear();
		ResourceManager::getInstance()._loadImplement(_file, true, XML_TYPE, INSTANCE_TYPE_NAME);
		return mVectorWidgetPtr;
	}

	void LayoutManager::_load(xml::ElementPtr _node, const std::string& _file, Version _version)
	{
#if MYGUI_DEBUG_MODE == 1
		MYGUI_LOG(Info, "load layout '" << _file << "'");
#endif
		parseLayout(mVectorWidgetPtr, _node);
	}

	VectorWidgetPtr& LayoutManager::loadLayout(const std::string& _file, const std::string& _prefix, Widget* _parent)
	{
		static VectorWidgetPtr widgets;
		widgets.clear();

		layoutPrefix = _prefix;
		layoutParent = _parent;
		widgets = load(_file);
		layoutPrefix = "";
		layoutParent = nullptr;
		return widgets;
	}

	void LayoutManager::unloadLayout(VectorWidgetPtr& _widgets)
	{
		WidgetManager::getInstance().destroyWidgets(_widgets);
	}

	void LayoutManager::parseLayout(VectorWidgetPtr& _widgets, xml::ElementPtr _root)
	{
		// берем детей и крутимся
		xml::ElementEnumerator widget = _root->getElementEnumerator();
		while (widget.next("Widget")) parseWidget(_widgets, widget, layoutParent);
	}

	void LayoutManager::parseWidget(VectorWidgetPtr& _widgets, xml::ElementEnumerator& _widget, Widget* _parent)
	{
		// парсим атрибуты виджета
		std::string widgetType, widgetSkin, widgetName, widgetLayer, tmp;

		_widget->findAttribute("type", widgetType);
		_widget->findAttribute("skin", widgetSkin);
		_widget->findAttribute("layer", widgetLayer);

		Align align = Align::Default;
		if (_widget->findAttribute("align", tmp)) align = Align::parse(tmp);

		_widget->findAttribute("name", widgetName);
		if (!widgetName.empty()) widgetName = layoutPrefix + widgetName;

		WidgetStyle style = WidgetStyle::Child;
		if (_widget->findAttribute("style", tmp)) style = WidgetStyle::parse(tmp);
		if (_parent != nullptr && style != WidgetStyle::Popup) widgetLayer.clear();

		IntCoord coord;
		if (_widget->findAttribute("position", tmp)) coord = IntCoord::parse(tmp);
		else if (_widget->findAttribute("position_real", tmp))
		{
			if (_parent == nullptr || style == WidgetStyle::Popup)
				coord = CoordConverter::convertFromRelative(FloatCoord::parse(tmp), Gui::getInstance().getViewSize());
			else
				coord = CoordConverter::convertFromRelative(FloatCoord::parse(tmp), _parent->getSize());
		}

		Widget* wid;
		if (nullptr == _parent)
			wid = Gui::getInstance().createWidgetT(widgetType, widgetSkin, coord, align, widgetLayer, widgetName);
		else
			wid = _parent->createWidgetT(style, widgetType, widgetSkin, coord, align, widgetLayer, widgetName);

		if (layoutParent == _parent) _widgets.push_back(wid);

		// берем детей и крутимся
		xml::ElementEnumerator node = _widget->getElementEnumerator();
		while (node.next())
		{
			if (node->getName() == "Widget")
			{
				parseWidget(_widgets, node, wid);
			}
			else if (node->getName() == "Property")
			{
				wid->setProperty(node->findAttribute("key"), node->findAttribute("value"));
			}
			else if (node->getName() == "UserString")
			{
				wid->setUserString(node->findAttribute("key"), node->findAttribute("value"));
			}
			else if (node->getName() == "Controller")
			{
				const std::string& type = node->findAttribute("type");
				MyGUI::ControllerItem* item = MyGUI::ControllerManager::getInstance().createItem(type);
				if (item)
				{
					xml::ElementEnumerator prop = node->getElementEnumerator();
					while (prop.next("Property"))
					{
						item->setProperty(prop->findAttribute("key"), prop->findAttribute("value"));
					}
					MyGUI::ControllerManager::getInstance().addItem(wid, item);
				}
				else
				{
					//LOG
				}
			}

		}
	}

} // namespace MyGUI
