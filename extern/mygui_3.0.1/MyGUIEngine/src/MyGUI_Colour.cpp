/*!
	@file
	@author		Albert Semenov
	@date		12/2008
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
#include "MyGUI_Colour.h"

namespace MyGUI
{

	const Colour Colour::Zero = Colour(0, 0, 0, 0);
	const Colour Colour::Black = Colour(0, 0, 0, 1);
	const Colour Colour::White = Colour(1, 1, 1, 1);
	const Colour Colour::Red = Colour(1, 0, 0, 1);
	const Colour Colour::Green = Colour(0, 1, 0, 1);
	const Colour Colour::Blue = Colour(0, 0, 1, 1);

} // namespace MyGUI
