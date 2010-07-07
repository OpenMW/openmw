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
#ifndef __MYGUI_I_SUB_WIDGET_H__
#define __MYGUI_I_SUB_WIDGET_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_ICroppedRectangle.h"
#include "MyGUI_ILayerNode.h"
#include "MyGUI_Types.h"
#include "MyGUI_IRenderTarget.h"
#include "MyGUI_IStateInfo.h"
#include "MyGUI_IObject.h"

namespace MyGUI
{

	class ISubWidget;
	typedef std::vector<ISubWidget*> VectorSubWidget;

	class MYGUI_EXPORT ISubWidget :
		public ICroppedRectangle,
		public IObject
	{
		MYGUI_RTTI_DERIVED( ISubWidget )

	public:
		virtual ~ISubWidget() { }

		virtual void createDrawItem(ITexture* _texture, ILayerNode* _node) = 0;
		virtual void destroyDrawItem() = 0;

		virtual void setAlpha(float _alpha) { }

		virtual void setStateData(IStateInfo* _data) { }

		virtual void doRender() = 0;

	};

} // namespace MyGUI

#endif // __MYGUI_I_SUB_WIDGET_H__
