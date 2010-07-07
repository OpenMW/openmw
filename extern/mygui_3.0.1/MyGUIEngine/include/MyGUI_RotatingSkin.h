/*!
	@file
	@author		George Evmenov
	@date		05/2009
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
#ifndef __MYGUI_ROTATING_SKIN_H__
#define __MYGUI_ROTATING_SKIN_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_XmlDocument.h"
#include "MyGUI_Types.h"
#include "MyGUI_ICroppedRectangle.h"
#include "MyGUI_SubSkin.h"

namespace MyGUI
{

	class MYGUI_EXPORT RotatingSkin : public SubSkin
	{
		MYGUI_RTTI_DERIVED( RotatingSkin )

	public:
		RotatingSkin();
		virtual ~RotatingSkin();

		/** Set angle of rotation */
		void setAngle(float _angle);
		/** Get angle of rotation */
		float getAngle() const { return mAngle; }

		/** Set center of rotation
			@param _center Center point.
			@param _local If true - _center point calculated as point on SubWidget, else calculated as point on screen.
		*/
		void setCenter(const IntPoint &_center, bool _local = true);
		/** Get center of rotation */
		IntPoint getCenter(bool _local = true) const;

		// метод для отрисовки себя
		virtual void doRender();

	protected:
		void recalculateAngles();

	private:
		float mAngle;
		IntPoint mCenterPos;
		bool mLocalCenter;

		/*
			0 3
			1 2
		*/
		float mBaseAngles[4];
		float mBaseDistances[4];
	};

} // namespace MyGUI

#endif // __MYGUI_ROTATING_SKIN_H__
