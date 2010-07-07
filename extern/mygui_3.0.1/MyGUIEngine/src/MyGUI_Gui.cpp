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
#include "MyGUI_Gui.h"
#include "MyGUI_Widget.h"

#include "MyGUI_InputManager.h"
#include "MyGUI_SubWidgetManager.h"
#include "MyGUI_LogManager.h"
#include "MyGUI_SkinManager.h"
#include "MyGUI_WidgetManager.h"
#include "MyGUI_LayerManager.h"
#include "MyGUI_FontManager.h"
#include "MyGUI_ControllerManager.h"
#include "MyGUI_PointerManager.h"
#include "MyGUI_ClipboardManager.h"
#include "MyGUI_LayoutManager.h"
#include "MyGUI_PluginManager.h"
#include "MyGUI_DynLibManager.h"
#include "MyGUI_LanguageManager.h"
#include "MyGUI_ResourceManager.h"
#include "MyGUI_RenderManager.h"
#include "MyGUI_FactoryManager.h"

namespace MyGUI
{

	const std::string INSTANCE_TYPE_NAME("Gui");

	Gui* Gui::msInstance = nullptr;

	Gui* Gui::getInstancePtr()
	{
		return msInstance;
	}

	Gui& Gui::getInstance()
	{
		MYGUI_ASSERT(0 != msInstance, "instance " << INSTANCE_TYPE_NAME << " was not created");
		return (*msInstance);
	}

	Gui::Gui() :
		mIsInitialise(false)
	{
		MYGUI_ASSERT(0 == msInstance, "instance " << INSTANCE_TYPE_NAME << " is exsist");
		msInstance = this;
	}

	Gui::~Gui()
	{
		msInstance = nullptr;
	}

	void Gui::initialise(const std::string& _core, const std::string& _logFileName)
	{
		// самый первый лог
		LogManager::registerSection(MYGUI_LOG_SECTION, _logFileName);

		MYGUI_ASSERT(!mIsInitialise, INSTANCE_TYPE_NAME << " initialised twice");

		MYGUI_LOG(Info, "* Initialise: " << INSTANCE_TYPE_NAME);
		MYGUI_LOG(Info, "* MyGUI version "
			<< MYGUI_VERSION_MAJOR << "."
			<< MYGUI_VERSION_MINOR << "."
			<< MYGUI_VERSION_PATCH);

		// создаем и инициализируем синглтоны
		mResourceManager = new ResourceManager();
		mLayerManager = new LayerManager();
		mWidgetManager = new WidgetManager();
		mInputManager = new InputManager();
		mSubWidgetManager = new SubWidgetManager();
		mSkinManager = new SkinManager();
		mFontManager = new FontManager();
		mControllerManager = new ControllerManager();
		mPointerManager = new PointerManager();
		mClipboardManager = new ClipboardManager();
		mLayoutManager = new LayoutManager();
		mDynLibManager = new DynLibManager();
		mPluginManager = new PluginManager();
		mLanguageManager = new LanguageManager();
		mFactoryManager = new FactoryManager();

		mResourceManager->initialise();
		mLayerManager->initialise();
		mWidgetManager->initialise();
		mInputManager->initialise();
		mSubWidgetManager->initialise();
		mSkinManager->initialise();
		mFontManager->initialise();
		mControllerManager->initialise();
		mPointerManager->initialise();
		mClipboardManager->initialise();
		mLayoutManager->initialise();
		mDynLibManager->initialise();
		mPluginManager->initialise();
		mLanguageManager->initialise();
		mFactoryManager->initialise();

		WidgetManager::getInstance().registerUnlinker(this);

		// загружаем дефолтные настройки если надо
		if ( _core.empty() == false ) mResourceManager->load(_core);

		mViewSize = RenderManager::getInstance().getViewSize();
		resizeWindow(mViewSize);

		MYGUI_LOG(Info, INSTANCE_TYPE_NAME << " successfully initialized");
		mIsInitialise = true;
	}

	void Gui::shutdown()
	{
		if (!mIsInitialise) return;
		MYGUI_LOG(Info, "* Shutdown: " << INSTANCE_TYPE_NAME);

		_destroyAllChildWidget();

		// деинициализируем и удаляем синглтоны
		mPointerManager->shutdown();
		mInputManager->shutdown();
		mSkinManager->shutdown();
		mSubWidgetManager->shutdown();
		mLayerManager->shutdown();
		mFontManager->shutdown();
		mControllerManager->shutdown();
		mClipboardManager->shutdown();
		mLayoutManager->shutdown();
		mPluginManager->shutdown();
		mDynLibManager->shutdown();
		mLanguageManager->shutdown();
		mResourceManager->shutdown();
		mFactoryManager->shutdown();

		WidgetManager::getInstance().unregisterUnlinker(this);
		mWidgetManager->shutdown();

		delete mPointerManager;
		delete mWidgetManager;
		delete mInputManager;
		delete mSkinManager;
		delete mSubWidgetManager;
		delete mLayerManager;
		delete mFontManager;
		delete mControllerManager;
		delete mClipboardManager;
		delete mLayoutManager;
		delete mDynLibManager;
		delete mPluginManager;
		delete mLanguageManager;
		delete mResourceManager;
		delete mFactoryManager;

		MYGUI_LOG(Info, INSTANCE_TYPE_NAME << " successfully shutdown");

		// last gui log
		LogManager::unregisterSection(MYGUI_LOG_SECTION);

		mIsInitialise = false;
	}

