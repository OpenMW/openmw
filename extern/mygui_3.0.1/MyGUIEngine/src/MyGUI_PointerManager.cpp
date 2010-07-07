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
#include "MyGUI_LayerManager.h"
#include "MyGUI_PointerManager.h"
#include "MyGUI_CoordConverter.h"
#include "MyGUI_WidgetManager.h"
#include "MyGUI_XmlDocument.h"
#include "MyGUI_Widget.h"
#include "MyGUI_FactoryManager.h"
#include "MyGUI_InputManager.h"
#include "MyGUI_Gui.h"

#include "MyGUI_ResourceManualPointer.h"
#include "MyGUI_ResourceImageSetPointer.h"

namespace MyGUI
{

	const std::string XML_TYPE("Pointer");
	const std::string XML_TYPE_RESOURCE("Resource");
	const std::string XML_TYPE_PROPERTY("Property");
	const std::string RESOURCE_DEFAULT_NAME("Default");

	MYGUI_INSTANCE_IMPLEMENT( PointerManager )

	void PointerManager::initialise()
	{
		MYGUI_ASSERT(!mIsInitialise, INSTANCE_TYPE_NAME << " initialised twice");
		MYGUI_LOG(Info, "* Initialise: " << INSTANCE_TYPE_NAME);

		Gui::getInstance().eventFrameStart += newDelegate(this, &PointerManager::notifyFrameStart);
		InputManager::getInstance().eventChangeMouseFocus += newDelegate(this, &PointerManager::notifyChangeMouseFocus);
		WidgetManager::getInstance().registerUnlinker(this);

		ResourceManager::getInstance().registerLoadXmlDelegate(XML_TYPE) = newDelegate(this, &PointerManager::_load);

		FactoryManager::getInstance().registerFactory<ResourceManualPointer>(XML_TYPE_RESOURCE);
		FactoryManager::getInstance().registerFactory<ResourceImageSetPointer>(XML_TYPE_RESOURCE);

		mPointer = nullptr;
		mMousePointer = nullptr;
		mWidgetOwner = nullptr;
		mVisible = true;

		mSkinName = "StaticImage";

		MYGUI_LOG(Info, INSTANCE_TYPE_NAME << " successfully initialized");
		mIsInitialise = true;
	}

	void PointerManager::shutdown()
	{
		if (!mIsInitialise) return;
		MYGUI_LOG(Info, "* Shutdown: " << INSTANCE_TYPE_NAME);

		InputManager::getInstance().eventChangeMouseFocus -= newDelegate(this, &PointerManager::notifyChangeMouseFocus);
		Gui::getInstance().eventFrameStart -= newDelegate(this, &PointerManager::notifyFrameStart);

		FactoryManager::getInstance().unregisterFactory<ResourceManualPointer>(XML_TYPE_RESOURCE);
		FactoryManager::getInstance().unregisterFactory<ResourceImageSetPointer>(XML_TYPE_RESOURCE);

		// удаляем все виджеты
		_destroyAllChildWidget();

		mWidgetOwner = nullptr;

		WidgetManager::getInstance().unregisterUnlinker(this);
		ResourceManager::getInstance().unregisterLoadXmlDelegate(XML_TYPE);

		MYGUI_LOG(Info, INSTANCE_TYPE_NAME << " successfully shutdown");
		mIsInitialise = false;
	}

	bool PointerManager::load(const std::string& _file)
	{
		return ResourceManager::getInstance()._loadImplement(_file, true, XML_TYPE, INSTANCE_TYPE_NAME);
	}

	void PointerManager::_load(xml::ElementPtr _node, const std::string& _file, Version _version)
	{
		std::string pointer;
		std::string layer;

		xml::ElementEnumerator node = _node->getElementEnumerator();
		while (node.next())
		{
			if (node->getName() == XML_TYPE)
			{
				layer = node->findAttribute("layer");
				pointer = node->findAttribute("default");

				// сохраняем
				std::string shared_text = node->findAttribute("texture");

				// берем детей и крутимся, основной цикл
				xml::ElementEnumerator info = node->getElementEnumerator();
				while (info.next("Info"))
				{
					std::string name = info->findAttribute("name");
					if (name.empty()) continue;

					std::string texture = info->findAttribute("texture");

					std::string type = (shared_text.empty() && texture.empty()) ? "ResourceImageSetPointer" : "ResourceManualPointer";

					xml::Document doc;
					xml::ElementPtr root = doc.createRoot("MyGUI");
					xml::ElementPtr newnode = root->createChild("Resource");
					newnode->addAttribute("type", type);
					newnode->addAttribute("name", name);

					std::string tmp;
					if (info->findAttribute("point", tmp))
					{
						xml::ElementPtr prop = newnode->createChild("Property");
						prop->addAttribute("key", "Point");
						prop->addAttribute("value", tmp);
					}

					if (info->findAttribute("size", tmp))
					{
						xml::ElementPtr prop = newnode->createChild("Property");
						prop->addAttribute("key", "Size");
						prop->addAttribute("value", tmp);
					}

					if (info->findAttribute("resource", tmp))
					{
						xml::ElementPtr prop = newnode->createChild("Property");
						prop->addAttribute("key", "Resource");
						prop->addAttribute("value", tmp);
					}

					if (info->findAttribute("offset", tmp))
					{
						xml::ElementPtr prop = newnode->createChild("Property");
						prop->addAttribute("key", "Coord");
						prop->addAttribute("value", tmp);
					}

					if (!shared_text.empty() || !texture.empty())
					{
						xml::ElementPtr prop = newnode->createChild("Property");
						prop->addAttribute("key", "Texture");
						prop->addAttribute("value",  shared_text.empty() ? texture : shared_text);
					}

					ResourceManager::getInstance()._load(root, _file, _version);
				}

			}
			else if (node->getName() == XML_TYPE_PROPERTY)
			{
				const std::string& key = node->findAttribute("key");
				const std::string& value = node->findAttribute("value");
				if (key == "Default")
					setDeafultPointer(value);
				else if (key == "Layer")
					setLayerName(value);
				else if (key == "Skin")
					mSkinName = value;
			}
		}

		if (!layer.empty())
			setLayerName(layer);

		if (!pointer.empty())
			setDeafultPointer(pointer);

	}

