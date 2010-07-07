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
#ifndef __MYGUI_INSTANCE_H__
#define __MYGUI_INSTANCE_H__

#define MYGUI_INSTANCE_HEADER(type) \
	private: \
		static type* msInstance; \
		bool mIsInitialise; \
	public: \
		type();\
		~type();\
		static type& getInstance(); \
		static type* getInstancePtr();


#define MYGUI_INSTANCE_IMPLEMENT(type) \
	const std::string INSTANCE_TYPE_NAME(#type); \
	type* type::msInstance = nullptr; \
	type* type::getInstancePtr() { return msInstance; } \
	type& type::getInstance() { MYGUI_ASSERT(0 != msInstance, "instance " << INSTANCE_TYPE_NAME << " was not created"); return (*msInstance); } \
	type::type() : mIsInitialise(false) { MYGUI_ASSERT(0 == msInstance, "instance " << INSTANCE_TYPE_NAME << " is exsist"); msInstance = this; } \
	type::~type() { msInstance = nullptr; }

#endif // __MYGUI_INSTANCE_H__
