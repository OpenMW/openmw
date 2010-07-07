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
#include "MyGUI_SkinManager.h"
#include "MyGUI_SubWidgetManager.h"
#include "MyGUI_WidgetManager.h"
#include "MyGUI_ResourceSkin.h"
#include "MyGUI_WidgetDefines.h"
#include "MyGUI_LayerItem.h"
#include "MyGUI_LayerManager.h"
#include "MyGUI_RenderItem.h"
#include "MyGUI_ISubWidget.h"
#include "MyGUI_ISubWidgetText.h"
#include "MyGUI_StaticText.h"
#include "MyGUI_FactoryManager.h"
#include "MyGUI_LanguageManager.h"
#include "MyGUI_CoordConverter.h"
#include "MyGUI_RenderManager.h"

namespace MyGUI
{

	const float WIDGET_TOOLTIP_TIMEOUT = 0.5f;

	Widget::Widget(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name) :
		mMaskPickInfo(nullptr),
		mText(nullptr),
		mMainSkin(nullptr),
		mEnabled(true),
		mInheritsEnabled(true),
		mSubSkinsVisible(true),
		mInheritsVisible(true),
		mAlpha(ALPHA_MIN),
		mRealAlpha(ALPHA_MIN),
		mInheritsAlpha(true),
		mTexture(nullptr),
		mParent(nullptr),
		mIWidgetCreator(nullptr),
		mNeedKeyFocus(false),
		mNeedMouseFocus(true),
		mInheritsPick(false),
		mWidgetClient(nullptr),
		mNeedToolTip(false),
		mEnableToolTip(true),
		mToolTipVisible(false),
		mToolTipCurrentTime(0),
		mToolTipOldIndex(ITEM_NONE),
		mWidgetStyle(WidgetStyle::Child),
		mDisableUpdateRelative(false)
	{
		_initialise(_style, _coord, _align, _info, _parent, _croppedParent, _creator, _name);
	}

	Widget::Widget() :
		mMaskPickInfo(nullptr),
		mText(nullptr),
		mMainSkin(nullptr),
		mEnabled(true),
		mInheritsEnabled(true),
		mSubSkinsVisible(true),
		mInheritsVisible(true),
		mAlpha(ALPHA_MIN),
		mRealAlpha(ALPHA_MIN),
		mInheritsAlpha(true),
		mTexture(nullptr),
		mParent(nullptr),
		mIWidgetCreator(nullptr),
		mNeedKeyFocus(false),
		mNeedMouseFocus(true),
		mInheritsPick(false),
		mWidgetClient(nullptr),
		mNeedToolTip(false),
		mEnableToolTip(true),
		mToolTipVisible(false),
		mToolTipCurrentTime(0),
		mToolTipOldIndex(ITEM_NONE),
		mWidgetStyle(WidgetStyle::Child),
		mDisableUpdateRelative(false)
	{
	}

