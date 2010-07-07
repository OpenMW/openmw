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
#include "MyGUI_SubWidgetManager.h"
#include "MyGUI_FactoryManager.h"
#include "MyGUI_CommonStateInfo.h"

#include "MyGUI_SubSkin.h"
#include "MyGUI_MainSkin.h"
#include "MyGUI_RotatingSkin.h"
#include "MyGUI_SimpleText.h"
#include "MyGUI_EditText.h"
#include "MyGUI_RawRect.h"
#include "MyGUI_TileRect.h"

namespace MyGUI
{

	MYGUI_INSTANCE_IMPLEMENT( SubWidgetManager )

	void SubWidgetManager::initialise()
	{
		MYGUI_ASSERT(!mIsInitialise, INSTANCE_TYPE_NAME << " initialised twice");
		MYGUI_LOG(Info, "* Initialise: " << INSTANCE_TYPE_NAME);

		FactoryManager& factory = FactoryManager::getInstance();

		factory.registerFactory<SubSkinStateInfo>("BasisSkin/State", "SubSkin");
		factory.registerFactory<SubSkinStateInfo>("BasisSkin/State", "MainSkin");
		factory.registerFactory<SubSkinStateInfo>("BasisSkin/State", "RotatingSkin");
		factory.registerFactory<SubSkinStateInfo>("BasisSkin/State", "RawRect");
		factory.registerFactory<TileRectStateInfo>("BasisSkin/State", "TileRect");
		factory.registerFactory<EditTextStateInfo>("BasisSkin/State", "EditText");
		factory.registerFactory<EditTextStateInfo>("BasisSkin/State", "SimpleText");

		factory.registerFactory<SubSkin>("BasisSkin");
		factory.registerFactory<MainSkin>("BasisSkin");
		factory.registerFactory<RotatingSkin>("BasisSkin");
		factory.registerFactory<RawRect>("BasisSkin");
		factory.registerFactory<TileRect>("BasisSkin");
		factory.registerFactory<EditText>("BasisSkin");
		factory.registerFactory<SimpleText>("BasisSkin");

		MYGUI_LOG(Info, INSTANCE_TYPE_NAME << " successfully initialized");
		mIsInitialise = true;
	}

	void SubWidgetManager::shutdown()
	{
		if (!mIsInitialise) return;
		MYGUI_LOG(Info, "* Shutdown: " << INSTANCE_TYPE_NAME);

		FactoryManager& factory = FactoryManager::getInstance();

		factory.unregisterFactory("BasisSkin/State", "SubSkin");
		factory.unregisterFactory("BasisSkin/State", "MainSkin");
		factory.unregisterFactory("BasisSkin/State", "RotatingSkin");
		factory.unregisterFactory("BasisSkin/State", "RawRect");
		factory.unregisterFactory("BasisSkin/State", "TileRect");
		factory.unregisterFactory("BasisSkin/State", "EditText");
		factory.unregisterFactory("BasisSkin/State", "SimpleText");

		factory.unregisterFactory<SubSkin>("BasisSkin");
		factory.unregisterFactory<MainSkin>("BasisSkin");
		factory.unregisterFactory<RotatingSkin>("BasisSkin");
		factory.unregisterFactory<RawRect>("BasisSkin");
		factory.unregisterFactory<TileRect>("BasisSkin");
		factory.unregisterFactory<EditText>("BasisSkin");
		factory.unregisterFactory<SimpleText>("BasisSkin");

		MYGUI_LOG(Info, INSTANCE_TYPE_NAME << " successfully shutdown");
		mIsInitialise = false;
	}

} // namespace MyGUI