	void PointerManager::notifyFrameStart(float _time)
	{
		mPoint = InputManager::getInstance().getMousePosition();
		if (nullptr != mMousePointer && mPointer != nullptr)
			mPointer->setPosition(mMousePointer, mPoint);
	}

	void PointerManager::setVisible(bool _visible)
	{
		if (nullptr != mMousePointer) mMousePointer->setVisible(_visible);
		mVisible = _visible;
	}

	void PointerManager::setPointer(const std::string& _name, Widget* _owner)
	{
		if (nullptr == mMousePointer)
			return;

		IResource* result = getByName(_name);
		if (result == nullptr)
		{
			mPointer = nullptr;
			mMousePointer->setVisible(false);
			return;
		}

		mMousePointer->setVisible(mVisible);
		mPointer = result->castType<IPointer>();
		mPointer->setImage(mMousePointer);
		mPointer->setPosition(mMousePointer, mPoint);

		mWidgetOwner = _owner;
	}

	void PointerManager::_unlinkWidget(Widget* _widget)
	{
		if (_widget == mWidgetOwner) setPointer(mDefaultName, nullptr);
		else if (_widget == mMousePointer) mMousePointer = nullptr;
	}

	void PointerManager::resetToDefaultPointer()
	{
		setPointer(mDefaultName, nullptr);
	}

	// создает виджет
	Widget* PointerManager::baseCreateWidget(WidgetStyle _style, const std::string& _type, const std::string& _skin, const IntCoord& _coord, Align _align, const std::string& _layer, const std::string& _name)
	{
		Widget* widget = WidgetManager::getInstance().createWidget(_style, _type, _skin, _coord, _align, nullptr, nullptr, this, _name);
		mWidgetChild.push_back(widget);
		// присоединяем виджет с уровню
		if (!_layer.empty())
			LayerManager::getInstance().attachToLayerNode(_layer, widget);
		return widget;
	}

	// удяляет неудачника
	void PointerManager::_destroyChildWidget(Widget* _widget)
	{
		MYGUI_ASSERT(nullptr != _widget, "invalid widget pointer");

		VectorWidgetPtr::iterator iter = std::find(mWidgetChild.begin(), mWidgetChild.end(), _widget);
		if (iter != mWidgetChild.end())
		{
			// сохраняем указатель
			MyGUI::Widget* widget = *iter;

			// удаляем из списка
			*iter = mWidgetChild.back();
			mWidgetChild.pop_back();

			// отписываем от всех
			WidgetManager::getInstance().unlinkFromUnlinkers(_widget);

			// непосредственное удаление
			_deleteWidget(widget);
		}
		else
		{
			MYGUI_EXCEPT("Widget '" << _widget->getName() << "' not found");
		}
	}

	// удаляет всех детей
	void PointerManager::_destroyAllChildWidget()
	{
		WidgetManager& manager = WidgetManager::getInstance();
		while (!mWidgetChild.empty())
		{
			// сразу себя отписывем, иначе вложенной удаление убивает все
			Widget* widget = mWidgetChild.back();
			mWidgetChild.pop_back();

			// отписываем от всех
			manager.unlinkFromUnlinkers(widget);

			// и сами удалим, так как его больше в списке нет
			_deleteWidget(widget);
		}
	}

	void PointerManager::setDeafultPointer(const std::string& _value)
	{
		Update();

		mDefaultName = _value;
		setPointer(mDefaultName, nullptr);
	}

	void PointerManager::setLayerName(const std::string& _value)
	{
		Update();

		mLayerName = _value;
		if (LayerManager::getInstance().isExist(_value))
			LayerManager::getInstance().attachToLayerNode(mLayerName, mMousePointer);
	}

	void PointerManager::Update()
	{
		if (mMousePointer == nullptr)
			mMousePointer = static_cast<StaticImage*>(baseCreateWidget(WidgetStyle::Overlapped, StaticImage::getClassTypeName(), mSkinName, IntCoord(), Align::Default, "", ""));
	}

	IPointer* PointerManager::getByName(const std::string& _name) const
	{
		IResource* result = nullptr;
		if (!_name.empty() && _name != RESOURCE_DEFAULT_NAME)
			result = ResourceManager::getInstance().getByName(_name, false);

		if (result == nullptr)
			result = ResourceManager::getInstance().getByName(mDefaultName, false);

		return result ? result->castType<IPointer>(false) : nullptr;
	}

	void PointerManager::notifyChangeMouseFocus(Widget* _widget)
	{
		std::string pointer = _widget == nullptr ? "" : _widget->getPointer();
		if (pointer != mCurrentMousePointer)
		{
			mCurrentMousePointer = pointer;
			if (mCurrentMousePointer.empty())
			{
				resetToDefaultPointer();
				eventChangeMousePointer(mDefaultName);
			}
			else
			{
				setPointer(mCurrentMousePointer, _widget);
				eventChangeMousePointer(mCurrentMousePointer);
			}
		}
	}

	void PointerManager::setPointer(const std::string& _name)
	{
		setPointer(_name, nullptr);
	}

} // namespace MyGUI
