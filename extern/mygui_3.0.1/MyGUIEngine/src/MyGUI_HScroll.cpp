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
#include "MyGUI_HScroll.h"
#include "MyGUI_InputManager.h"
#include "MyGUI_Button.h"
#include "MyGUI_ResourceSkin.h"

namespace MyGUI
{

	HScroll::HScroll()
	{
	}

	void HScroll::_initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name)
	{
		Base::_initialise(_style, _coord, _align, _info, _parent, _croppedParent, _creator, _name);

		initialiseWidgetSkin(_info);
	}

	HScroll::~HScroll()
	{
		shutdownWidgetSkin();
	}

	void HScroll::baseChangeWidgetSkin(ResourceSkin* _info)
	{
		shutdownWidgetSkin();
		Base::baseChangeWidgetSkin(_info);
		initialiseWidgetSkin(_info);
	}

	void HScroll::initialiseWidgetSkin(ResourceSkin* _info)
	{
	}

	void HScroll::shutdownWidgetSkin()
	{
	}

	void HScroll::updateTrack()
	{
		if (mWidgetTrack == nullptr)
			return;

		_forcePeek(mWidgetTrack);
		// размер диапазана в пикселях
		int pos = getLineSize();

		// скрываем если диапазан маленький или места мало
		if ((mScrollRange < 2) || (pos <= mWidgetTrack->getWidth()))
		{
			mWidgetTrack->setVisible(false);
			if ( nullptr != mWidgetFirstPart ) mWidgetFirstPart->setSize(pos/2, mWidgetFirstPart->getHeight());
			if ( nullptr != mWidgetSecondPart ) mWidgetSecondPart->setCoord(pos/2 + mSkinRangeStart, mWidgetSecondPart->getTop(), pos - pos/2, mWidgetSecondPart->getHeight());
			return;
		}
		// если скрыт то покажем
		if (!mWidgetTrack->isVisible())
		{
			mWidgetTrack->setVisible(true);
		}

		// и обновляем позицию
		pos = (int)(((size_t)(pos-getTrackSize()) * mScrollPosition) / (mScrollRange-1) + mSkinRangeStart);

		mWidgetTrack->setPosition(pos, mWidgetTrack->getTop());
		if ( nullptr != mWidgetFirstPart )
		{
			int height = pos + mWidgetTrack->getWidth()/2 - mWidgetFirstPart->getLeft();
			mWidgetFirstPart->setSize(height, mWidgetFirstPart->getHeight());
		}
		if ( nullptr != mWidgetSecondPart )
		{
			int top = pos + mWidgetTrack->getWidth()/2;
			int height = mWidgetSecondPart->getWidth() + mWidgetSecondPart->getLeft() - top;
			mWidgetSecondPart->setCoord(top, mWidgetSecondPart->getTop(), height, mWidgetSecondPart->getHeight());
		}
	}

	void HScroll::TrackMove(int _left, int _top)
	{
		if (mWidgetTrack == nullptr)
			return;

		const IntPoint& point = InputManager::getInstance().getLastLeftPressed();

		// расчитываем позицию виджета
		int start = mPreActionOffset.left + (_left - point.left);
		if (start < (int)mSkinRangeStart) start = (int)mSkinRangeStart;
		else if (start > (mCoord.width - (int)mSkinRangeEnd - mWidgetTrack->getWidth())) start = (mCoord.width - (int)mSkinRangeEnd - mWidgetTrack->getWidth());
		if (mWidgetTrack->getLeft() != start) mWidgetTrack->setPosition(IntPoint(start, mWidgetTrack->getTop()));

		// расчитываем положение соответствующее позиции
		// плюс пол позиции
		int pos = start - (int)mSkinRangeStart + (getLineSize() - getTrackSize()) / (((int)mScrollRange-1) * 2);
		// высчитываем ближайшее значение и обновляем
		pos = pos * (int)(mScrollRange-1) / (getLineSize() - getTrackSize());

		// проверяем на выходы и изменения
		if (pos < 0) pos = 0;
		else if (pos >= (int)mScrollRange) pos = (int)mScrollRange - 1;
		if (pos == (int)mScrollPosition) return;

		mScrollPosition = pos;
		// отсылаем событие
		eventScrollChangePosition(this, (int)mScrollPosition);
	}

	void HScroll::setTrackSize(int _size)
	{
		if (mWidgetTrack != nullptr)
			mWidgetTrack->setSize(((int)_size < (int)mMinTrackSize)? (int)mMinTrackSize : (int)_size, mWidgetTrack->getHeight());
		updateTrack();
	}

	int HScroll::getTrackSize()
	{
		return mWidgetTrack == nullptr ? 1 : mWidgetTrack->getWidth();
	}

	int HScroll::getLineSize()
	{
		return mCoord.width - (int)(mSkinRangeStart + mSkinRangeEnd);
	}

} // namespace MyGUI
