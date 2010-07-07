/*!
	@file
	@author		Albert Semenov
	@date		09/2008
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
#ifndef __MYGUI_RAW_RECT_H__
#define __MYGUI_RAW_RECT_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Colour.h"
#include "MyGUI_XmlDocument.h"
#include "MyGUI_Types.h"
#include "MyGUI_ICroppedRectangle.h"
#include "MyGUI_SubSkin.h"
#include "MyGUI_RenderFormat.h"

namespace MyGUI
{

	class MYGUI_EXPORT RawRect : public SubSkin
	{
		MYGUI_RTTI_DERIVED( RawRect )

	public:
		RawRect();
		virtual ~RawRect();

		virtual void setAlpha(float _alpha);

		void setRectColour(const Colour& _colourLT, const Colour& _colourRT, const Colour& _colourLB, const Colour& _colourRB);

		void setRectTexture(const FloatPoint& _pointLT, const FloatPoint& _pointRT, const FloatPoint& _pointLB, const FloatPoint& _pointRB);

		virtual void setStateData(IStateInfo * _data);

		// метод для отрисовки себя
		virtual void doRender();

	private:
		FloatPoint mRectTextureLT;
		FloatPoint mRectTextureRT;
		FloatPoint mRectTextureLB;
		FloatPoint mRectTextureRB;

		Colour mColourLT;
		Colour mColourRT;
		Colour mColourLB;
		Colour mColourRB;

		uint32 mRenderColourLT;
		uint32 mRenderColourRT;
		uint32 mRenderColourLB;
		uint32 mRenderColourRB;

		VertexColourType mVertexFormat;
	};

} // namespace MyGUI

#endif // __MYGUI_RAW_RECT_H__