	void Widget::_initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name)
	{
		mCoord = IntCoord(_coord.point(), _info->getSize());
		mStateInfo = _info->getStateInfo();
		mMaskPickInfo = _info->getMask();

		mTextureName = _info->getTextureName();
		mTexture = RenderManager::getInstance().getTexture(mTextureName);

		mAlign = _align;
		mCroppedParent = _croppedParent;

		mName = _name;
		mParent = _parent;
		mIWidgetCreator = _creator;

		mWidgetStyle = _style;

		// имя отсылателя сообщений
		mWidgetEventSender = this;

#if MYGUI_DEBUG_MODE == 1
		// проверяем соответсвие входных данных
		if (mWidgetStyle == WidgetStyle::Child)
		{
			MYGUI_ASSERT(mCroppedParent, "must be cropped");
			MYGUI_ASSERT(mParent, "must be parent");
		}
		else if (mWidgetStyle == WidgetStyle::Overlapped)
		{
			MYGUI_ASSERT((mParent == nullptr) == (mCroppedParent == nullptr), "error cropped");
		}
		else if (mWidgetStyle == WidgetStyle::Popup)
		{
			MYGUI_ASSERT(!mCroppedParent, "cropped must be nullptr");
			MYGUI_ASSERT(mParent, "must be parent");
		}
#endif

		// корректируем абсолютные координаты
		mAbsolutePosition = _coord.point();

		if (nullptr != mCroppedParent)
		{
			mAbsolutePosition += mCroppedParent->getAbsolutePosition();
		}

		const IntSize& parent_size = mCroppedParent ? mCroppedParent->getSize() : Gui::getInstance().getViewSize();

		if (parent_size.width)
		{
			mRelativeCoord.left = (float)_coord.left / (float)parent_size.width;
			mRelativeCoord.width = (float)_coord.width / (float)parent_size.width;
		}
		else
		{
			mRelativeCoord.left = 0;
			mRelativeCoord.width = 0;
		}

		if (parent_size.height)
		{
			mRelativeCoord.top = (float)_coord.top / (float)parent_size.height;
			mRelativeCoord.height = (float)_coord.height / (float)parent_size.height;
		}
		else
		{
			mRelativeCoord.top = 0;
			mRelativeCoord.height = 0;
		}

		initialiseWidgetSkin(_info, _coord.size());

		// дочернее окно обыкновенное
		if (mWidgetStyle == WidgetStyle::Child)
		{
			if (mParent) mParent->addChildItem(this);
		}
		// дочернее нуно перекрывающееся
		else if (mWidgetStyle == WidgetStyle::Overlapped)
		{
			// дочернее перекрывающееся
			if (mParent) mParent->addChildNode(this);
		}
	}

	Widget::~Widget()
	{
		Gui::getInstance().eventFrameStart -= newDelegate(this, &Widget::frameEntered);

		if (mToolTipVisible) eventToolTip(this, ToolTipInfo(ToolTipInfo::Hide));

		shutdownWidgetSkin(true);

		_destroyAllChildWidget();

		// дочернее окно обыкновенное
		if (mWidgetStyle == WidgetStyle::Child)
		{
			if (mParent) mParent->removeChildItem(this);
		}
		// дочернее нуно перекрывающееся
		else if (mWidgetStyle == WidgetStyle::Overlapped)
		{
			// дочернее перекрывающееся
			if (mParent) mParent->removeChildNode(this);
		}
	}

	void Widget::changeWidgetSkin(const std::string& _skinname)
	{
		ResourceSkin* skin_info = SkinManager::getInstance().getByName(_skinname);
		baseChangeWidgetSkin(skin_info);
	}

	void Widget::baseChangeWidgetSkin(ResourceSkin* _info)
	{
		IntSize size = mCoord.size();

		saveLayerItem();

		shutdownWidgetSkin();
		initialiseWidgetSkin(_info, size);

		restoreLayerItem();
	}

	void Widget::initialiseWidgetSkin(ResourceSkin* _info, const IntSize& _size)
	{
		FactoryManager& factory = FactoryManager::getInstance();

		mTextureName = _info->getTextureName();
		mTexture = RenderManager::getInstance().getTexture(mTextureName);

		setRenderItemTexture(mTexture);
		mStateInfo = _info->getStateInfo();
		Widget::setSize(_info->getSize());

		// загружаем кирпичики виджета
		for (VectorSubWidgetInfo::const_iterator iter=_info->getBasisInfo().begin(); iter!=_info->getBasisInfo().end(); ++iter)
		{
			IObject* object = factory.createObject("BasisSkin", (*iter).type);
			if (object == nullptr) continue;

			ISubWidget* sub = object->castType<ISubWidget>();
			sub->_setCroppedParent(this);
			sub->setCoord((*iter).coord);
			sub->setAlign((*iter).align);

			mSubSkinChild.push_back(sub);
			addRenderItem(sub);

			// ищем дефолтные сабвиджеты
			if (mMainSkin == nullptr) mMainSkin = sub->castType<ISubWidgetRect>(false);
			if (mText == nullptr) mText = sub->castType<ISubWidgetText>(false);
		}

		if (!isRootWidget())
		{
			// проверяем наследуемую скрытость
			if ((!mParent->isVisible()) || (!mParent->_isInheritsVisible()))
			{
				bool value = false;
				mInheritsVisible = value;
				for (VectorSubWidget::iterator skin = mSubSkinChild.begin(); skin != mSubSkinChild.end(); ++skin)
					(*skin)->setVisible(value);
				for (VectorWidgetPtr::iterator widget = mWidgetChild.begin(); widget != mWidgetChild.end(); ++widget)
					(*widget)->_setInheritsVisible(value);
				for (VectorWidgetPtr::iterator widget = mWidgetChildSkin.begin(); widget != mWidgetChildSkin.end(); ++widget)
					(*widget)->_setInheritsVisible(value);
			}
			// проверяем наследуемый дизейбл
			if ((!mParent->isEnabled()) || (!mParent->_isInheritsEnable()))
			{
				bool value = false;
				mInheritsEnabled = false;
				for (VectorWidgetPtr::iterator iter = mWidgetChild.begin(); iter != mWidgetChild.end(); ++iter)
					(*iter)->_setInheritsEnable(value);
				for (VectorWidgetPtr::iterator iter = mWidgetChildSkin.begin(); iter != mWidgetChildSkin.end(); ++iter)
					(*iter)->_setInheritsEnable(value);
			}
		}

		Widget::setState("normal");//FIXME - явный вызов

		// парсим свойства
		const MapString& properties = _info->getProperties();
		if (!properties.empty())
		{
			MapString::const_iterator iter = properties.end();
			if ((iter = properties.find("NeedKey")) != properties.end()) setNeedKeyFocus(utility::parseBool(iter->second));
			if ((iter = properties.find("NeedMouse")) != properties.end()) setNeedMouseFocus(utility::parseBool(iter->second));
			if ((iter = properties.find("Pointer")) != properties.end()) mPointer = iter->second;
			if ((iter = properties.find("Visible")) != properties.end()) { setVisible(utility::parseBool(iter->second)); }

			// OBSOLETE
			if ((iter = properties.find("AlignText")) != properties.end()) _setTextAlign(Align::parse(iter->second));
			if ((iter = properties.find("Colour")) != properties.end()) _setTextColour(Colour::parse(iter->second));
			if ((iter = properties.find("Show")) != properties.end()) { setVisible(utility::parseBool(iter->second)); }
			if ((iter = properties.find("TextAlign")) != properties.end()) _setTextAlign(Align::parse(iter->second));
			if ((iter = properties.find("TextColour")) != properties.end()) _setTextColour(Colour::parse(iter->second));
			if ((iter = properties.find("FontName")) != properties.end()) _setFontName(iter->second);
			if ((iter = properties.find("FontHeight")) != properties.end()) _setFontHeight(utility::parseInt(iter->second));
		}

		// выставляем альфу, корректировка по отцу автоматически
		Widget::setAlpha(ALPHA_MAX);//FIXME - явный вызов

		// создаем детей скина
		const VectorChildSkinInfo& child = _info->getChild();
		for (VectorChildSkinInfo::const_iterator iter=child.begin(); iter!=child.end(); ++iter)
		{
			//FIXME - явный вызов
			Widget* widget = Widget::baseCreateWidget(iter->style, iter->type, iter->skin, iter->coord, iter->align, iter->layer, "");
			widget->_setInternalData(iter->name);
			// заполняем UserString пропертями
			for (MapString::const_iterator prop=iter->params.begin(); prop!=iter->params.end(); ++prop)
			{
				widget->setUserString(prop->first, prop->second);
			}
			// для детей скина свой список
			mWidgetChildSkin.push_back(widget);
			mWidgetChild.pop_back();
		}

		Widget::setSize(_size);//FIXME - явный вызов
	}

	void Widget::shutdownWidgetSkin(bool _deep)
	{
		// удаляем все сабскины
		mMainSkin = nullptr;
		mText = nullptr;

		removeAllRenderItems();

		for (VectorSubWidget::iterator skin = mSubSkinChild.begin(); skin != mSubSkinChild.end(); ++skin)
		{
			delete (*skin);
		}
		mSubSkinChild.clear();

		mStateInfo.clear();

		// удаляем виджеты чтобы ли в скине
		for (VectorWidgetPtr::iterator iter=mWidgetChildSkin.begin(); iter!=mWidgetChildSkin.end(); ++iter)
		{
			// Добавляем себя чтобы удалилось
			mWidgetChild.push_back(*iter);
			_destroyChildWidget(*iter);
		}
		mWidgetChildSkin.clear();
	}

	Widget* Widget::baseCreateWidget(WidgetStyle _style, const std::string& _type, const std::string& _skin, const IntCoord& _coord, Align _align, const std::string& _layer, const std::string& _name)
	{
		Widget* widget = WidgetManager::getInstance().createWidget(_style, _type, _skin, _coord, _align, this,
			_style == WidgetStyle::Popup ? nullptr : this, this, _name);

		mWidgetChild.push_back(widget);

		// присоединяем виджет с уровню
		if (!_layer.empty() && widget->isRootWidget()) LayerManager::getInstance().attachToLayerNode(_layer, widget);

		return widget;
	}

	Widget* Widget::createWidgetRealT(const std::string& _type, const std::string& _skin, const FloatCoord& _coord, Align _align, const std::string& _name)
	{
		return createWidgetT(_type, _skin, CoordConverter::convertFromRelative(_coord, getSize()), _align, _name);
	}

	void Widget::_updateView()
	{

		bool margin = mCroppedParent ? _checkMargin() : false;

		// вьюпорт стал битым
		if (margin)
		{
			// проверка на полный выход за границу
			if (_checkOutside())
			{
				// запоминаем текущее состояние
				mIsMargin = margin;

				// скрываем
				_setSubSkinVisible(false);

				// для тех кому нужно подправить себя при движении
				//for (VectorSubWidget::iterator skin = mSubSkinChild.begin(); skin != mSubSkinChild.end(); ++skin) (*skin)->_updateView();

				// вся иерархия должна быть проверенна
				for (VectorWidgetPtr::iterator widget = mWidgetChild.begin(); widget != mWidgetChild.end(); ++widget) (*widget)->_updateView();
				for (VectorWidgetPtr::iterator widget = mWidgetChildSkin.begin(); widget != mWidgetChildSkin.end(); ++widget) (*widget)->_updateView();

				return;
			}

		}
		// мы не обрезаны и были нормальные
		else if (!mIsMargin)
		{
			// запоминаем текущее состояние
			//mIsMargin = margin;

			//_setSubSkinVisible(true);
			// для тех кому нужно подправить себя при движении
			for (VectorSubWidget::iterator skin = mSubSkinChild.begin(); skin != mSubSkinChild.end(); ++skin) (*skin)->_updateView();

			return;
		}

		// запоминаем текущее состояние
		mIsMargin = margin;

		// если скин был скрыт, то покажем
		_setSubSkinVisible(true);

		// обновляем наших детей, а они уже решат обновлять ли своих детей
		for (VectorWidgetPtr::iterator widget = mWidgetChild.begin(); widget != mWidgetChild.end(); ++widget) (*widget)->_updateView();
		for (VectorWidgetPtr::iterator widget = mWidgetChildSkin.begin(); widget != mWidgetChildSkin.end(); ++widget) (*widget)->_updateView();
		for (VectorSubWidget::iterator skin = mSubSkinChild.begin(); skin != mSubSkinChild.end(); ++skin) (*skin)->_updateView();

	}

	void Widget::setCaption(const UString& _caption)
	{
		if (nullptr != mText) mText->setCaption(_caption);
	}

	const UString& Widget::getCaption()
	{
		if (nullptr == mText)
		{
			static UString empty;
			return empty;
		}
		return mText->getCaption();
	}

	bool Widget::setState(const std::string& _state)
	{
		MapWidgetStateInfo::const_iterator iter = mStateInfo.find(_state);
		if (iter == mStateInfo.end()) return false;
		size_t index=0;
		for (VectorSubWidget::iterator skin = mSubSkinChild.begin(); skin != mSubSkinChild.end(); ++skin, ++index)
		{
			IStateInfo* data = (*iter).second[index];
			if (data != nullptr)
			{
				(*skin)->setStateData(data);
			}
		}
		return true;
	}

	void Widget::_destroyChildWidget(Widget* _widget)
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
	void Widget::_destroyAllChildWidget()
	{
		WidgetManager& manager = WidgetManager::getInstance();
		while (!mWidgetChild.empty())
		{

			// сразу себя отписывем, иначе вложенной удаление убивает все
			Widget* widget = mWidgetChild.back();
			mWidgetChild.pop_back();

			//if (widget->isRootWidget()) widget->detachWidget();

			// отписываем от всех
			manager.unlinkFromUnlinkers(widget);

			// и сами удалим, так как его больше в списке нет
			delete widget;
		}
	}

	IntCoord Widget::getClientCoord()
	{
		MYGUI_ASSERT(mWidgetClient != this, "mWidgetClient can not be this widget");
		if (mWidgetClient != nullptr) return mWidgetClient->getCoord();
		return IntCoord(0, 0, mCoord.width, mCoord.height);
	}

	void Widget::setAlpha(float _alpha)
	{
		if (mAlpha == _alpha) return;
		mAlpha = _alpha;
		if (nullptr != mParent) mRealAlpha = mAlpha * (mInheritsAlpha ? mParent->_getRealAlpha() : ALPHA_MAX);
		else mRealAlpha = mAlpha;

		for (VectorWidgetPtr::iterator widget = mWidgetChild.begin(); widget != mWidgetChild.end(); ++widget) (*widget)->_updateAlpha();
		for (VectorWidgetPtr::iterator widget = mWidgetChildSkin.begin(); widget != mWidgetChildSkin.end(); ++widget) (*widget)->_updateAlpha();
		for (VectorSubWidget::iterator skin = mSubSkinChild.begin(); skin != mSubSkinChild.end(); ++skin) (*skin)->setAlpha(mRealAlpha);
	}

	void Widget::_updateAlpha()
	{
		MYGUI_DEBUG_ASSERT(nullptr != mParent, "Widget must have parent");
		mRealAlpha = mAlpha * (mInheritsAlpha ? mParent->_getRealAlpha() : ALPHA_MAX);

		for (VectorWidgetPtr::iterator widget = mWidgetChild.begin(); widget != mWidgetChild.end(); ++widget) (*widget)->_updateAlpha();
		for (VectorWidgetPtr::iterator widget = mWidgetChildSkin.begin(); widget != mWidgetChildSkin.end(); ++widget) (*widget)->_updateAlpha();
		for (VectorSubWidget::iterator skin = mSubSkinChild.begin(); skin != mSubSkinChild.end(); ++skin) (*skin)->setAlpha(mRealAlpha);
	}

	void Widget::setInheritsAlpha(bool _inherits)
	{
		mInheritsAlpha = _inherits;
		// принудительно обновляем
		float alpha = mAlpha;
		mAlpha = 101;
		setAlpha(alpha);
	}

	ILayerItem * Widget::getLayerItemByPoint(int _left, int _top)
	{
		// проверяем попадание
		if (!mSubSkinsVisible
			|| !mEnabled
			|| !mVisible
			|| (!mNeedMouseFocus && !mInheritsPick)
			|| !_checkPoint(_left, _top)
			// если есть маска, проверяем еще и по маске
			|| ((!mMaskPickInfo->empty()) && (!mMaskPickInfo->pick(IntPoint(_left - mCoord.left, _top - mCoord.top), mCoord))))
				return nullptr;
		// спрашиваем у детишек
		for (VectorWidgetPtr::reverse_iterator widget= mWidgetChild.rbegin(); widget != mWidgetChild.rend(); ++widget)
		{
			// общаемся только с послушными детьми
			if ((*widget)->mWidgetStyle == WidgetStyle::Popup) continue;

			ILayerItem * item = (*widget)->getLayerItemByPoint(_left - mCoord.left, _top - mCoord.top);
			if (item != nullptr) return item;
		}
		// спрашиваем у детишек скна
		for (VectorWidgetPtr::reverse_iterator widget= mWidgetChildSkin.rbegin(); widget != mWidgetChildSkin.rend(); ++widget)
		{
			ILayerItem * item = (*widget)->getLayerItemByPoint(_left - mCoord.left, _top - mCoord.top);
			if (item != nullptr) return item;
		}
		// непослушные дети
		return mInheritsPick ? nullptr : this;
	}

	void Widget::_updateAbsolutePoint()
	{
		// мы рут, нам не надо
		if (!mCroppedParent) return;

		mAbsolutePosition = mCroppedParent->getAbsolutePosition() + mCoord.point();

		for (VectorWidgetPtr::iterator widget = mWidgetChild.begin(); widget != mWidgetChild.end(); ++widget) (*widget)->_updateAbsolutePoint();
		for (VectorWidgetPtr::iterator widget = mWidgetChildSkin.begin(); widget != mWidgetChildSkin.end(); ++widget) (*widget)->_updateAbsolutePoint();
		for (VectorSubWidget::iterator skin = mSubSkinChild.begin(); skin != mSubSkinChild.end(); ++skin) (*skin)->_correctView();
	}

	void Widget::_setUVSet(const FloatRect& _rect)
	{
		if (nullptr != mMainSkin) mMainSkin->_setUVSet(_rect);
	}

	void Widget::_setTextureName(const std::string& _texture)
	{
		//if (_texture == mTextureName) return;

		mTextureName = _texture;
		mTexture = RenderManager::getInstance().getTexture(mTextureName);

		setRenderItemTexture(mTexture);
	}

	const std::string& Widget::_getTextureName()
	{
		return mTextureName;
	}

	void Widget::_setSubSkinVisible(bool _visible)
	{
		if (mSubSkinsVisible == _visible) return;
		mSubSkinsVisible = _visible;

		// просто обновляем
		for (VectorSubWidget::iterator skin = mSubSkinChild.begin(); skin != mSubSkinChild.end(); ++skin)
		{
			(*skin)->_updateView();
		}
	}

	void Widget::_forcePeek(Widget* _widget)
	{
		MYGUI_ASSERT(mWidgetClient != this, "mWidgetClient can not be this widget");
		if (mWidgetClient != nullptr) mWidgetClient->_forcePeek(_widget);

		size_t size = mWidgetChild.size();
		if ( (size < 2) || (mWidgetChild[size-1] == _widget) ) return;
		for (size_t pos=0; pos<size; pos++)
		{
			if (mWidgetChild[pos] == _widget)
			{
				mWidgetChild[pos] = mWidgetChild[size-1];
				mWidgetChild[size-1] = _widget;
				return;
			}
		}
	}

	const std::string& Widget::getLayerName()
	{
		ILayer* layer = getLayer();
		if (nullptr == layer)
		{
			static std::string empty;
			return empty;
		}
		return layer->getName();
	}

	void Widget::_getContainer(Widget*& _list, size_t& _index)
	{
		_list = nullptr;
		_index = ITEM_NONE;
		_requestGetContainer(this, _list, _index);
	}

	Widget* Widget::findWidget(const std::string& _name)
	{
		if (_name == mName) return this;
		MYGUI_ASSERT(mWidgetClient != this, "mWidgetClient can not be this widget");
		if (mWidgetClient != nullptr) return mWidgetClient->findWidget(_name);
		for (VectorWidgetPtr::iterator widget = mWidgetChild.begin(); widget != mWidgetChild.end(); ++widget)
		{
			Widget* find = (*widget)->findWidget(_name);
			if (nullptr != find) return find;
		}
		return nullptr;
	}

	void Widget::setNeedToolTip(bool _need)
	{
		if (mNeedToolTip == _need) return;
		mNeedToolTip = _need;

		if (mNeedToolTip)
		{
			Gui::getInstance().eventFrameStart += newDelegate(this, &Widget::frameEntered);
			mToolTipCurrentTime = 0;
		}
		else
		{
			Gui::getInstance().eventFrameStart -= newDelegate(this, &Widget::frameEntered);
		}
	}

	void Widget::frameEntered(float _frame)
	{
		if ( ! mEnableToolTip ) return;

		IntPoint point = InputManager::getInstance().getMousePositionByLayer();

		if (mToolTipOldPoint != point)
		{

			mToolTipCurrentTime = 0;

			bool inside = getAbsoluteRect().inside(point);
			if (inside)
			{
				inside = false;
				// проверяем не перекрывают ли нас
				Widget* widget = InputManager::getInstance().getMouseFocusWidget();
				while (widget != 0)
				{
					if (widget/*->getName()*/ == this/*mName*/)
					{
						inside = true;
						break;
					}
					// если виджет берет тултип, значит сбрасываем
					if (widget->getNeedToolTip())
						widget = 0;//widget->getParent();
					else
						widget = widget->getParent();
				}

				if (inside)
				{
					// теперь смотрим, не поменялся ли индекс внутри окна
					size_t index = _getContainerIndex(point);
					if (mToolTipOldIndex != index)
					{
						if (mToolTipVisible)
						{
							mToolTipCurrentTime = 0;
							mToolTipVisible = false;
							eventToolTip(this, ToolTipInfo(ToolTipInfo::Hide));
						}
						mToolTipOldIndex = index;
					}

				}
				else
				{
					if (mToolTipVisible)
					{
						mToolTipCurrentTime = 0;
						mToolTipVisible = false;
						eventToolTip(this, ToolTipInfo(ToolTipInfo::Hide));
					}
				}

			}
			else
			{
				if (mToolTipVisible)
				{
					mToolTipCurrentTime = 0;
					mToolTipVisible = false;
					eventToolTip(this, ToolTipInfo(ToolTipInfo::Hide));
				}
			}

			mToolTipOldPoint = point;
		}
		else
		{
			bool inside = getAbsoluteRect().inside(point);
			if (inside)
			{
				inside = false;
				// проверяем не перекрывают ли нас
				Widget* widget = InputManager::getInstance().getMouseFocusWidget();
				while (widget != 0)
				{
					if (widget/*->getName()*/ == this/*mName*/)
					{
						inside = true;
						break;
					}
					// если виджет берет тултип, значит сбрасываем
					if (widget->getNeedToolTip())
						widget = 0;//widget->getParent();
					else
						widget = widget->getParent();
				}

				if (inside)
				{
					if ( ! mToolTipVisible)
					{
						mToolTipCurrentTime += _frame;
						if (mToolTipCurrentTime > WIDGET_TOOLTIP_TIMEOUT)
						{
							mToolTipVisible = true;
							eventToolTip(this, ToolTipInfo(ToolTipInfo::Show, mToolTipOldIndex, point));
						}
					}
				}
			}
		}
	}

	void Widget::setEnableToolTip(bool _enable)
	{
		if (_enable == mEnableToolTip) return;
		mEnableToolTip = _enable;

		if ( ! mEnableToolTip)
		{
			if (mToolTipVisible)
			{
				mToolTipCurrentTime = 0;
				mToolTipVisible = false;
				eventToolTip(this, ToolTipInfo(ToolTipInfo::Hide));
			}
		}
		else
		{
			mToolTipCurrentTime = 0;
		}
	}

	void Widget::_resetContainer(bool _updateOnly)
	{
		if ( mEnableToolTip)
		{
			if (mToolTipVisible)
			{
				mToolTipVisible = false;
				eventToolTip(this, ToolTipInfo(ToolTipInfo::Hide));
			}
			mToolTipCurrentTime = 0;
			mToolTipOldIndex = ITEM_NONE;
		}
	}

	void Widget::setMaskPick(const std::string& _filename)
	{
		if (mOwnMaskPickInfo.load(_filename))
		{
			mMaskPickInfo = &mOwnMaskPickInfo;
		}
		else
		{
			MYGUI_LOG(Error, "mask not load '" << _filename << "'");
		}
	}

	void Widget::setRealPosition(const FloatPoint& _point)
	{
		setPosition(CoordConverter::convertFromRelative(_point, mCroppedParent == nullptr ? Gui::getInstance().getViewSize() : mCroppedParent->getSize()));
	}

	void Widget::setRealSize(const FloatSize& _size)
	{
		setSize(CoordConverter::convertFromRelative(_size, mCroppedParent == nullptr ? Gui::getInstance().getViewSize() : mCroppedParent->getSize()));
	}

	void Widget::setRealCoord(const FloatCoord& _coord)
	{
		setCoord(CoordConverter::convertFromRelative(_coord, mCroppedParent == nullptr ? Gui::getInstance().getViewSize() : mCroppedParent->getSize()));
	}

	void Widget::_linkChildWidget(Widget* _widget)
	{
		VectorWidgetPtr::iterator iter = std::find(mWidgetChild.begin(), mWidgetChild.end(), _widget);
		MYGUI_ASSERT(iter == mWidgetChild.end(), "widget already exist");
		mWidgetChild.push_back(_widget);
	}

	void Widget::_unlinkChildWidget(Widget* _widget)
	{
		VectorWidgetPtr::iterator iter = std::remove(mWidgetChild.begin(), mWidgetChild.end(), _widget);
		MYGUI_ASSERT(iter != mWidgetChild.end(), "widget not found");
		mWidgetChild.erase(iter);
	}

	void Widget::_setTextAlign(Align _align)
	{
		StaticText* text = this->castType<StaticText>(false);
		if (text) text->setTextAlign(_align);

		if (mText != nullptr) mText->setTextAlign(_align);
	}

	Align Widget::_getTextAlign()
	{
		StaticText* text = this->castType<StaticText>(false);
		if (text) return text->getTextAlign();

		if (mText != nullptr) return mText->getTextAlign();
		return Align::Default;
	}

	void Widget::_setTextColour(const Colour& _colour)
	{
		StaticText* text = this->castType<StaticText>(false);
		if (text) return text->setTextColour(_colour);

		if (nullptr != mText) mText->setTextColour(_colour);
	}

	const Colour& Widget::_getTextColour()
	{
		StaticText* text = this->castType<StaticText>(false);
		if (text) return text->getTextColour();

		return (nullptr == mText) ? Colour::Zero : mText->getTextColour();
	}

	void Widget::_setFontName(const std::string& _font)
	{
		StaticText* text = this->castType<StaticText>(false);
		if (text) text->setFontName(_font);

		if (nullptr != mText) mText->setFontName(_font);
	}

	const std::string& Widget::_getFontName()
	{
		StaticText* text = this->castType<StaticText>(false);
		if (text) return text->getFontName();

		if (nullptr == mText)
		{
			static std::string empty;
			return empty;
		}
		return mText->getFontName();
	}

	void Widget::_setFontHeight(int _height)
	{
		StaticText* text = this->castType<StaticText>(false);
		if (text) text->setFontHeight(_height);

		if (nullptr != mText) mText->setFontHeight(_height);
	}

	int Widget::_getFontHeight()
	{
		StaticText* text = this->castType<StaticText>(false);
		if (text) return text->getFontHeight();

		return (nullptr == mText) ? 0 : mText->getFontHeight();
	}

	IntSize Widget::_getTextSize()
	{
		StaticText* text = this->castType<StaticText>(false);
		if (text) return text->getTextSize();

		return (nullptr == mText) ? IntSize() : mText->getTextSize();
	}

	IntCoord Widget::_getTextRegion()
	{
		StaticText* text = this->castType<StaticText>(false);
		if (text) return text->getTextRegion();

		return (nullptr == mText) ? IntCoord() : mText->getCoord();
	}

	void Widget::_setAlign(const IntCoord& _oldcoord, bool _update)
	{
		// для виджета изменение х у  не меняються
		_setAlign(_oldcoord.size(), _update);
	}

	void Widget::_setAlign(const IntSize& _oldsize, bool _update)
	{
		const IntSize& size = mCroppedParent ? mCroppedParent->getSize() : Gui::getInstance().getViewSize();

		bool need_move = false;
		bool need_size = false;
		IntCoord coord = mCoord;

		// первоначальное выравнивание
		if (mAlign.isHRelative())
		{
			coord.left = int((float)size.width * mRelativeCoord.left);
			coord.width = int((float)size.width * mRelativeCoord.width);
		}
		else if (mAlign.isHStretch())
		{
			// растягиваем
			coord.width = mCoord.width + (size.width - _oldsize.width);
			need_size = true;
		}
		else if (mAlign.isRight())
		{
			// двигаем по правому краю
			coord.left = mCoord.left + (size.width - _oldsize.width);
			need_move = true;
		}
		else if (mAlign.isHCenter())
		{
			// выравнивание по горизонтали без растяжения
			coord.left = (size.width - mCoord.width) / 2;
			need_move = true;
		}

		if (mAlign.isVRelative())
		{
			coord.top = int((float)size.height * mRelativeCoord.top);
			coord.height = int((float)size.height * mRelativeCoord.height);
		}
		else if (mAlign.isVStretch())
		{
			// растягиваем
			coord.height = mCoord.height + (size.height - _oldsize.height);
			need_size = true;
		}
		else if (mAlign.isBottom())
		{
			// двигаем по нижнему краю
			coord.top = mCoord.top + (size.height - _oldsize.height);
			need_move = true;
		}
		else if (mAlign.isVCenter())
		{
			// выравнивание по вертикали без растяжения
			coord.top = (size.height - mCoord.height) / 2;
			need_move = true;
		}

		if (mAlign.isHRelative() || mAlign.isVRelative())
		{
			mDisableUpdateRelative = true;
			setCoord(coord);
			mDisableUpdateRelative = false;
		}
		else if (need_move)
		{
			if (need_size) setCoord(coord);
			else setPosition(coord.point());
		}
		else if (need_size)
		{
			setSize(coord.size());
		}
		else
		{
			_updateView(); // только если не вызвано передвижение и сайз
		}

	}

	void Widget::setPosition(const IntPoint& _point)
	{
		if (mAlign.isHRelative() || mAlign.isVRelative())
		{

			const IntSize& parent_size = mCroppedParent ? mCroppedParent->getSize() : Gui::getInstance().getViewSize();

			if (parent_size.width)
			{
				mRelativeCoord.left = (float)_point.left / (float)parent_size.width;
			}
			else
			{
				mRelativeCoord.left = 0;
			}

			if (parent_size.height)
			{
				mRelativeCoord.top = (float)_point.top / (float)parent_size.height;
			}
			else
			{
				mRelativeCoord.top = 0;
			}

		}

		// обновляем абсолютные координаты
		mAbsolutePosition += _point - mCoord.point();

		for (VectorWidgetPtr::iterator widget = mWidgetChild.begin(); widget != mWidgetChild.end(); ++widget) (*widget)->_updateAbsolutePoint();
		for (VectorWidgetPtr::iterator widget = mWidgetChildSkin.begin(); widget != mWidgetChildSkin.end(); ++widget) (*widget)->_updateAbsolutePoint();

		mCoord = _point;

		_updateView();
	}

	void Widget::setSize(const IntSize& _size)
	{
		if (mAlign.isHRelative() || mAlign.isVRelative())
		{

			const IntSize& parent_size = mCroppedParent ? mCroppedParent->getSize() : Gui::getInstance().getViewSize();

			if (parent_size.width)
			{
				mRelativeCoord.width = (float)_size.width / (float)parent_size.width;
			}
			else
			{
				mRelativeCoord.width = 0;
			}

			if (parent_size.height)
			{
				mRelativeCoord.height = (float)_size.height / (float)parent_size.height;
			}
			else
			{
				mRelativeCoord.height = 0;
			}

		}

		// устанавливаем новую координату а старую пускаем в расчеты
		IntSize old = mCoord.size();
		mCoord = _size;

		bool visible = true;

		// обновляем выравнивание
		bool margin = mCroppedParent ? _checkMargin() : false;

		if (margin)
		{
			// проверка на полный выход за границу
			if (_checkOutside())
			{
				// скрываем
				visible = false;
			}
		}

		_setSubSkinVisible(visible);

		// передаем старую координату , до вызова, текущая координата отца должна быть новой
		for (VectorWidgetPtr::iterator widget = mWidgetChild.begin(); widget != mWidgetChild.end(); ++widget) (*widget)->_setAlign(old, mIsMargin || margin);
		for (VectorWidgetPtr::iterator widget = mWidgetChildSkin.begin(); widget != mWidgetChildSkin.end(); ++widget) (*widget)->_setAlign(old, mIsMargin || margin);
		for (VectorSubWidget::iterator skin = mSubSkinChild.begin(); skin != mSubSkinChild.end(); ++skin) (*skin)->_setAlign(old, mIsMargin || margin);

		// запоминаем текущее состояние
		mIsMargin = margin;

	}

	void Widget::setCoord(const IntCoord& _coord)
	{
		if (!mDisableUpdateRelative && (mAlign.isHRelative() || mAlign.isVRelative()))
		{

			const IntSize& parent_size = mCroppedParent ? mCroppedParent->getSize() : Gui::getInstance().getViewSize();

			if (parent_size.width)
			{
				mRelativeCoord.left = (float)_coord.left / (float)parent_size.width;
				mRelativeCoord.width = (float)_coord.width / (float)parent_size.width;
			}
			else
			{
				mRelativeCoord.left = 0;
				mRelativeCoord.width = 0;
			}

			if (parent_size.height)
			{
				mRelativeCoord.top = (float)_coord.top / (float)parent_size.height;
				mRelativeCoord.height = (float)_coord.height / (float)parent_size.height;
			}
			else
			{
				mRelativeCoord.top = 0;
				mRelativeCoord.height = 0;
			}

		}

		// обновляем абсолютные координаты
		mAbsolutePosition += _coord.point() - mCoord.point();

		for (VectorWidgetPtr::iterator widget = mWidgetChild.begin(); widget != mWidgetChild.end(); ++widget) (*widget)->_updateAbsolutePoint();
		for (VectorWidgetPtr::iterator widget = mWidgetChildSkin.begin(); widget != mWidgetChildSkin.end(); ++widget) (*widget)->_updateAbsolutePoint();

		// устанавливаем новую координату а старую пускаем в расчеты
		IntCoord old = mCoord;
		mCoord = _coord;

		bool visible = true;

		// обновляем выравнивание
		bool margin = mCroppedParent ? _checkMargin() : false;

		if (margin)
		{
			// проверка на полный выход за границу
			if (_checkOutside())
			{
				// скрываем
				visible = false;
			}
		}

		_setSubSkinVisible(visible);

		// передаем старую координату , до вызова, текущая координата отца должна быть новой
		for (VectorWidgetPtr::iterator widget = mWidgetChild.begin(); widget != mWidgetChild.end(); ++widget) (*widget)->_setAlign(old, mIsMargin || margin);
		for (VectorWidgetPtr::iterator widget = mWidgetChildSkin.begin(); widget != mWidgetChildSkin.end(); ++widget) (*widget)->_setAlign(old, mIsMargin || margin);
		for (VectorSubWidget::iterator skin = mSubSkinChild.begin(); skin != mSubSkinChild.end(); ++skin) (*skin)->_setAlign(old, mIsMargin || margin);

		// запоминаем текущее состояние
		mIsMargin = margin;

	}

	void Widget::setAlign(Align _align)
	{
		ICroppedRectangle::setAlign(_align);

		if (mAlign.isHRelative() || mAlign.isVRelative())
		{
			const IntSize& parent_size = mCroppedParent ? mCroppedParent->getSize() : Gui::getInstance().getViewSize();

			if (parent_size.width)
			{
				mRelativeCoord.left = (float)mCoord.left / (float)parent_size.width;
				mRelativeCoord.width = (float)mCoord.width / (float)parent_size.width;
			}
			else
			{
				mRelativeCoord.left = 0;
				mRelativeCoord.width = 0;
			}

			if (parent_size.height)
			{
				mRelativeCoord.top = (float)mCoord.top / (float)parent_size.height;
				mRelativeCoord.height = (float)mCoord.height / (float)parent_size.height;
			}
			else
			{
				mRelativeCoord.top = 0;
				mRelativeCoord.height = 0;
			}

		}

	}

	void Widget::detachFromWidget(const std::string& _layer)
	{
		std::string oldlayer = getLayerName();

		Widget* parent = getParent();
		if (parent)
		{
			// отдетачиваемся от лееров
			if ( ! isRootWidget() )
			{
				detachFromLayerItemNode(true);

				if (mWidgetStyle == WidgetStyle::Child)
				{
					mParent->removeChildItem(this);
				}
				else if (mWidgetStyle == WidgetStyle::Overlapped)
				{
					mParent->removeChildNode(this);
				}

				mWidgetStyle = WidgetStyle::Overlapped;

				mCroppedParent = nullptr;

				// обновляем координаты
				mAbsolutePosition = mCoord.point();

				for (VectorWidgetPtr::iterator widget = mWidgetChild.begin(); widget != mWidgetChild.end(); ++widget) (*widget)->_updateAbsolutePoint();
				for (VectorWidgetPtr::iterator widget = mWidgetChildSkin.begin(); widget != mWidgetChildSkin.end(); ++widget) (*widget)->_updateAbsolutePoint();

				// сбрасываем обрезку
				mMargin.clear();

				_updateView();
			}

			// нам нужен самый рутовый парент
			while (parent->getParent()) { parent = parent->getParent(); }

			mIWidgetCreator = parent->mIWidgetCreator;
			mIWidgetCreator->_linkChildWidget(this);
			mParent->_unlinkChildWidget(this);
			mParent = nullptr;
		}

		if (!_layer.empty())
		{
			LayerManager::getInstance().attachToLayerNode(_layer, this);
		}
		else if (!oldlayer.empty())
		{
			LayerManager::getInstance().attachToLayerNode(oldlayer, this);
		}

		// корректируем параметры
		float alpha = mAlpha;
		mAlpha = -1;
		setAlpha(alpha);

	}

	void Widget::attachToWidget(Widget* _parent, WidgetStyle _style, const std::string& _layer)
	{
		MYGUI_ASSERT(_parent, "parent must be valid");
		MYGUI_ASSERT(_parent != this, "cyclic attach (attaching to self)");

		// attach to client if widget have it
		if (_parent->getClientWidget()) _parent = _parent->getClientWidget();

		// проверяем на цикличность атача
		Widget* parent = _parent;
		while (parent->getParent())
		{
			MYGUI_ASSERT(parent != this, "cyclic attach");
			parent = parent->getParent();
		}

		// отдетачиваемся от всего
		detachFromWidget();

		mWidgetStyle = _style;

		if (_style == WidgetStyle::Popup)
		{
			mIWidgetCreator->_unlinkChildWidget(this);
			mIWidgetCreator = _parent;
			mParent = _parent;
			mParent->_linkChildWidget(this);

			mCroppedParent = nullptr;

			if (!_layer.empty())
			{
				LayerManager::getInstance().attachToLayerNode(_layer, this);
			}
		}
		else if (_style == WidgetStyle::Child)
		{
			LayerManager::getInstance().detachFromLayer(this);

			mIWidgetCreator->_unlinkChildWidget(this);
			mIWidgetCreator = _parent;
			mParent = _parent;
			mParent->_linkChildWidget(this);

			mCroppedParent = _parent;
			mAbsolutePosition = _parent->getAbsolutePosition() + mCoord.point();

			for (VectorWidgetPtr::iterator widget = mWidgetChild.begin(); widget != mWidgetChild.end(); ++widget) (*widget)->_updateAbsolutePoint();
			for (VectorWidgetPtr::iterator widget = mWidgetChildSkin.begin(); widget != mWidgetChildSkin.end(); ++widget) (*widget)->_updateAbsolutePoint();

			mParent->addChildItem(this);

			_updateView();
		}
		else if (_style == WidgetStyle::Overlapped)
		{
			LayerManager::getInstance().detachFromLayer(this);

			mIWidgetCreator->_unlinkChildWidget(this);
			mIWidgetCreator = _parent;
			mParent = _parent;
			mParent->_linkChildWidget(this);

			mCroppedParent = _parent;
			mAbsolutePosition = _parent->getAbsolutePosition() + mCoord.point();

			for (VectorWidgetPtr::iterator widget = mWidgetChild.begin(); widget != mWidgetChild.end(); ++widget) (*widget)->_updateAbsolutePoint();
			for (VectorWidgetPtr::iterator widget = mWidgetChildSkin.begin(); widget != mWidgetChildSkin.end(); ++widget) (*widget)->_updateAbsolutePoint();

			mParent->addChildNode(this);

			_updateView();
		}

		// корректируем параметры
		float alpha = mAlpha;
		mAlpha = -1;
		setAlpha(alpha);

	}

	void Widget::setWidgetStyle(WidgetStyle _style, const std::string& _layer)
	{
		if (_style == mWidgetStyle) return;
		if (nullptr == getParent()) return;

		Widget* parent = mParent;

		detachFromWidget();
		attachToWidget(parent, _style, _layer);
		// ищем леер к которому мы присоедененны
		/*Widget* root = this;
		while (!root->isRootWidget())
		{
			root = root->getParent();
		}

		// отсоединяем рут
		std::string layername;
		ILayer* layer = root->getLayer();
		if (layer)
		{
			layername = layer->getName();
			LayerManager::getInstance().detachFromLayer(root);

			// если мы рут, то придется отцеплят более высокого рута
			if (root == this)
			{
				layername.clear();

				if (getParent())
				{
					// ищем леер к которому мы присоедененны
					root = getParent();
					while (!root->isRootWidget())
					{
						root = root->getParent();
					}

					layer = root->getLayer();
					if (layer)
					{
						layername = layer->getName();
						LayerManager::getInstance().detachFromLayer(root);
					}

				}
			}
		}

		// корректируем
		mWidgetStyle = _style;
		if (_style == WidgetStyle::Child)
		{

			Widget* parent = getParent();
			if (parent)
			{
				mAbsolutePosition = parent->getAbsolutePosition() + mCoord.point();
				mCroppedParent = parent;
				for (VectorWidgetPtr::iterator widget = mWidgetChild.begin(); widget != mWidgetChild.end(); ++widget) (*widget)->_updateAbsolutePoint();
				for (VectorWidgetPtr::iterator widget = mWidgetChildSkin.begin(); widget != mWidgetChildSkin.end(); ++widget) (*widget)->_updateAbsolutePoint();
			}

		}
		else if (_style == WidgetStyle::Popup)
		{

			mCroppedParent = nullptr;
			// обновляем координаты
			mAbsolutePosition = mCoord.point();
			// сбрасываем обрезку
			mMargin.clear();

			for (VectorWidgetPtr::iterator widget = mWidgetChild.begin(); widget != mWidgetChild.end(); ++widget) (*widget)->_updateAbsolutePoint();
			for (VectorWidgetPtr::iterator widget = mWidgetChildSkin.begin(); widget != mWidgetChildSkin.end(); ++widget) (*widget)->_updateAbsolutePoint();

		}
		else if (_style == WidgetStyle::Overlapped)
		{

			Widget* parent = getParent();
			if (parent)
			{
				mAbsolutePosition = parent->getAbsolutePosition() + mCoord.point();
				mCroppedParent = parent;
				for (VectorWidgetPtr::iterator widget = mWidgetChild.begin(); widget != mWidgetChild.end(); ++widget) (*widget)->_updateAbsolutePoint();
				for (VectorWidgetPtr::iterator widget = mWidgetChildSkin.begin(); widget != mWidgetChildSkin.end(); ++widget) (*widget)->_updateAbsolutePoint();
			}

		}

		// присоединяем обратно
		if (!layername.empty())
		{
			LayerManager::getInstance().attachToLayerNode(layername, root);
		}*/

	}

	void Widget::setCaptionWithNewLine(const std::string& _value)
	{
		// change '\n' on char 10
		size_t pos = _value.find("\\n");
		if (pos == std::string::npos)
		{
			setCaption(LanguageManager::getInstance().replaceTags(_value));
		}
		else
		{
			std::string value(_value);
			while (pos != std::string::npos)
			{
				value[pos++] = '\n';
				value.erase(pos, 1);
				pos = value.find("\\n");
			}
			setCaption(LanguageManager::getInstance().replaceTags(value));
		}
	}

	Widget* Widget::createWidgetT(const std::string& _type, const std::string& _skin, const IntCoord& _coord, Align _align, const std::string& _name)
	{
		return baseCreateWidget(WidgetStyle::Child, _type, _skin, _coord, _align, "", _name);
	}

	Widget* Widget::createWidgetT(const std::string& _type, const std::string& _skin, int _left, int _top, int _width, int _height, Align _align, const std::string& _name)
	{
		return createWidgetT(_type, _skin, IntCoord(_left, _top, _width, _height), _align, _name);
	}

	Widget* Widget::createWidgetRealT(const std::string& _type, const std::string& _skin, float _left, float _top, float _width, float _height, Align _align, const std::string& _name)
	{
		return createWidgetRealT(_type, _skin, FloatCoord(_left, _top, _width, _height), _align, _name);
	}

	Widget* Widget::createWidgetT(WidgetStyle _style, const std::string& _type, const std::string& _skin, const IntCoord& _coord, Align _align, const std::string& _layer, const std::string& _name)
	{
		return baseCreateWidget(_style, _type, _skin, _coord, _align, _layer, _name);
	}

	EnumeratorWidgetPtr Widget::getEnumerator()
	{
		MYGUI_ASSERT(mWidgetClient != this, "mWidgetClient can not be this widget");
		if (mWidgetClient != nullptr) return mWidgetClient->getEnumerator();
		return Enumerator<VectorWidgetPtr>(mWidgetChild.begin(), mWidgetChild.end());
	}

	size_t Widget::getChildCount()
	{
		MYGUI_ASSERT(mWidgetClient != this, "mWidgetClient can not be this widget");
		if (mWidgetClient != nullptr) return mWidgetClient->getChildCount();
		return mWidgetChild.size();
	}

	Widget* Widget::getChildAt(size_t _index)
	{
		MYGUI_ASSERT(mWidgetClient != this, "mWidgetClient can not be this widget");
		if (mWidgetClient != nullptr) return mWidgetClient->getChildAt(_index);
		MYGUI_ASSERT_RANGE(_index, mWidgetChild.size(), "Widget::getChildAt");
		return mWidgetChild[_index];
	}

	const std::string& Widget::getPointer()
	{
		if (!mEnabled)
		{
			static std::string empty;
			return empty;
		}
		return mPointer;
	}

	void Widget::setProperty(const std::string& _key, const std::string& _value)
	{
		/// @wproperty{Widget, Widget_Caption, string} Sets caption
		if (_key == "Widget_Caption") setCaptionWithNewLine(_value);
		/// @wproperty{Widget, Widget_Position, IntPoint} Sets position
		else if (_key == "Widget_Position") setPosition(utility::parseValue<IntPoint>(_value));
		else if (_key == "Widget_Size") setSize(utility::parseValue<IntSize>(_value));
		else if (_key == "Widget_Coord") setCoord(utility::parseValue<IntCoord>(_value));
		else if (_key == "Widget_Visible") setVisible(utility::parseValue<bool>(_value));
		else if (_key == "Widget_Alpha") setAlpha(utility::parseValue<float>(_value));
		else if (_key == "Widget_Colour") setColour(utility::parseValue<Colour>(_value));
		else if (_key == "Widget_InheritsAlpha") setInheritsAlpha(utility::parseValue<bool>(_value));
		else if (_key == "Widget_InheritsPick") setInheritsPick(utility::parseValue<bool>(_value));
		else if (_key == "Widget_MaskPick") setMaskPick(_value);
		else if (_key == "Widget_State") setState(_value);
		else if (_key == "Widget_NeedKey") setNeedKeyFocus(utility::parseValue<bool>(_value));
		else if (_key == "Widget_NeedMouse") setNeedMouseFocus(utility::parseValue<bool>(_value));
		else if (_key == "Widget_Enabled") setEnabled(utility::parseValue<bool>(_value));
		else if (_key == "Widget_NeedToolTip") setNeedToolTip(utility::parseValue<bool>(_value));
		else if (_key == "Widget_Pointer") setPointer(_value);

#ifndef MYGUI_DONT_USE_OBSOLETE
		else if (_key == "Widget_TextColour")
		{
			MYGUI_LOG(Warning, "Widget_TextColour is obsolete, use Text_TextColour");
			_setTextColour(Colour::parse(_value));
		}
		else if (_key == "Widget_FontName")
		{
			MYGUI_LOG(Warning, "Widget_FontName is obsolete, use Text_FontName");
			_setFontName(_value);
		}
		else if (_key == "Widget_FontHeight")
		{
			MYGUI_LOG(Warning, "Widget_FontHeight is obsolete, use Text_FontHeight");
			this->_setFontHeight(utility::parseValue<int>(_value));
		}
		else if (_key == "Widget_TextAlign")
		{
			MYGUI_LOG(Warning, "Widget_TextAlign is obsolete, use Text_TextAlign");
			_setTextAlign(Align::parse(_value));
		}
		else if (_key == "Widget_AlignText")
		{
			MYGUI_LOG(Warning, "Widget_AlignText is obsolete, use Text_TextAlign");
			_setTextAlign(Align::parse(_value));
		}
		else if (_key == "Widget_Show")
		{
			MYGUI_LOG(Warning, "Widget_Show is obsolete, use Widget_Visible");
			setVisible(utility::parseValue<bool>(_value));
		}
		else if (_key == "Widget_InheritsPeek")
		{
			MYGUI_LOG(Warning, "Widget_InheritsPeek is obsolete, use Widget_InheritsPick");
			setInheritsPick(utility::parseValue<bool>(_value));
		}
		else if (_key == "Widget_MaskPeek")
		{
			MYGUI_LOG(Warning, "Widget_MaskPeek is obsolete, use Widget_MaskPick");
			setMaskPick(_value);
		}
#endif // MYGUI_DONT_USE_OBSOLETE

		else
		{
			MYGUI_LOG(Warning, "Property " << _key << " not found");
			return;
		}

		eventChangeProperty(this, _key, _value);
	}

	void Widget::baseUpdateEnable()
	{
		if (mEnabled)
		{
			setState("normal");
		}
		else
		{
			setState("disabled");
		}
	}

	void Widget::setVisible(bool _value)
	{
		if (mVisible == _value) return;
		mVisible = _value;

		if (mInheritsVisible)
		{
			for (VectorSubWidget::iterator skin = mSubSkinChild.begin(); skin != mSubSkinChild.end(); ++skin)
				(*skin)->setVisible(_value);
			for (VectorWidgetPtr::iterator widget = mWidgetChild.begin(); widget != mWidgetChild.end(); ++widget)
				(*widget)->_setInheritsVisible(_value);
			for (VectorWidgetPtr::iterator widget = mWidgetChildSkin.begin(); widget != mWidgetChildSkin.end(); ++widget)
				(*widget)->_setInheritsVisible(_value);
		}

	}

	void Widget::_setInheritsVisible(bool _value)
	{
		if (mInheritsVisible == _value) return;
		mInheritsVisible = _value;

		if (mVisible)
		{
			for (VectorSubWidget::iterator skin = mSubSkinChild.begin(); skin != mSubSkinChild.end(); ++skin)
				(*skin)->setVisible(_value);
			for (VectorWidgetPtr::iterator widget = mWidgetChild.begin(); widget != mWidgetChild.end(); ++widget)
				(*widget)->_setInheritsVisible(_value);
			for (VectorWidgetPtr::iterator widget = mWidgetChildSkin.begin(); widget != mWidgetChildSkin.end(); ++widget)
				(*widget)->_setInheritsVisible(_value);
		}
	}

	void Widget::setEnabled(bool _value)
	{
		if (mEnabled == _value) return;
		mEnabled = _value;

		if (mInheritsEnabled)
		{
			for (VectorWidgetPtr::iterator iter = mWidgetChild.begin(); iter != mWidgetChild.end(); ++iter)
				(*iter)->_setInheritsEnable(_value);
			for (VectorWidgetPtr::iterator iter = mWidgetChildSkin.begin(); iter != mWidgetChildSkin.end(); ++iter)
				(*iter)->_setInheritsEnable(_value);

			baseUpdateEnable();
		}

		if (!mEnabled)
		{
			InputManager::getInstance().unlinkWidget(this);
		}
	}

	void Widget::_setInheritsEnable(bool _value)
	{
		if (mInheritsEnabled == _value) return;
		mInheritsEnabled = _value;

		if (mEnabled)
		{
			for (VectorWidgetPtr::iterator iter = mWidgetChild.begin(); iter != mWidgetChild.end(); ++iter)
				(*iter)->_setInheritsEnable(_value);
			for (VectorWidgetPtr::iterator iter = mWidgetChildSkin.begin(); iter != mWidgetChildSkin.end(); ++iter)
				(*iter)->_setInheritsEnable(_value);

			baseUpdateEnable();
		}

		if (!mEnabled)
		{
			InputManager::getInstance().unlinkWidget(this);
		}
	}

	void Widget::setColour(const Colour& _value)
	{
		for (VectorSubWidget::iterator skin = mSubSkinChild.begin(); skin != mSubSkinChild.end(); ++skin)
		{
			ISubWidgetRect* rect = (*skin)->castType<ISubWidgetRect>(false);
			if (rect)
				rect->_setColour(_value);
		}
	}

} // namespace MyGUI
