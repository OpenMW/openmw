/*
This file is part of Caelum.
See http://www.ogre3d.org/wiki/index.php/Caelum 

Copyright (c) 2008 Caelum team. See Contributors.txt for details.

Caelum is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Caelum is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Caelum. If not, see <http://www.gnu.org/licenses/>.
*/

void StarPointVP
(
	in float4 in_position : POSITION,
	in float3 in_texcoord : TEXCOORD0,
	
	uniform float4x4 worldviewproj_matrix,

    // These params are in clipspace; not pixels
    uniform float mag_scale,
    uniform float mag0_size,
    uniform float min_size,
    uniform float max_size,
    uniform float render_target_flipping,

    // width/height
    uniform float aspect_ratio,
	
	out float2 out_texcoord : TEXCOORD0,
	out float4 out_position : POSITION,
	out float4 out_color : COLOR
)
{
    float4 in_color = float4(1, 1, 1, 1);
    out_position = mul(worldviewproj_matrix, in_position);
    out_texcoord = in_texcoord.xy;

    float magnitude = in_texcoord.z;
    float size = exp(mag_scale * magnitude) * mag0_size;

    // Fade below minSize.
    float fade = saturate(size / min_size);
    out_color = float4(in_color.rgb, fade * fade);

    // Clamp size to range.
    size = clamp(size, min_size, max_size);

    // Splat the billboard on the screen.
    out_position.xy +=
            out_position.w *
            in_texcoord.xy *
            float2(size, size * aspect_ratio * render_target_flipping);
}

void StarPointFP
(
	in float4 in_color : COLOR,
	in float2 in_texcoord : TEXCOORD0,

	out float4 out_color : COLOR
)
{
    out_color = in_color;
    float sqlen = dot(in_texcoord, in_texcoord);

    // A gaussian bell of sorts.
    out_color.a *= 1.5 * exp(-(sqlen * 8));
}
