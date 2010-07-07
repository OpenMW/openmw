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
#include "MyGUI_ScrollViewBase.h"
#include "MyGUI_VScroll.h"
#include "MyGUI_HScroll.h"

namespace MyGUI
{

	ScrollViewBase::ScrollViewBase() :
		mVScroll(nullptr),
		mHScroll(nullptr),
		mClient(nullptr),
		mVisibleHScroll(true),
		mVisibleVScroll(true),
		mVRange(0),
		mHRange(0),
		mChangeContentByResize(false)
	{
	}

	void ScrollViewBase::updateScrollSize()
	{
		if (mClient == nullptr)
			return;

		eraseContent();
		IntSize contentSize = getContentSize();
		IntSize viewSize = getViewSize();

		// вертикальный контент не помещается
		if (contentSize.height > viewSize.height)
		{
			if (mVScroll != nullptr)
			{
				if (( ! mVScroll->isVisible()) && (mVisibleVScroll))
				{
					mVScroll->setVisible(true);
					mClient->setSize(mClient->getWidth() - mVScroll->getWidth(), mClient->getHeight());

					// размер может измениться
					if (mChangeContentByResize)
					{
						eraseContent();
						contentSize = getContentSize();
						viewSize = getViewSize();
					}

					if (mHScroll != nullptr)
					{
						mHScroll->setSize(mHScroll->getWidth() - mVScroll->getWidth(), mHScroll->getHeight());

						// если показали вертикальный скрол бар, уменьшилось вью по горизонтали,
						// пересчитываем горизонтальный скрол на предмет показа
						if ((contentSize.width > viewSize.width) && ( ! mHScroll->isVisible()) && (mVisibleHScroll))
						{
							mHScroll->setVisible(true);
							mClient->setSize(mClient->getWidth(), mClient->getHeight() - mHScroll->getHeight());
							mVScroll->setSize(mVScroll->getWidth(), mVScroll->getHeight() - mHScroll->getHeight());

							// размер может измениться
							if (mChangeContentByResize)
							{
								eraseContent();
								contentSize = getContentSize();
								viewSize = getViewSize();
							}

						}
					}
				}
			}
		}
		// вертикальный контент помещается
		else
		{
			if (mVScroll != nullptr)
			{
				if (mVScroll->isVisible())
				{
					mVScroll->setVisible(false);
					mClient->setSize(mClient->getWidth() + mVScroll->getWidth(), mClient->getHeight());

					// размер может измениться
					if (mChangeContentByResize)
					{
						eraseContent();
						contentSize = getContentSize();
						viewSize = getViewSize();
					}

					if (mHScroll != nullptr)
					{
						mHScroll->setSize(mHScroll->getWidth() + mVScroll->getWidth(), mHScroll->getHeight());

						// если скрыли вертикальный скрол бар, увеличилось вью по горизонтали,
						// пересчитываем горизонтальный скрол на предмет скрытия
						if ((contentSize.width <= viewSize.width) && (mHScroll->isVisible()))
						{
							mHScroll->setVisible(false);
							mClient->setSize(mClient->getWidth(), mClient->getHeight() + mHScroll->getHeight());
							mVScroll->setSize(mVScroll->getWidth(), mVScroll->getHeight() + mHScroll->getHeight());

							// размер может измениться
							if (mChangeContentByResize)
							{
								eraseContent();
								contentSize = getContentSize();
								viewSize = getViewSize();
							}

						}
					}
				}
			}
		}


		// горизонтальный контент не помещается
		if (contentSize.width > viewSize.width)
		{
			if (mHScroll != nullptr)
			{
				if (( ! mHScroll->isVisible()) && (mVisibleHScroll))
				{
					mHScroll->setVisible(true);
					mClient->setSize(mClient->getWidth(), mClient->getHeight() - mHScroll->getHeight());

					// размер может измениться
					if (mChangeContentByResize)
					{
						eraseContent();
						contentSize = getContentSize();
						viewSize = getViewSize();
					}

					if (mVScroll != nullptr)
					{
						mVScroll->setSize(mVScroll->getWidth(), mVScroll->getHeight() - mHScroll->getHeight());

						// если показали горизонтальный скрол бар, уменьшилось вью по вертикали,
						// пересчитываем вертикальный скрол на предмет показа
						if ((contentSize.height > viewSize.height) && ( ! mVScroll->isVisible()) && (mVisibleVScroll))
						{
							mVScroll->setVisible(true);
							mClient->setSize(mClient->getWidth() - mVScroll->getWidth(), mClient->getHeight());
							mHScroll->setSize(mHScroll->getWidth() - mVScroll->getWidth(), mHScroll->getHeight());

							// размер может измениться
							if (mChangeContentByResize)
							{
								eraseContent();
								contentSize = getContentSize();
								viewSize = getViewSize();
							}

						}
					}
				}
			}
		}
		// горизонтальный контент помещается
		else
		{
			if (mHScroll != nullptr)
			{
				if (mHScroll->isVisible())
				{
					mHScroll->setVisible(false);
					mClient->setSize(mClient->getWidth(), mClient->getHeight() + mHScroll->getHeight());

					// размер может измениться
					if (mChangeContentByResize)
					{
						eraseContent();
						contentSize = getContentSize();
						viewSize = getViewSize();
					}

					if (mVScroll != nullptr)
					{
						mVScroll->setSize(mVScroll->getWidth(), mVScroll->getHeight() + mHScroll->getHeight());

						// если скрыли горизонтальный скрол бар, увеличилось вью по вертикали,
						// пересчитываем вертикальный скрол на предмет скрытия
						if ((contentSize.height <= viewSize.height) && (mVScroll->isVisible()))
						{
							mVScroll->setVisible(false);
							mClient->setSize(mClient->getWidth() + mVScroll->getWidth(), mClient->getHeight());
							mHScroll->setSize(mHScroll->getWidth() + mVScroll->getWidth(), mHScroll->getHeight());

							// размер может измениться
							if (mChangeContentByResize)
							{
								eraseContent();
								contentSize = getContentSize();
								viewSize = getViewSize();
							}
						}
					}
				}
			}
		}

		mVRange = (viewSize.height >= contentSize.height) ? 0 : contentSize.height - viewSize.height;
		mHRange = (viewSize.width >= contentSize.width) ? 0 : contentSize.width - viewSize.width;

		if (mVScroll != nullptr)
		{
			size_t page = getVScrollPage();
			mVScroll->setScrollPage(page);
			mVScroll->setScrollViewPage(viewSize.width > (int)page ? viewSize.width : page);
			mVScroll->setScrollRange(mVRange + 1);
			if (contentSize.height) mVScroll->setTrackSize(int (float(mVScroll->getLineSize() * viewSize.height) / float(contentSize.height)));
		}
		if (mHScroll != nullptr)
		{
			size_t page = getHScrollPage();
			mHScroll->setScrollPage(page);
			mHScroll->setScrollViewPage(viewSize.height > (int)page ? viewSize.height : page);
			mHScroll->setScrollRange(mHRange + 1);
			if (contentSize.width) mHScroll->setTrackSize(int (float(mHScroll->getLineSize() * viewSize.width) / float(contentSize.width)));
		}
	}

