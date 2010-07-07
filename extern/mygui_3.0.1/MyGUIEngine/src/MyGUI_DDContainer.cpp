/*!
	@file
	@author		Albert Semenov
	@date		10/2008
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
#include "MyGUI_DDContainer.h"
#include "MyGUI_InputManager.h"
#include "MyGUI_LayerManager.h"

namespace MyGUI
{

	DDContainer::DDContainer() :
		mDropResult(false),
		mNeedDrop(false),
		mStartDrop(false),
		mOldDrop(nullptr),
		mCurrentSender(nullptr),
		mDropSenderIndex(ITEM_NONE),
		mDropItem(nullptr),
		mNeedDragDrop(false),
		mReseiverContainer(nullptr)
	{
	}

	void DDContainer::_initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name)
	{
		Base::_initialise(_style, _coord, _align, _info, _parent, _croppedParent, _creator, _name);

		initialiseWidgetSkin(_info);
	}

	DDContainer::~DDContainer()
	{
		shutdownWidgetSkin();
	}

	void DDContainer::baseChangeWidgetSkin(ResourceSkin* _info)
	{
		shutdownWidgetSkin();
		Base::baseChangeWidgetSkin(_info);
		initialiseWidgetSkin(_info);
	}

	void DDContainer::initialiseWidgetSkin(ResourceSkin* _info)
	{
	}

	void DDContainer::shutdownWidgetSkin()
	{
	}

	void DDContainer::onMouseButtonPressed(int _left, int _top, MouseButton _id)
	{
		// смещение внутри виджета, куда кликнули мышкой
		mClickInWidget = InputManager::getInstance().getLastLeftPressed() - getAbsolutePosition();

		mouseButtonPressed(_id);

		Base::onMouseButtonPressed(_left, _top, _id);
	}

	void DDContainer::onMouseButtonReleased(int _left, int _top, MouseButton _id)
	{
		mouseButtonReleased(_id);

		Base::onMouseButtonReleased(_left, _top, _id);
	}

	void DDContainer::onMouseDrag(int _left, int _top)
	{
		mouseDrag();

		Base::onMouseDrag(_left, _top);
	}

	void DDContainer::mouseButtonPressed(MouseButton _id)
	{
		if (MouseButton::Left == _id)
		{
			// сбрасываем инфу для дропа
			mDropResult = false;
			mOldDrop = nullptr;
			mDropInfo.reset();
			mReseiverContainer = nullptr;

			// сбрасываем, чтобы обновился дропный виджет
			mCurrentSender = nullptr;
			mStartDrop = false;

		}
		// если нажата другая клавиша и был дроп то сбрасываем
		else
		{
			endDrop(true);
		}
	}

	void DDContainer::mouseButtonReleased(MouseButton _id)
	{
		if (MouseButton::Left == _id)
		{
			endDrop(false);
		}
	}

	void DDContainer::mouseDrag()
	{
		// нужно ли обновить данные
		bool update = false;

		// первый раз дропаем елемент
		if (!mStartDrop && mDropSenderIndex != ITEM_NONE)
		{
			mStartDrop = true;
			mNeedDrop = false;
			update = true;
			// запрос на нужность дропа по индексу
			mDropInfo.set(this, mDropSenderIndex, nullptr, ITEM_NONE);
			mReseiverContainer = nullptr;

			eventStartDrag(this, mDropInfo, mNeedDrop);

			if (mNeedDrop)
			{
				eventChangeDDState(this, DDItemState::Start);
				setEnableToolTip(false);
			}
			else
			{
				// сбрасываем фокус мыши (не обязательно)
				InputManager::getInstance().resetMouseCaptureWidget();
			}
		}

		// дроп не нужен
		if (!mNeedDrop)
		{
			return;
		}

		// делаем запрос, над кем наша мыша
		const IntPoint& point = InputManager::getInstance().getMousePosition();
		Widget* item = LayerManager::getInstance().getWidgetFromPoint(point.left, point.top);

		updateDropItems();

		// если равно, значит уже спрашивали
		if (mOldDrop == item) return;
		mOldDrop = item;

		// сбрасываем старую подсветку
		if (mReseiverContainer) mReseiverContainer->_setContainerItemInfo(mDropInfo.receiver_index, false, false);

		mDropResult = false;
		mReseiverContainer = nullptr;
		Widget* receiver = nullptr;
		size_t receiver_index = ITEM_NONE;
		// есть виджет под нами
		if (item)
		{
			// делаем запрос на индекс по произвольному виджету
			item->_getContainer(receiver, receiver_index);
			// работаем только с контейнерами
			if (receiver && receiver->isType<DDContainer>())
			{
				// подписываемся на информацию о валидности дропа
				mReseiverContainer = static_cast<DDContainer*>(receiver);
				mReseiverContainer->_eventInvalideContainer = newDelegate(this, &DDContainer::notifyInvalideDrop);

				// делаем запрос на возможность дропа
				mDropInfo.set(this, mDropSenderIndex, mReseiverContainer, receiver_index);

				eventRequestDrop(this, mDropInfo, mDropResult);

				// устанавливаем новую подсветку
				mReseiverContainer->_setContainerItemInfo(mDropInfo.receiver_index, true, mDropResult);
			}
			else
			{
				mDropInfo.set(this, mDropSenderIndex, nullptr, ITEM_NONE);
			}
		}
		// нет виджета под нами
		else
		{
			mDropInfo.set(this, mDropSenderIndex, nullptr, ITEM_NONE);
		}

		DDItemState state;

		DDWidgetState data(mDropSenderIndex);
		data.update = update;

		if (receiver == nullptr)
		{
			data.accept = false;
			data.refuse = false;
			state = DDItemState::Miss;
		}
		else if (mDropResult)
		{
			data.accept = true;
			data.refuse = false;
			state = DDItemState::Accept;
		}
		else
		{
			data.accept = false;
			data.refuse = true;
			state = DDItemState::Refuse;
		}

		updateDropItemsState(data);

		eventChangeDDState(this, state);
	}

	void DDContainer::endDrop(bool _reset)
	{
		if (mStartDrop)
		{
			removeDropItems();

			// сбрасываем старую подсветку
			if (mReseiverContainer) mReseiverContainer->_setContainerItemInfo(mDropInfo.receiver_index, false, false);

			if (_reset) mDropResult = false;
			eventDropResult(this, mDropInfo, mDropResult);
			eventChangeDDState(this, DDItemState::End);
			setEnableToolTip(true);

			// сбрасываем инфу для дропа
			mStartDrop = false;
			mDropResult = false;
			mNeedDrop = false;
			mOldDrop = nullptr;
			mDropInfo.reset();
			mReseiverContainer = nullptr;
			mDropSenderIndex = ITEM_NONE;
		}
	}

	void DDContainer::removeDropItems()
	{
		mDropItem = nullptr;
	}

	void DDContainer::updateDropItems()
	{

		if (mDropItem == nullptr)
		{
			requestDragWidgetInfo(this, mDropItem, mDropDimension);
		}

		const IntPoint& point = InputManager::getInstance().getMousePositionByLayer();

		if (mDropItem)
		{
			mDropItem->setCoord(point.left - mClickInWidget.left + mDropDimension.left, point.top - mClickInWidget.top + mDropDimension.top, mDropDimension.width, mDropDimension.height);
			mDropItem->setVisible(true);
		}
	}

	void DDContainer::updateDropItemsState(const DDWidgetState& _state)
	{
		eventUpdateDropState(this, mDropItem, _state);
	}

	void DDContainer::notifyInvalideDrop(DDContainer* _sender)
	{
		mouseDrag();
	}

	void DDContainer::_getContainer(Widget*& _container, size_t& _index)
	{
		_container = this;
		_index = ITEM_NONE;
	}

	void DDContainer::setProperty(const std::string& _key, const std::string& _value)
	{
		if (_key == "DDContainer_NeedDragDrop") setNeedDragDrop(utility::parseValue<bool>(_value));
		else
		{
			Base::setProperty(_key, _value);
			return;
		}
		eventChangeProperty(this, _key, _value);
	}

} // namespace MyGUI
