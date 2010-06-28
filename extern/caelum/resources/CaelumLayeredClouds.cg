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

// Global cloud textures
sampler cloud_shape1 : register(s0);
sampler cloud_shape2 : register(s1);
sampler cloud_detail : register(s2);

// Get cloud layer intensity at a certain point.
float LayeredClouds_intensity
(
        in float2   pos,
        float       cloudMassInvScale,
        float       cloudDetailInvScale,
        float2      cloudMassOffset,
        float2      cloudDetailOffset,
        float       cloudMassBlend,
        float       cloudDetailBlend,
        float       cloudCoverageThreshold
)
{
	// Calculate the base alpha
	float2 finalMassOffset = cloudMassOffset + pos;
	float aCloud = lerp(tex2D(cloud_shape1, finalMassOffset * cloudMassInvScale).r,
						tex2D(cloud_shape2, finalMassOffset * cloudMassInvScale).r, 
						cloudMassBlend);
	float aDetail = tex2D(cloud_detail, (cloudDetailOffset + pos) * cloudDetailInvScale).r;
	aCloud = (aCloud + aDetail * cloudDetailBlend) / (1 + cloudDetailBlend);
	return max(0, aCloud - cloudCoverageThreshold);
}

// Entry point for Cloud vertex program.
void LayeredClouds_vp
(
		in float4 position : POSITION,
		in float2 uv : TEXCOORD0,
		
		uniform float4x4 worldViewProj,
		uniform float4x4 worldMatrix,
		uniform float3 sunDirection,

		out float4 oPosition : POSITION,
		out float2 oUv : TEXCOORD0,
		out float3 relPosition : TEXCOORD1,
		out float sunGlow : TEXCOORD2,
		out float4 worldPosition : TEXCOORD3
) {

	oPosition = mul(worldViewProj, position);
	worldPosition = mul(worldMatrix, position);
	oUv = uv;

    // This is the relative position, or view direction.
	relPosition = normalize (position.xyz);

    // Calculate the angle between the direction of the sun and the current
    // view direction. This we call "glow" and ranges from 1 next to the sun
    // to -1 in the opposite direction.
	sunGlow = dot (relPosition, normalize (-sunDirection));
}

float4 OldCloudColor
(
		float2       uv,
		float3       relPosition,
		float        sunGlow,

        uniform float   cloudMassInvScale,
        uniform float   cloudDetailInvScale,
        uniform float2  cloudMassOffset,
        uniform float2  cloudDetailOffset,
        uniform float   cloudMassBlend,
        uniform float   cloudDetailBlend,

        uniform float   cloudCoverageThreshold,

        uniform float4  sunColour,
        uniform float4  fogColour,
        uniform float   cloudSharpness,
        uniform float   cloudThickness

) {
    // Initialize output.
	float4 oCol = float4(1, 1, 1, 0);
	
	// Get cloud intensity.
	float intensity = LayeredClouds_intensity
    (
            uv,
            cloudMassInvScale,
            cloudDetailInvScale,
            cloudMassOffset,
            cloudDetailOffset,
            cloudMassBlend,
            cloudDetailBlend,
            cloudCoverageThreshold
    );

	// Opacity is exponential.
	float aCloud = saturate(exp(cloudSharpness * intensity) - 1);

	float shine = pow(saturate(sunGlow), 8) / 4;
	sunColour.rgb *= 1.5;
	float3 cloudColour = fogColour.rgb * (1 - intensity / 3);
	float thickness = saturate(0.8 - exp(-cloudThickness * (intensity + 0.2 - shine)));

	oCol.rgb = lerp(sunColour.rgb, cloudColour.rgb, thickness);
	oCol.a = aCloud;	
	
	return oCol;
}

//Converts a color from RGB to YUV color space
//the rgb color is in [0,1] [0,1] [0,1] range
//the yuv color is in [0,1] [-0.436,0.436] [-0.615,0.615] range
float3 YUVfromRGB(float3 col)
{
    return float3(dot(col, float3(0.299,0.587,0.114)),
				  dot(col, float3(-0.14713,-0.28886,0.436)),
				  dot(col, float3(0.615,-0.51499,-0.10001)));
}

float3 RGBfromYUV(float3 col)
{
    return float3(dot(col,float3(1,0,1.13983)),
				  dot(col,float3(1,-0.39465,-0.58060)),
				  dot(col,float3(1,2.03211,0)));
}

// Creates a color that has the intensity of col1 and the chrominance of col2
float3 MagicColorMix(float3 col1, float3 col2)
{
    return saturate(RGBfromYUV(float3(YUVfromRGB(col1).x, YUVfromRGB(col2).yz)));
}

// Entry point for Cloud fragment program.
void LayeredClouds_fp
(
		in float2       uv : TEXCOORD0,
		in float3       relPosition : TEXCOORD1,
		in float        sunGlow : TEXCOORD2,
		in float4 worldPosition : TEXCOORD3,			

        uniform float   cloudMassInvScale,
        uniform float   cloudDetailInvScale,
        uniform float2  cloudMassOffset,
        uniform float2  cloudDetailOffset,
        uniform float   cloudMassBlend,
        uniform float   cloudDetailBlend,

        uniform float   cloudCoverageThreshold,

        uniform float4  sunLightColour,
        uniform float4  sunSphereColour,
        uniform float4  fogColour,
        uniform float4	sunDirection,
        uniform float   cloudSharpness,
        uniform float   cloudThickness,
        uniform float3	camera_position,

        uniform float3	fadeDistMeasurementVector,
        uniform float	layerHeight,
        uniform float	cloudUVFactor,
        uniform float	heightRedFactor,

        uniform float   nearFadeDist,
        uniform float   farFadeDist,

		out float4 oCol : COLOR
) {
	uv *= cloudUVFactor;

    oCol = OldCloudColor(
            uv, relPosition, sunGlow,
		    cloudMassInvScale, cloudDetailInvScale,
            cloudMassOffset, cloudDetailOffset,
            cloudMassBlend, cloudDetailBlend,
            cloudCoverageThreshold,
		    sunLightColour,
            fogColour,
            cloudSharpness,
            cloudThickness);
	oCol.r += layerHeight / heightRedFactor;
	
	//float dist = distance(worldPosition.xyz, camera_position.xyz);
	float dist = length((worldPosition - camera_position) * fadeDistMeasurementVector);
	float aMod = 1;	
	if (dist > nearFadeDist) {
        aMod = saturate(lerp(0, 1, (farFadeDist - dist) / (farFadeDist - nearFadeDist)));
    }
    float alfa = oCol.a * aMod;
	
	float3 cloudDir = normalize(
	         float3(worldPosition.x, layerHeight, worldPosition.y) - camera_position);
	float angleDiff = saturate(dot(cloudDir, normalize(sunDirection.xyz)));
	
	float3 lCol = lerp(oCol.rgb, MagicColorMix(oCol.rgb, sunSphereColour.rgb), angleDiff);
	oCol.rgb = lerp(lCol, oCol.rgb, alfa);
	oCol.a = alfa;
}
