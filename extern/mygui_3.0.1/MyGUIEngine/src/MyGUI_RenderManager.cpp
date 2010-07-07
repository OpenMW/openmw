/*!
	@file
	@author		Albert Semenov
	@date		04/2009
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
#include "MyGUI_RenderManager.h"

namespace MyGUI
{

	const std::string INSTANCE_TYPE_NAME("RenderManager");

	RenderManager* RenderManager::msInstance = 0;

	RenderManager::RenderManager() :
		mIsInitialise(false)
	{
		MYGUI_ASSERT(0 == msInstance, "instance " << INSTANCE_TYPE_NAME << " is exsist");
		msInstance = this;
	}

	RenderManager::~RenderManager()
	{
		msInstance = 0;
	}

	RenderManager* RenderManager::getInstancePtr()
	{
		return msInstance;
	}

	RenderManager& RenderManager::getInstance()
	{
		MYGUI_ASSERT(0 != msInstance, "instance " << INSTANCE_TYPE_NAME << " was not created");
		return (*msInstance);
	}

} // namespace MyGUI
