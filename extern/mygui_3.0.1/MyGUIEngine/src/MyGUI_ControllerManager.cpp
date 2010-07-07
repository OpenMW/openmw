/*!
	@file
	@author		Albert Semenov
	@date		01/2008
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
#include "MyGUI_Gui.h"
#include "MyGUI_ControllerManager.h"
#include "MyGUI_WidgetManager.h"
#include "MyGUI_FactoryManager.h"

#include "MyGUI_ControllerEdgeHide.h"
#include "MyGUI_ControllerFadeAlpha.h"
#include "MyGUI_ControllerPosition.h"

namespace MyGUI
{

	MYGUI_INSTANCE_IMPLEMENT( ControllerManager )

	void ControllerManager::initialise()
	{
		MYGUI_ASSERT(!mIsInitialise, INSTANCE_TYPE_NAME << " initialised twice");
		MYGUI_LOG(Info, "* Initialise: " << INSTANCE_TYPE_NAME);

		WidgetManager::getInstance().registerUnlinker(this);

		const std::string factory_type = "Controller";

		FactoryManager::getInstance().registerFactory<ControllerEdgeHide>(factory_type);
		FactoryManager::getInstance().registerFactory<ControllerFadeAlpha>(factory_type);
		FactoryManager::getInstance().registerFactory<ControllerPosition>(factory_type);

		MYGUI_LOG(Info, INSTANCE_TYPE_NAME << " successfully initialized");
		mIsInitialise = true;
	}

	void ControllerManager::shutdown()
	{
		if (!mIsInitialise) return;
		MYGUI_LOG(Info, "* Shutdown: " << INSTANCE_TYPE_NAME);

		const std::string factory_type = "Controller";

		FactoryManager::getInstance().unregisterFactory<ControllerEdgeHide>(factory_type);
		FactoryManager::getInstance().unregisterFactory<ControllerFadeAlpha>(factory_type);
		FactoryManager::getInstance().unregisterFactory<ControllerPosition>(factory_type);

		WidgetManager::getInstance().unregisterUnlinker(this);
		clear();

		MYGUI_LOG(Info, INSTANCE_TYPE_NAME << " successfully shutdown");
		mIsInitialise = false;
	}

	void ControllerManager::clear()
	{
		for (ListControllerItem::iterator iter=mListItem.begin(); iter!=mListItem.end(); ++iter)
		{
			delete (*iter).second;
		}
		mListItem.clear();
	}

	ControllerItem* ControllerManager::createItem(const std::string& _type)
	{
		IObject* object = FactoryManager::getInstance().createObject("Controller", _type);
		return object == nullptr ? nullptr : object->castType<ControllerItem>();
	}

	void ControllerManager::addItem(Widget* _widget, ControllerItem * _item)
	{
		// если виджет первый, то подписываемся на кадры
		if (0 == mListItem.size()) Gui::getInstance().eventFrameStart += newDelegate(this, &ControllerManager::frameEntered);

		// подготавливаем
		_item->prepareItem(_widget);

		for (ListControllerItem::iterator iter=mListItem.begin(); iter!=mListItem.end(); ++iter)
		{
			// такой уже в списке есть
			if ((*iter).first == _widget)
			{
				if ((*iter).second->getTypeName() == _item->getTypeName())
				{
					delete (*iter).second;
					(*iter).second = _item;
					return;
				}
			}
		}

		// вставляем в самый конец
		mListItem.push_back(PairControllerItem(_widget, _item));
	}

	void ControllerManager::removeItem(Widget* _widget)
	{
		// не удаляем из списка, а обнуляем, в цикле он будет удален
		for (ListControllerItem::iterator iter=mListItem.begin(); iter!=mListItem.end(); ++iter)
		{
			if ((*iter).first == _widget) (*iter).first = nullptr;
		}
	}

	void ControllerManager::_unlinkWidget(Widget* _widget)
	{
		removeItem(_widget);
	}

	void ControllerManager::frameEntered(float _time)
	{
		for (ListControllerItem::iterator iter=mListItem.begin(); iter!=mListItem.end(); /*added in body*/)
		{
			if (nullptr == (*iter).first)
			{
				delete (*iter).second;
				// удаляем из списка, итератор не увеличиваем и на новый круг
				iter = mListItem.erase(iter);
				continue;
			}

			if ((*iter).second->addTime((*iter).first, _time))
			{
				++iter;
				continue;
			}

			// на следующей итерации виджет вылетит из списка
			(*iter).first = nullptr;
		}

		if (0 == mListItem.size()) Gui::getInstance().eventFrameStart -= newDelegate(this, &ControllerManager::frameEntered);
	}

} // namespace MyGUI
