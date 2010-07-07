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
#include "MyGUI_Progress.h"
#include "MyGUI_ResourceSkin.h"
#include "MyGUI_Widget.h"
#include "MyGUI_Gui.h"
#include "MyGUI_SkinManager.h"

namespace MyGUI
{

	const size_t PROGRESS_AUTO_WIDTH = 200;
	const size_t PROGRESS_AUTO_RANGE = 1000;
	const float PROGRESS_AUTO_COEF = 400;

	Progress::Progress() :
		mTrackWidth(1),
		mTrackStep(0),
		mTrackMin(0),
		mRange(0),
		mStartPosition(0),
		mEndPosition(0),
		mAutoPosition(0.0f),
		mAutoTrack(false),
		mFillTrack(false),
		mStartPoint(Align::Left),
		mClient(nullptr)
	{
	}

	void Progress::_initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name)
	{
		Base::_initialise(_style, _coord, _align, _info, _parent, _croppedParent, _creator, _name);

		initialiseWidgetSkin(_info);
	}

	Progress::~Progress()
	{
		shutdownWidgetSkin();
	}

	void Progress::baseChangeWidgetSkin(ResourceSkin* _info)
	{
		shutdownWidgetSkin();
		Base::baseChangeWidgetSkin(_info);
		initialiseWidgetSkin(_info);
	}

	void Progress::initialiseWidgetSkin(ResourceSkin* _info)
	{
		for (VectorWidgetPtr::iterator iter=mWidgetChildSkin.begin(); iter!=mWidgetChildSkin.end(); ++iter)
		{
			if (*(*iter)->_getInternalData<std::string>() == "Client")
			{
				MYGUI_DEBUG_ASSERT( ! mClient, "widget already assigned");
				mClient = (*iter);
			}
		}
		if (nullptr == mClient) mClient = this;

		const MapString& properties = _info->getProperties();
		MapString::const_iterator iterS = properties.find("TrackSkin");
		if (iterS != properties.end()) mTrackSkin = iterS->second;
		iterS = properties.find("TrackWidth");
		if (iterS != properties.end()) mTrackWidth = utility::parseInt(iterS->second);
		iterS = properties.find("TrackMin");
		if (iterS != properties.end()) mTrackMin = utility::parseInt(iterS->second);
		if (1 > mTrackWidth) mTrackWidth = 1;
		iterS = properties.find("TrackStep");
		if (iterS != properties.end()) mTrackStep = utility::parseInt(iterS->second);
		else mTrackStep = mTrackWidth;
		iterS = properties.find("TrackFill");
		if (iterS != properties.end()) mFillTrack = utility::parseBool(iterS->second);
		iterS = properties.find("StartPoint");
		if (iterS != properties.end()) setProgressStartPoint(Align::parse(iterS->second));

	}

	void Progress::shutdownWidgetSkin()
	{
		mClient = nullptr;
	}

	void Progress::setProgressRange(size_t _range)
	{
		if (mAutoTrack) return;
		mRange = _range;
		if (mEndPosition > mRange) mEndPosition = mRange;
		if (mStartPosition > mRange) mStartPosition = mRange;
		updateTrack();
	}

	void Progress::setProgressPosition(size_t _pos)
	{
		if (mAutoTrack) return;
		mEndPosition = _pos;
		if (mEndPosition > mRange) mEndPosition = mRange;
		updateTrack();
	}

	void Progress::setProgressAutoTrack(bool _auto)
	{
		if (mAutoTrack == _auto) return;
		mAutoTrack = _auto;

		if (mAutoTrack)
		{
			Gui::getInstance().eventFrameStart += newDelegate(this, &Progress::frameEntered);
			mRange = PROGRESS_AUTO_RANGE;
			mEndPosition = mStartPosition = 0;
			mAutoPosition = 0.0f;
		}
		else
		{
			Gui::getInstance().eventFrameStart -= newDelegate(this, &Progress::frameEntered);
			mRange = mEndPosition = mStartPosition = 0;
		}
		updateTrack();
	}

	void Progress::frameEntered(float _time)
	{
		if (!mAutoTrack) return;
		mAutoPosition += (PROGRESS_AUTO_COEF * _time);
		size_t pos = (size_t)mAutoPosition;

		if (pos > (mRange + PROGRESS_AUTO_WIDTH)) mAutoPosition = 0.0f;

		if (pos > mRange) mEndPosition = mRange;
		else mEndPosition = size_t(mAutoPosition);

		if (pos < PROGRESS_AUTO_WIDTH) mStartPosition = 0;
		else mStartPosition = pos - PROGRESS_AUTO_WIDTH;

		updateTrack();
	}

	void Progress::setPosition(const IntPoint& _point)
	{
		Base::setPosition(_point);
	}

	void Progress::setSize(const IntSize& _size)
	{
		updateTrack();

		Base::setSize(_size);
	}

	void Progress::setCoord(const IntCoord& _coord)
	{
		updateTrack();

		Base::setCoord(_coord);
	}

	void Progress::updateTrack()
	{
		// все скрыто
		if ((0 == mRange) || (0 == mEndPosition))
		{
			for (VectorWidgetPtr::iterator iter=mVectorTrack.begin(); iter!=mVectorTrack.end(); ++iter)
			{
				(*iter)->setVisible(false);
			}
			return;
		}

		// тут попроще расчеты
		if (mFillTrack)
		{
			if (mVectorTrack.empty())
			{
				Widget* widget = mClient->createWidget<Widget>(mTrackSkin, IntCoord(), Align::Left | Align::VStretch);
				mVectorTrack.push_back(widget);
			}
			else
			{
				// первый показываем и ставим норм альфу
				VectorWidgetPtr::iterator iter=mVectorTrack.begin();
				(*iter)->setVisible(true);
				(*iter)->setAlpha(ALPHA_MAX);

				// все начиная со второго скрываем
				++iter;
				for (; iter!=mVectorTrack.end(); ++iter)
				{
					(*iter)->setVisible(false);
				}
			}

			Widget* wid = mVectorTrack.front();

			// полностью виден
			if ((0 == mStartPosition) && (mRange == mEndPosition))
			{
				setTrackPosition(wid, 0, 0, getClientWidth(), getClientHeight());
			}
			// эх
			else
			{
				int pos = (int)mStartPosition * (getClientWidth() - mTrackMin) / (int)mRange;
				setTrackPosition(wid, pos, 0, ((int)mEndPosition * (getClientWidth() - mTrackMin) / (int)mRange) - pos + mTrackMin, getClientHeight());
			}

			return;
		}

		// сначала проверяем виджеты для трека
		int width = getClientWidth() - mTrackWidth + mTrackStep;
		int count = width / mTrackStep;
		int ost = (width % mTrackStep);
		if (ost > 0)
		{
			width += mTrackStep - ost;
			count ++;
		}

		while ((int)mVectorTrack.size() < count)
		{
			Widget* widget = mClient->createWidget<Widget>(mTrackSkin, IntCoord(/*(int)mVectorTrack.size() * mTrackStep, 0, mTrackWidth, getClientHeight()*/), Align::Left | Align::VStretch);
			widget->setVisible(false);
			mVectorTrack.push_back(widget);
		}

		// все видно
		if ((0 == mStartPosition) && (mRange == mEndPosition))
		{
			int pos = 0;
			for (VectorWidgetPtr::iterator iter=mVectorTrack.begin(); iter!=mVectorTrack.end(); ++iter)
			{
				(*iter)->setAlpha(ALPHA_MAX);
				(*iter)->setVisible(true);
				setTrackPosition(*iter, pos * mTrackStep, 0, mTrackWidth, getClientHeight());
				pos++;
			}
		}
		// эх, придется считать
		else
		{
			// сколько не видно
			int hide_pix = (width * (int)mStartPosition / (int)mRange);
			int hide_count = hide_pix / mTrackStep;
			// сколько видно
			int show_pix = (width * (int)mEndPosition / (int)mRange);
			int show_count = show_pix / mTrackStep;

			int pos = 0;
			for (VectorWidgetPtr::iterator iter=mVectorTrack.begin(); iter!=mVectorTrack.end(); ++iter)
			{
				if (0 > show_count)
				{
					(*iter)->setVisible(false);
				}
				else if (0 == show_count)
				{
					(*iter)->setAlpha((float)(show_pix % mTrackStep) / (float)mTrackStep);
					(*iter)->setVisible(true);
					setTrackPosition(*iter, pos * mTrackStep, 0, mTrackWidth, getClientHeight());
				}
				else
				{
					if (0 < hide_count)
					{
						(*iter)->setVisible(false);
					}
					else if (0 == hide_count)
					{
						(*iter)->setAlpha(1.0f - ((float)(hide_pix % mTrackStep) / (float)mTrackStep));
						(*iter)->setVisible(true);
						setTrackPosition(*iter, pos * mTrackStep, 0, mTrackWidth, getClientHeight());
					}
					else
					{
						(*iter)->setAlpha(ALPHA_MAX);
						(*iter)->setVisible(true);
						setTrackPosition(*iter, pos * mTrackStep, 0, mTrackWidth, getClientHeight());
					}
				}
				hide_count --;
				show_count --;
				pos ++;
			}
		}
	}

	void Progress::setTrackPosition(Widget* _widget, int _left, int _top, int _width, int _height)
	{
		if (mStartPoint.isLeft()) _widget->setCoord(_left, _top, _width, _height);
		else if (mStartPoint.isRight()) _widget->setCoord(mClient->getWidth() - _left - _width, _top, _width, _height);
		else if (mStartPoint.isTop()) _widget->setCoord(_top, _left, _height, _width);
		else if (mStartPoint.isBottom()) _widget->setCoord(_top, mClient->getHeight() - _left - _width, _height, _width);
	}

	void Progress::setProgressStartPoint(Align _align)
	{
		if ((_align == Align::Left) || (_align == Align::Right) || (_align == Align::Top) || (_align == Align::Bottom))
		{
			mStartPoint = _align;
		}
		else
		{
			mStartPoint = Align::Left;
			MYGUI_LOG(Warning, "Progress bar support only Left, Right, Top or Bottom align values");
		}
		updateTrack();
	}

	void Progress::setProperty(const std::string& _key, const std::string& _value)
	{
		if (_key == "Progress_Range") setProgressRange(utility::parseValue<size_t>(_value));
		else if (_key == "Progress_Position") setProgressPosition(utility::parseValue<size_t>(_value));
		else if (_key == "Progress_AutoTrack") setProgressAutoTrack(utility::parseValue<bool>(_value));
		else if (_key == "Progress_StartPoint") setProgressStartPoint(utility::parseValue<Align>(_value));
		else
		{
			Base::setProperty(_key, _value);
			return;
		}
		eventChangeProperty(this, _key, _value);
	}

} // namespace MyGUI
