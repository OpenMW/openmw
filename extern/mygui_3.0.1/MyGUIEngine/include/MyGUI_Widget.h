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
#ifndef __MYGUI_WIDGET_H__
#define __MYGUI_WIDGET_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Any.h"
#include "MyGUI_ICroppedRectangle.h"
#include "MyGUI_ISubWidgetRect.h"
#include "MyGUI_ISubWidgetText.h"
#include "MyGUI_LayerItem.h"
#include "MyGUI_WidgetUserData.h"
#include "MyGUI_WidgetEvent.h"
#include "MyGUI_IWidgetCreator.h"
#include "MyGUI_ResourceSkin.h"
#include "MyGUI_IObject.h"

namespace MyGUI
{

	class MYGUI_EXPORT Widget :
		public IObject,
		public ICroppedRectangle,
		public LayerItem,
		public UserData,
		public WidgetEvent,
		public IWidgetCreator,
		public delegates::IDelegateUnlink
	{
		// для вызова закрытых деструкторов
		friend class IWidgetCreator;

		MYGUI_RTTI_BASE( Widget )

	public:
		Widget();

		/** Create child widget
			@param _type widget type
			@param _skin widget skin
			@param _coord int coordinates of widget (_left, _top, _width, _height)
			@param _align widget align (possible values can be found in enum Align)
			@param _name if needed (you can use it for finding widget by name later)
		*/
		Widget* createWidgetT(const std::string& _type, const std::string& _skin, const IntCoord& _coord, Align _align, const std::string& _name = "");

		/** See Widget::createWidgetT(const std::string& _type, const std::string& _skin, const IntCoord& _coord, Align _align, const std::string& _name = "") */
		Widget* createWidgetT(const std::string& _type, const std::string& _skin, int _left, int _top, int _width, int _height, Align _align, const std::string& _name = "");

		/** Create widget using coordinates relative to parent. see Widget::createWidgetT(const std::string& _type, const std::string& _skin, const IntCoord& _coord, Align _align, const std::string& _name = "") */
		Widget* createWidgetRealT(const std::string& _type, const std::string& _skin, const FloatCoord& _coord, Align _align, const std::string& _name = "");

		/** Create widget using coordinates relative to parent. see Widget::createWidgetT(const std::string& _type, const std::string& _skin, const IntCoord& _coord, Align _align, const std::string& _name = "") */
		Widget* createWidgetRealT(const std::string& _type, const std::string& _skin, float _left, float _top, float _width, float _height, Align _align, const std::string& _name = "");

		// templates for creating widgets by type
		/** Same as Widget::createWidgetT but return T pointer instead of Widget* */
		template <typename T>
		T* createWidget(const std::string& _skin, const IntCoord& _coord, Align _align, const std::string& _name = "")
		{
			return static_cast<T*>(createWidgetT(T::getClassTypeName(), _skin, _coord, _align, _name));
		}

		/** Same as Widget::createWidgetT but return T pointer instead of Widget* */
		template <typename T>
		T* createWidget(const std::string& _skin, int _left, int _top, int _width, int _height, Align _align, const std::string& _name = "")
		{
			return static_cast<T*>(createWidgetT(T::getClassTypeName(), _skin, IntCoord(_left, _top, _width, _height), _align, _name));
		}

		/** Same as Widget::createWidgetRealT but return T* instead of Widget* */
		template <typename T>
		T* createWidgetReal(const std::string& _skin, const FloatCoord& _coord, Align _align, const std::string& _name = "")
		{
			return static_cast<T*>(createWidgetRealT(T::getClassTypeName(), _skin, _coord, _align, _name));
		}

		/** Same as Widget::createWidgetRealT but return T* instead of Widget* */
		template <typename T>
		T* createWidgetReal(const std::string& _skin, float _left, float _top, float _width, float _height, Align _align, const std::string& _name = "")
		{
			return static_cast<T*>(createWidgetRealT(T::getClassTypeName(), _skin, _left, _top, _width, _height, _align, _name));
		}

		/** Create child widget
			@param _style Child, Popup or Overlapped widget style
			@param _type widget type
			@param _skin widget skin
			@param _coord int coordinates of widget (_left, _top, _width, _height)
			@param _align widget align (possible values can be found in enum Align)
			@param _name if needed (you can use it for finding widget by name later)
		*/
		Widget* createWidgetT(WidgetStyle _style, const std::string& _type, const std::string& _skin, const IntCoord& _coord, Align _align, const std::string& _layer = "", const std::string& _name = "");

		/** Same as Widget::createWidgetT but return T* instead of Widget* */
		template <typename T>
		T* createWidget(WidgetStyle _style, const std::string& _skin, const IntCoord& _coord, Align _align, const std::string& _layer = "", const std::string& _name = "")
		{
			return static_cast<T*>(createWidgetT(_style, T::getClassTypeName(), _skin, _coord, _align, _layer, _name));
		}

		//! Get name of widget
		const std::string& getName() const { return mName; }


		/** Set widget position (position of left top corner) */
		virtual void setPosition(const IntPoint& _value);
		/** Set widget size */
		virtual void setSize(const IntSize& _value);
		/** Set widget position and size */
		virtual void setCoord(const IntCoord& _value);

		/** See Widget::setPosition(const IntPoint& _pos) */
		void setPosition(int _left, int _top) { setPosition(IntPoint(_left, _top)); }
		/** See Widget::setSize(const IntSize& _size) */
		void setSize(int _width, int _height) { setSize(IntSize(_width, _height)); }
		/** See Widget::setCoord(const IntCoord& _coord) */
		void setCoord(int _left, int _top, int _width, int _height) { setCoord(IntCoord(_left, _top, _width, _height)); }

		/** Set widget position (position of left top corner)*/
		void setRealPosition(const FloatPoint& _value);
		/** Set widget size */
		void setRealSize(const FloatSize& _value);
		/** Set widget position and size*/
		void setRealCoord(const FloatCoord& _value);

		/** See Widget::setRealPosition(const FloatPoint& _point) */
		void setRealPosition(float _left, float _top) { setRealPosition(FloatPoint(_left, _top)); }
		/** See Widget::setRealSize(const FloatSize& _size) */
		void setRealSize(float _width, float _height) { setRealSize(FloatSize(_width, _height)); }
		/** See Widget::setRealPosition(const FloatCoord& _coord) */
		void setRealCoord(float _left, float _top, float _width, float _height) { setRealCoord(FloatCoord(_left, _top, _width, _height)); }

		/** Hide or show widget */
		virtual void setVisible(bool _value);

		/** Set align */
		virtual void setAlign(Align _value);

		/** Set widget caption */
		virtual void setCaption(const UString& _value);
		/** Get widget caption */
		virtual const UString& getCaption();

		/** Set widget opacity */
		void setAlpha(float _value);
		/** Get widget opacity */
		float getAlpha() { return mAlpha; }

		/** Enable or disable inherits alpha mode */
		void setInheritsAlpha(bool _value);
		/** Get inherits alpha mode flag */
		bool isInheritsAlpha() { return mInheritsAlpha; }

		/** Set widget's state */
		bool setState(const std::string& _value);

		void setColour(const Colour& _value);

		// являемся ли мы рутовым виджетом
		/** Is this widget is root widget (root == without parents) */
		bool isRootWidget() { return nullptr == mCroppedParent; }

		/** Get parent widget or nullptr if no parent */
		Widget* getParent() { return mParent; }

		/** Get child widgets Enumerator */
		EnumeratorWidgetPtr getEnumerator();

		/** Get child count */
		size_t getChildCount();

		/** Get child by index (index from 0 to child_count - 1) */
		Widget* getChildAt(size_t _index);

		/** Find widget by name (search recursively through all childs starting from this widget) */
		Widget* findWidget(const std::string& _name);

		/** Set need key focus flag */
		void setNeedKeyFocus(bool _value) { mNeedKeyFocus = _value; }
		/** Is need key focus
			If disable this widget won't be reacting on keyboard at all.\n
			Enabled (true) by default.
		*/
		bool isNeedKeyFocus() { return mNeedKeyFocus; }

		/** Set need mouse focus flag */
		void setNeedMouseFocus(bool _value) { mNeedMouseFocus = _value; }
		/** Is need mouse focus
			If disable this widget won't be reacting on mouse at all.\n
			Enabled (true) by default.
		*/
		bool isNeedMouseFocus() { return mNeedMouseFocus; }

		/** Set inherits mode flag
			This mode makes all child widgets pickable even if widget don't
			need mouse focus (was set setNeedKeyFocus(false) ).\n
			Disabled (false) by default.
		*/
		void setInheritsPick(bool _value) { mInheritsPick = _value; }
		/** Get inherits mode flag */
		bool isInheritsPick() { return mInheritsPick; }

		/** Set picking mask for widget */
		void setMaskPick(const std::string& _filename);

		/** Enable or disable widget */
		virtual void setEnabled(bool _value);
		/** Enable or disable widget without changing widget's state */
		void setEnabledSilent(bool _value) { mEnabled = _value; }
		/** Is widget enabled */
		bool isEnabled() { return mEnabled; }

		/** Set mouse pointer for this widget */
		void setPointer(const std::string& _value) { mPointer = _value; }
		/** Get mouse pointer name for this widget */
		const std::string& getPointer();

		/** Get widget's layer, return "" if widget is not root widget (root == without parents) */
		const std::string& getLayerName();

		/** Get rect where child widgets placed */
		IntCoord getClientCoord();

		/** Get clien area widget or nullptr if widget don't have client */
		Widget* getClientWidget() { return mWidgetClient; }

		/** Get text sub widget or nullptr if no text sub widget */
		ISubWidgetText * getSubWidgetText() { return mText; }
		/** Get sub widget of first texture or nullptr if no sub widget with texture */
		ISubWidgetRect * getSubWidgetMain() { return mMainSkin; }

		/** Set need tool tip mode flag. Enable this if you need tool tip events for widget */
		void setNeedToolTip(bool _value);
		/** Get need tool tip mode flag */
		bool getNeedToolTip() { return mNeedToolTip; }

		/** Enable or disable tooltip event */
		void setEnableToolTip(bool _value);
		/** Get tool tip enabled flag */
		bool getEnableToolTip() { return mEnableToolTip; }

		/** Detach widget from widgets hierarchy
			@param _layer Attach to specified layer (if any)
		*/
		void detachFromWidget(const std::string& _layer = "");

		/** Attach widget to parent
			@param _style Child widget type
			@param _layer Attach to specified layer (if any)
			@note you might also need to call void Widget::setWidgetStyle(WidgetStyle _style);
				to set widget style (widget attached with MyGUI::WidgetStyle::Popup by default)
		*/
		void attachToWidget(Widget* _parent, WidgetStyle _style = WidgetStyle::Child, const std::string& _layer = "");

		/** Change widget skin */
		void changeWidgetSkin(const std::string& _skinname);

		/** Set widget style.
			@param _layer Attach to specified layer (if any)
			@note When choosing WidgetStyle::Popup style you also need attach widget to layer
			see LayerManager::attachToLayerNode
		*/
		void setWidgetStyle(WidgetStyle _style, const std::string& _layer = "");
		/** Get widget style */
		WidgetStyle getWidgetStyle() { return mWidgetStyle; }

		/** Set any widget property
			@param _key Property name (for example Widget_Alpha or Edit_MultiLine)
			@param _value Value converted to string
		*/
		virtual void setProperty(const std::string& _key, const std::string& _value);


	/*internal:*/
		// метод для запроса номера айтема и контейнера
		virtual void _getContainer(Widget*& _container, size_t& _index);

		// дает приоритет виджету при пиккинге
		void _forcePeek(Widget* _widget);

		void _setUVSet(const FloatRect& _rect);

		virtual void _setTextureName(const std::string& _texture);
		virtual const std::string& _getTextureName();

		IWidgetCreator * _getIWidgetCreator() { return mIWidgetCreator; }

		IntCoord _getTextRegion();
		IntSize _getTextSize();
		void _setFontName(const std::string& _font);
		const std::string& _getFontName();
		void _setFontHeight(int _height);
		int _getFontHeight();
		void _setTextAlign(Align _align);
		Align _getTextAlign();
		void _setTextColour(const Colour& _colour);
		const Colour& _getTextColour();

		// устанавливает строку заменив /n на реальный перенос
		void setCaptionWithNewLine(const std::string& _value);
		virtual void _initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name);