	bool Gui::injectMouseMove( int _absx, int _absy, int _absz) { return mInputManager->injectMouseMove(_absx, _absy, _absz); }
	bool Gui::injectMousePress( int _absx, int _absy, MouseButton _id ) { return mInputManager->injectMousePress(_absx, _absy, _id); }
	bool Gui::injectMouseRelease( int _absx, int _absy, MouseButton _id ) { return mInputManager->injectMouseRelease(_absx, _absy, _id); }

	bool Gui::injectKeyPress(KeyCode _key, Char _text) { return mInputManager->injectKeyPress(_key, _text); }
	bool Gui::injectKeyRelease(KeyCode _key) { return mInputManager->injectKeyRelease(_key); }


	Widget* Gui::baseCreateWidget(WidgetStyle _style, const std::string& _type, const std::string& _skin, const IntCoord& _coord, Align _align, const std::string& _layer, const std::string& _name)
	{
		Widget* widget = WidgetManager::getInstance().createWidget(_style, _type, _skin, _coord, _align, nullptr, nullptr, this, _name);
		mWidgetChild.push_back(widget);
		// присоединяем виджет с уровню
		if (!_layer.empty()) LayerManager::getInstance().attachToLayerNode(_layer, widget);
		return widget;
	}

	Widget* Gui::findWidgetT(const std::string& _name, bool _throw)
	{
		for (VectorWidgetPtr::iterator iter = mWidgetChild.begin(); iter!=mWidgetChild.end(); ++iter)
		{
			Widget* widget = (*iter)->findWidget(_name);
			if (widget != nullptr) return widget;
		}
		MYGUI_ASSERT(!_throw, "Widget '" << _name << "' not found");
		return nullptr;
	}

	// удяляет неудачника
	void Gui::_destroyChildWidget(Widget* _widget)
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
			mWidgetManager->unlinkFromUnlinkers(_widget);

			// непосредственное удаление
			_deleteWidget(widget);
		}
		else MYGUI_EXCEPT("Widget '" << _widget->getName() << "' not found");
	}

	// удаляет всех детей
	void Gui::_destroyAllChildWidget()
	{
		while (!mWidgetChild.empty())
		{
			// сразу себя отписывем, иначе вложенной удаление убивает все
			Widget* widget = mWidgetChild.back();
			mWidgetChild.pop_back();

			//widget->detachWidget();

			// отписываем от всех
			mWidgetManager->unlinkFromUnlinkers(widget);

			// и сами удалим, так как его больше в списке нет
			_deleteWidget(widget);
		}
	}

	bool Gui::load(const std::string& _file)
	{
		return mResourceManager->load(_file);
	}

	void Gui::destroyWidget(Widget* _widget)
	{
		mWidgetManager->destroyWidget(_widget);
	}

	void Gui::destroyWidgets(VectorWidgetPtr& _widgets)
	{
		mWidgetManager->destroyWidgets(_widgets);
	}

	void Gui::destroyWidgets(EnumeratorWidgetPtr& _widgets)
	{
		mWidgetManager->destroyWidgets(_widgets);
	}

	void Gui::setVisiblePointer(bool _value)
	{
		mPointerManager->setVisible(_value);
	}

	bool Gui::isVisiblePointer()
	{
		return mPointerManager->isVisible();
	}

	void Gui::_injectFrameEntered(float _time)
	{
		eventFrameStart(_time);
	}

	void Gui::_unlinkWidget(Widget* _widget)
	{
		eventFrameStart.clear(_widget);
	}

	void Gui::_linkChildWidget(Widget* _widget)
	{
		VectorWidgetPtr::iterator iter = std::find(mWidgetChild.begin(), mWidgetChild.end(), _widget);
		MYGUI_ASSERT(iter == mWidgetChild.end(), "widget already exist");
		mWidgetChild.push_back(_widget);
	}

	void Gui::_unlinkChildWidget(Widget* _widget)
	{
		VectorWidgetPtr::iterator iter = std::remove(mWidgetChild.begin(), mWidgetChild.end(), _widget);
		MYGUI_ASSERT(iter != mWidgetChild.end(), "widget not found");
		mWidgetChild.erase(iter);
	}

	void Gui::resizeWindow(const IntSize& _size)
	{
		IntSize oldViewSize = mViewSize;
		mViewSize = _size;

		// выравниваем рутовые окна
		for (VectorWidgetPtr::iterator iter = mWidgetChild.begin(); iter!=mWidgetChild.end(); ++iter)
		{
			((ICroppedRectangle*)(*iter))->_setAlign(oldViewSize, true);
		}
	}

} // namespace MyGUI
