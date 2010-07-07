/*!
	@file
	@author		Denis Koronchik
	@date		09/2007
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
#ifndef __MYGUI_PLUGIN_H__
#define __MYGUI_PLUGIN_H__

#include "MyGUI_Prerequest.h"
#include <string>

namespace MyGUI
{

	/*!	\brief Base plugin class
	*/
	class MYGUI_EXPORT IPlugin
	{
	public:
		IPlugin() { }

		virtual ~IPlugin() { }

		/*!	Get the name of the plugin.
			@remarks An implementation must be supplied for this method to uniquely
			identify the plugin
		*/
		virtual const std::string& getName() const = 0;

		/*!	Perform the plugin initial installation sequence
		*/
		virtual void install() = 0;

		/*! Perform any tasks the plugin needs to perform on full system
			initialisation.
		*/
		virtual void initialize() = 0;

		/*!	Perform any tasks the plugin needs to perform when the system is shut down
		*/
		virtual void shutdown() = 0;

		/*!	Perform the final plugin uninstallation sequence
		*/
		virtual void uninstall() = 0;
	};

}

#endif // __MYGUI_PLUGIN_H__
