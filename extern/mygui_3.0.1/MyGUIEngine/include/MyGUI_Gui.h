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
#ifndef __MYGUI_GUI_H__
#define __MYGUI_GUI_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Types.h"
#include "MyGUI_Instance.h"
#include "MyGUI_XmlDocument.h"
#include "MyGUI_IWidgetCreator.h"
#include "MyGUI_IUnlinkWidget.h"
#include "MyGUI_Widget.h"

namespace MyGUI
{

	typedef delegates::CMultiDelegate1<float> FrameEventDelegate;

	class MYGUI_EXPORT Gui : public IWidgetCreator, public IUnlinkWidget
	{
		friend class WidgetManager;
		MYGUI_INSTANCE_HEADER( Gui )

	public:
		/** Initialise GUI and all GUI Managers
			@param
				_core name of core config file for MyGUI (contain main config files with skins, layers, fonts, etc.)
			@param
				_logFileName Log file name
		*/
		void initialise(const std::string& _core = "core.xml", const std::string& _logFileName = MYGUI_LOG_FILENAME);

		/** Shutdown GUI and all GUI Managers*/
		void shutdown();

		// methods for creating widgets
		/** Create widget
			@param _type widget type
			@param _skin widget skin
			@param _coord int coordinates of widget (_left, _top, _width, _height)
			@param _align widget align (possible values can be found in enum Align)
			@param _layer layer where widget will be created (all layers usually defined in core.layer file).
				If your widget will overlap with any other you shoud select _layer with "overlapped" property enabled.
			@param _name if needed (you can use it for finding widget by name later)
		*/
		Widget* createWidgetT(const std::string& _type, const std::string& _skin, const IntCoord& _coord, Align _align, const std::string& _layer, const std::string& _name = "")
		{
			return baseCreateWidget(WidgetStyle::Overlapped, _type, _skin, _coord, _align, _layer, _name);
		}
		/** See Gui::createWidgetT */
		Widget* createWidgetT(const std::string& _type, const std::string& _skin, int _left, int _top, int _width, int _height, Align _align, const std::string& _layer, const std::string& _name = "")
		{
			return createWidgetT(_type, _skin, IntCoord(_left, _top, _width, _height), _align, _layer, _name);
		}
		/** Create widget using coordinates relative to parent. see Gui::createWidgetT */
		Widget* createWidgetRealT(const std::string& _type, const std::string& _skin, const FloatCoord& _coord, Align _align, const std::string& _layer, const std::string& _name = "")
		{
			return createWidgetT(_type, _skin, IntCoord((int)(_coord.left*mViewSize.width), (int)(_coord.top*mViewSize.height), (int)(_coord.width*mViewSize.width), (int)(_coord.height*mViewSize.height)), _align, _layer, _name);
		}
		/** Create widget using coordinates relative to parent. see Gui::createWidgetT */
		Widget* createWidgetRealT(const std::string& _type, const std::string& _skin, float _left, float _top, float _width, float _height, Align _align, const std::string& _layer, const std::string& _name = "")
		{
			return createWidgetT(_type, _skin, IntCoord((int)(_left*mViewSize.width), (int)(_top*mViewSize.height), (int)(_width*mViewSize.width), (int)(_height*mViewSize.height)), _align, _layer, _name);
		}

		// templates for creating widgets by type
		/** Same as Gui::createWidgetT but return T* instead of Widget* */
		template <typename T>
		T* createWidget(const std::string& _skin, const IntCoord& _coord, Align _align, const std::string& _layer, const std::string& _name = "")
		{
			return static_cast<T*>(createWidgetT(T::getClassTypeName(), _skin, _coord, _align, _layer, _name));
		}
		/** Same as Gui::createWidgetT but return T* instead of Widget* */
		template <typename T>
		T* createWidget(const std::string& _skin, int _left, int _top, int _width, int _height, Align _align, const std::string& _layer, const std::string& _name = "")
		{
			return static_cast<T*>(createWidgetT(T::getClassTypeName(), _skin, IntCoord(_left, _top, _width, _height), _align, _layer, _name));
		}
		/** Same as Gui::createWidgetRealT but return T* instead of Widget* */
		template <typename T>
		T* createWidgetReal(const std::string& _skin, const FloatCoord& _coord, Align _align, const std::string& _layer, const std::string& _name = "")
		{
			return static_cast<T*>(createWidgetRealT(T::getClassTypeName(), _skin, _coord, _align, _layer, _name));
		}
		/** Same as Gui::createWidgetRealT but return T* instead of Widget* */
		template <typename T>
		T* createWidgetReal(const std::string& _skin, float _left, float _top, float _width, float _height, Align _align, const std::string& _layer, const std::string& _name = "")
		{
			return static_cast<T*>(createWidgetRealT(T::getClassTypeName(), _skin, _left, _top, _width, _height, _align, _layer, _name));
		}

		/** Get view size of GUI area */
		const IntSize& getViewSize() const { return mViewSize; }

		int getViewWidth() { return mViewSize.width; }
		int getViewHeight() { return mViewSize.height; }

		// mirror of InputManager methods
		/** Inject MouseMove event
			@return true if event has been processed by GUI
		*/
		bool injectMouseMove(int _absx, int _absy, int _absz);
		/** Inject MousePress event
			@return true if event has been processed by GUI
		*/
		bool injectMousePress(int _absx, int _absy, MouseButton _id);
		/** Inject MouseRelease event
			@return true if event has been processed by GUI
		*/
		bool injectMouseRelease(int _absx, int _absy, MouseButton _id);