	/*obsolete:*/
#ifndef MYGUI_DONT_USE_OBSOLETE

		MYGUI_OBSOLETE("use : void Widget::setCoord(const IntCoord& _coord)")
		void setPosition(const IntCoord& _coord) { setCoord(_coord); }
		MYGUI_OBSOLETE("use : void Widget::setCoord(int _left, int _top, int _width, int _height)")
		void setPosition(int _left, int _top, int _width, int _height) { setCoord(_left, _top, _width, _height); }

		MYGUI_OBSOLETE("use : void Widget::setEnableToolTip")
		void enableToolTip(bool _enable) { setEnableToolTip(_enable); }

		MYGUI_OBSOLETE("use : void setInheritsPick(bool _inherits)")
		void setInheritsPeek(bool _inherits) { setInheritsPick(_inherits); }
		MYGUI_OBSOLETE("use : bool isInheritsPick()")
		bool isInheritsPeek() { return isInheritsPick(); }

		MYGUI_OBSOLETE("use : void setMaskPick(const std::string& _filename)")
		void setMaskPeek(const std::string& _filename) { setMaskPick(_filename); }

		MYGUI_OBSOLETE("use : const IntCoord& StaticText::getTextRegion()")
		IntCoord getTextCoord() { return _getTextRegion(); }
		MYGUI_OBSOLETE("use : IntSize StaticText::getTextSize()")
		IntSize getTextSize() { return _getTextSize(); }