	void ScrollViewBase::updateScrollPosition()
	{
		// размер контекста
		IntSize contentSize = getContentSize();
		// текущее смещение контекста
		IntPoint contentPoint = getContentPosition();
		// расчетное смещение
		IntPoint offset = contentPoint;

		IntSize viewSize = getViewSize();

		Align align = getContentAlign();

		if (contentSize.width > viewSize.width)
		{
			// максимальный выход влево
			if ((offset.left + viewSize.width) > contentSize.width)
			{
				offset.left = contentSize.width - viewSize.width;
			}
			// максимальный выход вправо
			else if (offset.left < 0)
			{
				offset.left = 0;
			}
		}
		else
		{
			if (align.isLeft())
			{
				offset.left = 0;
			}
			else if (align.isRight())
			{
				offset.left = contentSize.width - viewSize.width;
			}
			else
			{
				offset.left = (contentSize.width - viewSize.width) / 2;
			}
		}

		if (contentSize.height > viewSize.height)
		{
			// максимальный выход вверх
			if ((offset.top + viewSize.height) > contentSize.height)
			{
				offset.top = contentSize.height - viewSize.height;
			}
			// максимальный выход вниз
			else if (offset.top < 0)
			{
				offset.top = 0;
			}
		}
		else
		{
			if (align.isTop())
			{
				offset.top = 0;
			}
			else if (align.isBottom())
			{
				offset.top = contentSize.height - viewSize.height;
			}
			else
			{
				offset.top = (contentSize.height - viewSize.height) / 2;
			}
		}

		if (offset != contentPoint)
		{
			if (nullptr != mVScroll) mVScroll->setScrollPosition(offset.top);
			if (nullptr != mHScroll) mHScroll->setScrollPosition(offset.left);
			setContentPosition(offset);
		}
	}

} // namespace MyGUI
