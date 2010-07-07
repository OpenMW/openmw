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
#ifndef __MYGUI_DELEGATE_H__
#define __MYGUI_DELEGATE_H__

#include "MyGUI_Diagnostic.h"
#include <typeinfo>
#include <list>

// source
// http://rsdn.ru/article/cpp/delegates.xml

// генерация делегатов для различного колличества параметров
namespace MyGUI
{

	namespace delegates
	{
		// базовый класс для тех классов, что хотят себя отвязывать от мульти делегатов
		class MYGUI_EXPORT IDelegateUnlink
		{
		public:
			virtual ~IDelegateUnlink() { }

			IDelegateUnlink() { m_baseDelegateUnlink = this; }
			bool compare(IDelegateUnlink * _unlink) const { return m_baseDelegateUnlink == _unlink->m_baseDelegateUnlink; }

		private:
			IDelegateUnlink * m_baseDelegateUnlink;
		};

		inline IDelegateUnlink * GetDelegateUnlink(void * _base) { return 0; }
		inline IDelegateUnlink * GetDelegateUnlink(IDelegateUnlink * _base) { return _base; }
	}

	// без параметров
	#define MYGUI_SUFFIX       0
	#define MYGUI_TEMPLATE
	#define MYGUI_TEMPLATE_PARAMS
	#define MYGUI_TEMPLATE_ARGS
	#define MYGUI_T_TEMPLATE_PARAMS  <typename T>
	#define MYGUI_T_TEMPLATE_ARGS <T>
	#define MYGUI_PARAMS
	#define MYGUI_ARGS
	#define MYGUI_TYPENAME

	#include "MyGUI_DelegateImplement.h"

	// один параметр
	#define MYGUI_SUFFIX       1
	#define MYGUI_TEMPLATE	template
	#define MYGUI_TEMPLATE_PARAMS  <typename TP1>
	#define MYGUI_TEMPLATE_ARGS    <TP1>
	#define MYGUI_T_TEMPLATE_PARAMS  <typename T, typename TP1>
	#define MYGUI_T_TEMPLATE_ARGS    <T, TP1>
	#define MYGUI_PARAMS       TP1 p1
	#define MYGUI_ARGS         p1
	#define MYGUI_TYPENAME     typename

	#include "MyGUI_DelegateImplement.h"

	// два параметра
	#define MYGUI_SUFFIX       2
	#define MYGUI_TEMPLATE	template
	#define MYGUI_TEMPLATE_PARAMS  <typename TP1, typename TP2>
	#define MYGUI_TEMPLATE_ARGS    <TP1, TP2>
	#define MYGUI_T_TEMPLATE_PARAMS  <typename T, typename TP1, typename TP2>
	#define MYGUI_T_TEMPLATE_ARGS    <T, TP1, TP2>
	#define MYGUI_PARAMS       TP1 p1, TP2 p2
	#define MYGUI_ARGS         p1, p2
	#define MYGUI_TYPENAME     typename

	#include "MyGUI_DelegateImplement.h"

	// три параметра
	#define MYGUI_SUFFIX       3
	#define MYGUI_TEMPLATE	template
	#define MYGUI_TEMPLATE_PARAMS  <typename TP1, typename TP2, typename TP3>
	#define MYGUI_TEMPLATE_ARGS    <TP1, TP2, TP3>
	#define MYGUI_T_TEMPLATE_PARAMS  <typename T, typename TP1, typename TP2, typename TP3>
	#define MYGUI_T_TEMPLATE_ARGS    <T, TP1, TP2, TP3>
	#define MYGUI_PARAMS       TP1 p1, TP2 p2, TP3 p3
	#define MYGUI_ARGS         p1, p2, p3
	#define MYGUI_TYPENAME     typename

	#include "MyGUI_DelegateImplement.h"

	// четыре параметра
	#define MYGUI_SUFFIX       4
	#define MYGUI_TEMPLATE	template
	#define MYGUI_TEMPLATE_PARAMS  <typename TP1, typename TP2, typename TP3, typename TP4>
	#define MYGUI_TEMPLATE_ARGS    <TP1, TP2, TP3, TP4>
	#define MYGUI_T_TEMPLATE_PARAMS  <typename T, typename TP1, typename TP2, typename TP3, typename TP4>
	#define MYGUI_T_TEMPLATE_ARGS    <T, TP1, TP2, TP3, TP4>
	#define MYGUI_PARAMS       TP1 p1, TP2 p2, TP3 p3, TP4 p4
	#define MYGUI_ARGS         p1, p2, p3, p4
	#define MYGUI_TYPENAME     typename

	#include "MyGUI_DelegateImplement.h"

	// пять параметров
	#define MYGUI_SUFFIX       5
	#define MYGUI_TEMPLATE	template
	#define MYGUI_TEMPLATE_PARAMS  <typename TP1, typename TP2, typename TP3, typename TP4, typename TP5>
	#define MYGUI_TEMPLATE_ARGS    <TP1, TP2, TP3, TP4, TP5>
	#define MYGUI_T_TEMPLATE_PARAMS  <typename T, typename TP1, typename TP2, typename TP3, typename TP4, typename TP5>
	#define MYGUI_T_TEMPLATE_ARGS    <T, TP1, TP2, TP3, TP4, TP5>
	#define MYGUI_PARAMS       TP1 p1, TP2 p2, TP3 p3, TP4 p4, TP5 p5
	#define MYGUI_ARGS         p1, p2, p3, p4, p5
	#define MYGUI_TYPENAME     typename

	#include "MyGUI_DelegateImplement.h"


} // namespace MyGUI

#endif // __MYGUI_DELEGATE_H__