		MYGUI_OBSOLETE("use : void StaticText::setFontName(const std::string& _font)")
		void setFontName(const std::string& _font) { _setFontName(_font); }
		MYGUI_OBSOLETE("use : const std::string& StaticText::getFontName()")
		const std::string& getFontName() { return _getFontName(); }

		MYGUI_OBSOLETE("use : void StaticText::setFontHeight(int _height)")
		void setFontHeight(int _height) { _setFontHeight(_height); }
		MYGUI_OBSOLETE("use : int StaticText::getFontHeight()")
		int getFontHeight() { return _getFontHeight(); }

		MYGUI_OBSOLETE("use : void StaticText::setTextAlign(Align _align)")
		void setTextAlign(Align _align) { _setTextAlign(_align); }
		MYGUI_OBSOLETE("use : Align StaticText::getTextAlign()")
		Align getTextAlign() { return _getTextAlign(); }

		MYGUI_OBSOLETE("use : void StaticText::setTextColour(const Colour& _colour)")
		void setTextColour(const Colour& _colour) { _setTextColour(_colour); }
		MYGUI_OBSOLETE("use : const Colour& StaticText::getTextColour()")
		const Colour& getTextColour() { return _getTextColour(); }

#endif // MYGUI_DONT_USE_OBSOLETE