		/** Inject KeyPress event
			@return true if event has been processed by GUI
		*/
		bool injectKeyPress(KeyCode _key, Char _text = 0);
		/** Inject KeyReleas event
			@return true if event has been processed by GUI
		*/
		bool injectKeyRelease(KeyCode _key);

		/** Destroy any created widget */
		void destroyWidget(Widget* _widget);

		/** Destroy vector of widgets */
		void destroyWidgets(VectorWidgetPtr& _widgets);

		/** Destroy Enumerator of widgets */
		void destroyWidgets(EnumeratorWidgetPtr& _widgets);

		/** Find widget by name
			If widget is not found the exception will be thrown, or if the second parameter is false the nullptr pointer will be returned
		*/
		Widget* findWidgetT(const std::string& _name, bool _throw = true);

		/** Find widget by name and prefix
			If widget is not found the exception will be thrown, or if the second parameter is false the nullptr pointer will be returned
		*/
		Widget* findWidgetT(const std::string& _name, const std::string& _prefix, bool _throw = true)
		{
			return findWidgetT(_prefix + _name, _throw);
		}

		// mirror WidgetManager
		/** Find widget by name and cast it to T type.
			If widget not found or T and found widget have different types cause exception, or if the second parameter is false the nullptr pointer will be returned
		*/
		template <typename T>
		T* findWidget(const std::string& _name, bool _throw = true)
		{
			Widget* widget = findWidgetT(_name, _throw);
			if (nullptr == widget) return nullptr;
			return widget->castType<T>(_throw);
		}

		/** Find widget by name and prefix and cast it to T type
			If widget not found or T and found widget have different types cause exception, or if the second parameter is false the nullptr pointer will be returned
		*/
		template <typename T>
		T* findWidget(const std::string& _name, const std::string& _prefix, bool _throw = true)
		{
			return findWidget<T>(_prefix + _name, _throw);
		}


		/** Show or hide mouse pointer */
		void setVisiblePointer(bool _visible);
		/** Is mouse pointer visible */
		bool isVisiblePointer();


		// mirror ResourceManager
		/** Load config with any info (file can have different data such other config files that will be loaded, skins, layers, pointers, etc) */
		bool load(const std::string& _file);

		void resizeWindow(const IntSize& _size);

		/** Destroy child widget or throw exception if this child widget not found */
		void destroyChildWidget(Widget* _widget) { _destroyChildWidget(_widget); }

		/** Destroy all child widgets */
		void destroyAllChildWidget() { _destroyAllChildWidget(); }

		/** Get name of Gui ResourceGroup*/
		//static const std::string& getResourceGroup();

		/** Get root widgets Enumerator */
		EnumeratorWidgetPtr getEnumerator() { return EnumeratorWidgetPtr(mWidgetChild); }

	/*internal:*/

		/** Inject frame entered event (called be renderer, do not call it manually).
			This function is called every frame by renderer.
		*/
		void _injectFrameEntered(float _time);

	/*event:*/
		/** Multidelegate for GUI per frame call.\n
			signature : void method(float _time)\n
			@param _time Time elapsed since last frame
		*/
		FrameEventDelegate eventFrameStart;

	/*obsolete:*/
#ifndef MYGUI_DONT_USE_OBSOLETE

		MYGUI_OBSOLETE("use : void Gui::destroyWidgets(VectorWidgetPtr &_widgets)")
		void destroyWidgetsVector(VectorWidgetPtr& _widgets) { destroyWidgets(_widgets); }

		MYGUI_OBSOLETE("use : void Gui::setVisiblePointer(bool _value)")
		void hidePointer() { setVisiblePointer(false); }
		MYGUI_OBSOLETE("use : void Gui::setVisiblePointer(bool _value)")
		void showPointer() { setVisiblePointer(true); }
		MYGUI_OBSOLETE("use : bool Gui::isVisiblePointer()")
		bool isShowPointer() { return isVisiblePointer(); }
		MYGUI_OBSOLETE("called be renderer, do not call it manually")
		void injectFrameEntered(float _time) { }

#endif // MYGUI_DONT_USE_OBSOLETE

	private:
		// создает виджет
		virtual Widget* baseCreateWidget(WidgetStyle _style, const std::string& _type, const std::string& _skin, const IntCoord& _coord, Align _align, const std::string& _layer, const std::string& _name);

		// удяляет неудачника
		void _destroyChildWidget(Widget* _widget);

		// удаляет всех детей
		void _destroyAllChildWidget();

		virtual void _unlinkWidget(Widget* _widget);

		// добавляет в список виджет
		virtual void _linkChildWidget(Widget* _widget);

		// удаляет из списка
		virtual void _unlinkChildWidget(Widget* _widget);


	private:
		// вектор всех детей виджетов
		VectorWidgetPtr mWidgetChild;

		// размеры экрана
		IntSize mViewSize;

		// синглтоны гуя
		InputManager * mInputManager;
		SubWidgetManager * mSubWidgetManager;
		LayerManager* mLayerManager;
		SkinManager* mSkinManager;
		WidgetManager* mWidgetManager;
		FontManager* mFontManager;
		ControllerManager* mControllerManager;
		PointerManager* mPointerManager;
		ClipboardManager* mClipboardManager;
		LayoutManager* mLayoutManager;
		DynLibManager* mDynLibManager;
		PluginManager* mPluginManager;
		LanguageManager* mLanguageManager;
		ResourceManager* mResourceManager;
		FactoryManager* mFactoryManager;

	};

} // namespace MyGUI

#endif // __MYGUI_GUI_H__
