/*!
	@file
	@author		Albert Semenov
	@date		02/2008
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
#ifndef __MYGUI_LAYER_MANAGER_H__
#define __MYGUI_LAYER_MANAGER_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Instance.h"
#include "MyGUI_Enumerator.h"
#include "MyGUI_XmlDocument.h"
#include "MyGUI_IUnlinkWidget.h"
#include "MyGUI_ResourceManager.h"
#include "MyGUI_ILayer.h"

namespace MyGUI
{

	class MYGUI_EXPORT LayerManager :
		public IUnlinkWidget
	{
		MYGUI_INSTANCE_HEADER( LayerManager )

	public:
		typedef std::vector<ILayer*> VectorLayer;
		typedef Enumerator<VectorLayer> EnumeratorLayer;

	public:
		void initialise();
		void shutdown();

		/** Attach widget to specific layer
			@param _name Layer name
			@param _item Widget pointer
		*/
		void attachToLayerNode(const std::string& _name, Widget* _item);
		/** Detach widget from layer
			@param _item Widget pointer
		*/
		void detachFromLayer(Widget* _item);

		/** Up widget to be on top of its layer
			@param _item Widget pointer
		*/
		void upLayerItem(Widget* _item);

		/** Load additional MyGUI *_layer.xml file */
		bool load(const std::string& _file);
		void _load(xml::ElementPtr _node, const std::string& _file, Version _version);

		/** Check is layer exist */
		bool isExist(const std::string& _name) const;
		/** Get layer nodes Enumerator */
		EnumeratorLayer getEnumerator() { return EnumeratorLayer(mLayerNodes); }

		ILayer* getByName(const std::string& _name, bool _throw = true) const;

		/** Get top visible and enabled widget at specified position */
		Widget* getWidgetFromPoint(int _left, int _top);

		void renderToTarget(IRenderTarget* _target, bool _update);

		virtual void dumpStatisticToLog();

	private:
		// удаляем данный виджет из всех возможных мест
		void _unlinkWidget(Widget* _widget);

		void clear();

		void merge(VectorLayer& _layers);
		void destroy(ILayer* _layer);

	private:
		VectorLayer mLayerNodes;

	};

} // namespace MyGUI

#endif // __MYGUI_LAYER_MANAGER_H__
