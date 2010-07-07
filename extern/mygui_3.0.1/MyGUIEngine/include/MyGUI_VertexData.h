/*!
	@file
	@author		Albert Semenov
	@date		05/2009
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
#ifndef __MYGUI_VERTEX_DATA_H__
#define __MYGUI_VERTEX_DATA_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Types.h"

namespace MyGUI
{

	struct Vertex
	{
		void set(float _x, float _y, float _z, float _u, float _v, uint32 _colour)
		{
			x = _x;
			y = _y;
			z = _z;
			u = _u;
			v = _v;
			colour = _colour;
		}

		float x, y, z;
		uint32 colour;
		float u, v;
	};

	struct VertexQuad
	{
		enum Enum
		{
			CornerLT = 0,
			CornerRT = 1,
			CornerLB = 2,
			CornerLB2 = 3,
			CornerRT2 = 4,
			CornerRB = 5,
			VertexCount = 6
		};

		void set(float _l, float _t, float _r, float _b, float _z, float _u1, float _v1, float _u2, float _v2, uint32 _colour)
		{
			vertex[CornerLT].set(_l, _t, _z, _u1, _v1, _colour);
			vertex[CornerRT].set(_r, _t, _z, _u2, _v1, _colour);
			vertex[CornerLB].set(_l, _b, _z, _u1, _v2, _colour);
			vertex[CornerRB].set(_r, _b, _z, _u2, _v2, _colour);
			vertex[CornerRT2] = vertex[CornerRT];
			vertex[CornerLB2] = vertex[CornerLB];
		}

		void set(float _x1, float _y1, float _x2, float _y2, float _x3, float _y3, float _x4, float _y4, float _z, float _u1, float _v1, float _u2, float _v2, uint32 _colour)
		{
			vertex[CornerLT].set(_x1, _y1, _z, _u1, _v1, _colour);
			vertex[CornerRT].set(_x2, _y2, _z, _u2, _v1, _colour);
			vertex[CornerLB].set(_x4, _y4, _z, _u1, _v2, _colour);
			vertex[CornerRB].set(_x3, _y3, _z, _u2, _v2, _colour);
			vertex[CornerRT2] = vertex[CornerRT];
			vertex[CornerLB2] = vertex[CornerLB];
		}

		Vertex vertex[6];
	};

} // namespace MyGUI

#endif // __MYGUI_VERTEX_DATA_H__