	protected:
		// все создание только через фабрику
		Widget(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name);
		virtual ~Widget();

		virtual void baseChangeWidgetSkin(ResourceSkin* _info);

		void _updateView(); // обновления себя и детей

		void _setAlign(const IntSize& _oldsize, bool _update);
		void _setAlign(const IntCoord& _oldcoord, bool _update);

		// создает виджет
		virtual Widget* baseCreateWidget(WidgetStyle _style, const std::string& _type, const std::string& _skin, const IntCoord& _coord, Align _align, const std::string& _layer, const std::string& _name);

		// удяляет неудачника
		virtual void _destroyChildWidget(Widget* _widget);

		// удаляет всех детей
		virtual void _destroyAllChildWidget();

		// запрашиваем у конейтера айтем по позиции мыши
		virtual size_t _getContainerIndex(const IntPoint& _point) { return ITEM_NONE; }

		// сброс всех данных контейнера, тултипы и все остальное
		virtual void _resetContainer(bool _update);

		virtual void baseUpdateEnable();

		// наследуемся он LayerInfo
		virtual ILayerItem * getLayerItemByPoint(int _left, int _top);
		virtual const IntCoord& getLayerItemCoord() { return mCoord; }

	private:

		void frameEntered(float _frame);

		void initialiseWidgetSkin(ResourceSkin* _info, const IntSize& _size);
		void shutdownWidgetSkin(bool _deep = false);

