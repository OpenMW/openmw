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
#ifndef __MYGUI_PROGRESS_H__
#define __MYGUI_PROGRESS_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Align.h"
#include "MyGUI_Widget.h"

namespace MyGUI
{

	class MYGUI_EXPORT Progress :
		public Widget
	{
		MYGUI_RTTI_DERIVED( Progress )

	public:
		Progress();

		/** Set progress range */
		void setProgressRange(size_t _value);
		/** Get progress range */
		size_t getProgressRange() { return mRange; }

		/** Set progress position */
		void setProgressPosition(size_t _value);
		/** Get progress position */
		size_t getProgressPosition() { return mEndPosition; }

		/** Enable or disable progress auto tracking */
		void setProgressAutoTrack(bool _value);
		/** Get progress auto tracking flag */
		bool getProgressAutoTrack() { return mAutoTrack; }

		/** Set progress start point
			For example with Align::Top if will be filled from top to bottom.
		*/
		void setProgressStartPoint(Align _value);
		/** Get progress start point */
		Align getProgressStartPoint() { return mStartPoint; }

		//! @copydoc Widget::setPosition(const IntPoint& _value)
		virtual void setPosition(const IntPoint& _value);
		//! @copydoc Widget::setSize(const IntSize& _value)
		virtual void setSize(const IntSize& _value);
		//! @copydoc Widget::setCoord(const IntCoord& _value)
		virtual void setCoord(const IntCoord& _value);

		/** @copydoc Widget::setPosition(int _left, int _top) */
		void setPosition(int _left, int _top) { setPosition(IntPoint(_left, _top)); }
		/** @copydoc Widget::setSize(int _width, int _height) */
		void setSize(int _width, int _height) { setSize(IntSize(_width, _height)); }
		/** @copydoc Widget::setCoord(int _left, int _top, int _width, int _height) */
		void setCoord(int _left, int _top, int _width, int _height) { setCoord(IntCoord(_left, _top, _width, _height)); }

		/** @copydoc Widget::setProperty(const std::string& _key, const std::string& _value) */
		virtual void setProperty(const std::string& _key, const std::string& _value);

	/*obsolete:*/
#ifndef MYGUI_DONT_USE_OBSOLETE

		MYGUI_OBSOLETE("use : void Widget::setCoord(const IntCoord& _coord)")
		void setPosition(const IntCoord& _coord) { setCoord(_coord); }
		MYGUI_OBSOLETE("use : void Widget::setCoord(int _left, int _top, int _width, int _height)")
		void setPosition(int _left, int _top, int _width, int _height) { setCoord(_left, _top, _width, _height); }

#endif // MYGUI_DONT_USE_OBSOLETE

	/*internal:*/
		virtual void _initialise(WidgetStyle _style, const IntCoord& _coord, Align _align, ResourceSkin* _info, Widget* _parent, ICroppedRectangle * _croppedParent, IWidgetCreator * _creator, const std::string& _name);

	protected:
		virtual ~Progress();

		void baseChangeWidgetSkin(ResourceSkin* _info);

	private:
		void initialiseWidgetSkin(ResourceSkin* _info);
		void shutdownWidgetSkin();

		void frameEntered(float _time);
		void updateTrack();

		int getClientWidth() { return ((mStartPoint.isLeft()) || (mStartPoint.isRight())) ? mClient->getWidth() : mClient->getHeight(); }
		int getClientHeight() { return ((mStartPoint.isLeft()) || (mStartPoint.isRight())) ? mClient->getHeight() : mClient->getWidth(); }

		void setTrackPosition(Widget* _widget, int _left, int _top, int _width, int _height);

	private:
		std::string mTrackSkin;
		int mTrackWidth;
		int mTrackStep;
		int mTrackMin;

		VectorWidgetPtr mVectorTrack;
		size_t mRange;
		size_t mStartPosition, mEndPosition;
		float mAutoPosition;
		bool mAutoTrack;
		bool mFillTrack;

		Align mStartPoint;

		Widget* mClient;

	};

} // namespace MyGUI

#endif // __MYGUI_PROGRESS_H__
