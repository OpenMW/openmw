/*!
	@file
	@author		Albert Semenov
	@date		06/2009
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
#ifndef __MYGUI_I_POINTER_H__
#define __MYGUI_I_POINTER_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_IResource.h"

namespace MyGUI
{

	class MYGUI_EXPORT IPointer : public IResource
	{
		MYGUI_RTTI_DERIVED( IPointer )

	public:
		IPointer() { }
		virtual ~IPointer() { }

		virtual void setImage(StaticImage* _image) = 0;
		virtual void setPosition(StaticImage* _image, const IntPoint& _point) = 0;

	};

} // namespace MyGUI

#endif // __MYGUI_I_POINTER_H__