		void _updateAlpha();
		void _updateAbsolutePoint();

		// для внутреннего использования
		void _setInheritsVisible(bool _value);
		bool _isInheritsVisible() { return mInheritsVisible; }

		void _setInheritsEnable(bool _value);
		bool _isInheritsEnable() { return mInheritsEnabled; }

		// показывает скрывает все сабскины
		void _setSubSkinVisible(bool _visible);

		float _getRealAlpha() { return mRealAlpha; }

		// добавляет в список виджет
		virtual void _linkChildWidget(Widget* _widget);
		// удаляет из списка
		virtual void _unlinkChildWidget(Widget* _widget);

	protected:
		// список всех стейтов
		MapWidgetStateInfo mStateInfo;
		// информация о маске для пикинга
		MaskPickInfo const * mMaskPickInfo;
		MaskPickInfo mOwnMaskPickInfo;

		// вектор всех детей виджетов
		VectorWidgetPtr mWidgetChild;
		// вектор детей скина
		VectorWidgetPtr mWidgetChildSkin;
		// вектор всех детей сабскинов
		VectorSubWidget mSubSkinChild;

		// указатель на окно текста
		ISubWidgetText * mText;
		// указатель на первый не текстовой сабскин
		ISubWidgetRect * mMainSkin;

		// доступен ли на виджет
		bool mEnabled;
		bool mInheritsEnabled;
		// скрыты ли все сабскины при выходе за границу
		bool mSubSkinsVisible;
		// для иерархического скрытия
		bool mInheritsVisible;
		// прозрачность и флаг наследования альфы нашего оверлея
		float mAlpha;
		float mRealAlpha;
		bool mInheritsAlpha;
		// имя виджета
		std::string mName;
		// курсор который будет показан при наведении
		std::string mPointer;
		std::string mTextureName;
		ITexture* mTexture;

		// наш отец в иерархии виджетов
		Widget* mParent;

		// это тот кто нас создал, и кто нас будет удалять
		IWidgetCreator * mIWidgetCreator;

		// нужен ли виджету ввод с клавы
		bool mNeedKeyFocus;
		// нужен ли виджету фокус мыши
		bool mNeedMouseFocus;
		bool mInheritsPick;

		// клиентская зона окна
		// если виджет имеет пользовательские окна не в себе
		// то обязательно проинициализировать Client
		Widget* mWidgetClient;

		bool mNeedToolTip;
		bool mEnableToolTip;
		bool mToolTipVisible;
		float mToolTipCurrentTime;
		IntPoint mToolTipOldPoint;
		size_t mToolTipOldIndex;
		IntPoint m_oldMousePoint;

		// поведение виджета, перекрывающийся дочерний или всплывающий
		WidgetStyle mWidgetStyle;

		FloatCoord mRelativeCoord;
		bool mDisableUpdateRelative;

	};

} // namespace MyGUI

#endif // __MYGUI_WIDGET_H__
