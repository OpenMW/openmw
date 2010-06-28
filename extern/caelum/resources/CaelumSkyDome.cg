/*
This file is part of Caelum.
See http://www.ogre3d.org/wiki/index.php/Caelum 

Copyright (c) 2006-2007 Caelum team. See Contributors.txt for details.

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

float bias (float b, float x)
{
	return pow (x, log (b) / log (0.5));
}

float4 sunlightInscatter
(
    float4 sunColour,
    float absorption,
    float incidenceAngleCos,
    float sunlightScatteringFactor
)
{
	float scatteredSunlight = bias (sunlightScatteringFactor * 0.5, incidenceAngleCos);

	sunColour = sunColour * (1 - absorption) * float4 (0.9, 0.5, 0.09, 1);
	
    return sunColour * scatteredSunlight;
}

float fogExp (float z, float density) {
	return 1 - clamp (pow (2.71828, -z * density), 0, 1);
}

void SkyDomeVP
(
    in float4 position : POSITION,
    in float4 normal : NORMAL,
    in float2 uv : TEXCOORD0,

    uniform float lightAbsorption,
    uniform float4x4 worldViewProj,
    uniform float3 sunDirection,

    out float4 oPosition : POSITION,
    out float4 oCol : COLOR, 
    out float2 oUv : TEXCOORD0,
    out float incidenceAngleCos : TEXCOORD1,
    out float y : TEXCOORD2, 
    out float3 oNormal : TEXCOORD3
)
{
	sunDirection = normalize (sunDirection);
	normal = normalize (normal);
	float cosine = dot (-sunDirection, normal);
	incidenceAngleCos = -cosine;

	y = -sunDirection.y;

	oPosition = mul (worldViewProj, position);
	oCol = float4 (1, 1, 1, 1);
	oUv = uv;
	oNormal = -normal.xyz;
}

void SkyDomeFP
(
    float4 col : COLOR, 
    float2 uv : TEXCOORD0,
    float incidenceAngleCos : TEXCOORD1,
    float y : TEXCOORD2, 
    float3 normal : TEXCOORD3, 

    uniform sampler gradientsMap : register(s0), 
    uniform sampler1D atmRelativeDepth : register(s1), 
    uniform float4 hazeColour, 
    uniform float offset,

    out float4 oCol : COLOR
)
{
	float4 sunColour = float4 (3, 3, 3, 1);

#ifdef HAZE
	float fogDensity = 15;
	// Haze amount calculation
	float invHazeHeight = 100;
	float haze = fogExp (pow (clamp (1 - normal.y, 0, 1), invHazeHeight), fogDensity);
#endif // HAZE

	// Pass the colour
	oCol = tex2D (gradientsMap, uv + float2 (offset, 0)) * col;

	// Sunlight inscatter
	if (incidenceAngleCos > 0)
    {
		float sunlightScatteringFactor = 0.05;
		float sunlightScatteringLossFactor = 0.1;
		float atmLightAbsorptionFactor = 0.1;
		
		oCol.rgb += sunlightInscatter (
                sunColour, 
                clamp (atmLightAbsorptionFactor * (1 - tex1D (atmRelativeDepth, y).r), 0, 1), 
                clamp (incidenceAngleCos, 0, 1), 
                sunlightScatteringFactor).rgb * (1 - sunlightScatteringLossFactor);
	}

#ifdef HAZE
	// Haze pass
	hazeColour.a = 1;
	oCol = oCol * (1 - haze) + hazeColour * haze;
#endif // HAZE
}

void HazeVP
(
        in float4 position : POSITION,
		in float4 normal : NORMAL,

        uniform float4x4 worldViewProj,
        uniform float4 camPos, 
        uniform float3 sunDirection,

        out float4 oPosition : POSITION,
        out float haze : TEXCOORD0, 
        out float2 sunlight : TEXCOORD1
)
{
	sunDirection = normalize (sunDirection);
	oPosition = mul(worldViewProj, position);
	haze = length (camPos - position);
	sunlight.x = dot (-sunDirection, normalize (position - camPos));
	sunlight.y = -sunDirection.y;
}

void HazeFP
(
        in float haze : TEXCOORD0, 
        in float2 sunlight : TEXCOORD1, 

        uniform sampler1D atmRelativeDepth : register(s0), 
        uniform sampler2D gradientsMap : register (s1), 
        uniform float4 fogColour,

        out float4 oCol : COLOR
)
{
	float incidenceAngleCos = sunlight.x;
	float y = sunlight.y;

	float4 sunColour = float4 (3, 2.5, 1, 1);

    // Factor determining the amount of light lost due to absorption
	float atmLightAbsorptionFactor = 0.1; 
	float fogDensity = 15;

	haze = fogExp (haze * 0.005, atmLightAbsorptionFactor);

	// Haze amount calculation
	float invHazeHeight = 100;
	float hazeAbsorption = fogExp (pow (1 - y, invHazeHeight), fogDensity);

	float4 hazeColour;
	hazeColour = fogColour;
	if (incidenceAngleCos > 0) {
        // Factor determining the amount of scattering for the sun light
		float sunlightScatteringFactor = 0.1;
        // Factor determining the amount of sun light intensity lost due to scattering
		float sunlightScatteringLossFactor = 0.3;	

		float4 sunlightInscatterColour = sunlightInscatter (
                sunColour, 
                clamp ((1 - tex1D (atmRelativeDepth, y).r) * hazeAbsorption, 0, 1), 
                clamp (incidenceAngleCos, 0, 1), 
                sunlightScatteringFactor) * (1 - sunlightScatteringLossFactor);
		hazeColour.rgb =
                hazeColour.rgb * (1 - sunlightInscatterColour.a) +
                sunlightInscatterColour.rgb * sunlightInscatterColour.a * haze;
	}

	oCol = hazeColour;
	oCol.a = haze;
}
